#include "shelf.hpp"

#include <cstdint>
#include <unordered_map>
#include <inttypes.h>
#include <limits>

#include "login.hpp"
#include "utils.hpp"

// This should be defined in inttypes.h, but some old compilers don't have that.
#ifndef PRId64
#define PRId64 "lld"
#endif // PRId64

namespace shelf {

bool SQLUpsertOneRow(std::string query) {
    if (SQL_Query(query) != 0) {
        return false;
    }

    int affected_rows = SQL_AffectedRows();
    if (affected_rows != 1) {
        Printf(LOG_Error, "[shelf] Concurrent modification? SQL query '%s' affected %d rows, wanted 1", query.c_str(), affected_rows);
        return false;
    }

    return true;
}

bool ItemsToSavingsBook(int login_id, ServerIDType server_id, std::vector<CItem>& inventory) {
    return impl::ItemsToSavingsBookImpl(login_id, server_id, inventory, impl::LoadShelf, SQLUpsertOneRow);
}

bool ItemsFromSavingsBook(int login_id, ServerIDType server_id, std::vector<CItem>& inventory) {
    return impl::ItemsFromSavingsBookImpl(login_id, server_id, inventory, impl::LoadShelf, SQLUpsertOneRow);
}

int32_t MoneyToSavingsBook(int login_id, ServerIDType server_id, std::vector<CItem>& inventory, int32_t current_money, int32_t amount) {
    return impl::MoneyToSavingsBookImpl(login_id, server_id, inventory, current_money, amount, impl::LoadShelf, SQLUpsertOneRow);
}

int32_t MoneyFromSavingsBook(int login_id, ServerIDType server_id, std::vector<CItem>& inventory, int32_t current_money, int32_t amount) {
    return impl::MoneyFromSavingsBookImpl(login_id, server_id, inventory, current_money, amount, impl::LoadShelf, SQLUpsertOneRow);
}

namespace impl {

bool IsSavingsBook(const CItem& item) {
    return item.Id == 3587 && // Book of Fire
        item.IsMagic &&
        item.Effects.size() == 1 && item.Effects[0].Id1 == 42 && item.Effects[0].Value1 == 1; // Casts fire arrow spell.
}

std::vector<CItem>::const_iterator FindSavingsBook(const std::vector<CItem>& inventory) {
    for (auto it = inventory.begin(); it != inventory.end(); ++it) {
        if (IsSavingsBook(*it)) {
            return it;
        }
    }
    return inventory.end();
}

ServerIDType FixServerID(ServerIDType server_id) {
    // Nightmare+ servers use the same shelf.
    if (server_id >= NIGHTMARE) {
        return NIGHTMARE;
    }
    return server_id;
}

// Merge piles of identical items (example: item of 20 health potions and item
// of 40 same health potions will be merged to one item of 60 health potions).
// Maybe the engine can handle duplicate piles, but I'm too lazy to test.
// Modifies items in-place.
void MergeItemPiles(std::vector<CItem>& items, const std::vector<CItem>& add_items) {
    // Ensure that `items` won't be reallocated when adding elements. This is faster and also preserves iterators.
    items.reserve(items.size() + add_items.size());

    // Remember every non-magic pile.
    std::unordered_map<uint16_t, std::vector<CItem>::iterator> piles;

    for (auto it = items.begin(); it != items.end(); ++it) {
        if (!it->IsMagic) {
            piles[it->Id] = it;
        }
    }

    for (const auto& item : add_items) {
        if (item.IsMagic || piles.count(item.Id) == 0) {
            items.push_back(item);
        } else {
            // There's an existing pile of this item --- add to it.
            piles[item.Id]->Count += item.Count;
        }
    }
}

bool LoadShelf(int login_id, ServerIDType server_id, Field field, int32_t* mutex, std::string* items, int64_t* money, bool& shelf_exists) {
    const char* field_names = "";
    switch (field) {
    case ITEMS:
        field_names = "items";
        break;
    case MONEY:
        field_names = "money";
        break;
    case BOTH:
        field_names = "items, money";
        break;
    }

    std::string query = Format("SELECT mutex, %s FROM shelf WHERE login_id = %d AND server_id = %d", field_names, login_id, server_id);
    if (SQL_Query(query) != 0) {
        Printf(LOG_Error, "Failed to query shelf content for player %d and server %d: %s\n", login_id, server_id, SQL_Error().c_str());
        return false;
    }

    auto result = SQL_StoreResult();
    if (!result) {
        Printf(LOG_Error, "Failed to get query result for shelf content for player %d and server %d: %s\n", login_id, server_id, SQL_Error().c_str());
        return false;
    }

    shelf_exists = false;
    if (SQL_NumRows(result) != 0) {
        shelf_exists = true;

        auto row = SQL_FetchRow(result);

        if (mutex != nullptr) {
            *mutex = SQL_FetchInt(row, result, "mutex");
        }

        if (field & Field::ITEMS && items != nullptr) {
            *items = SQL_FetchString(row, result, "items");
        }
        if (field & Field::MONEY && money != nullptr) {
            *money = SQL_FetchInt64(row, result, "money");
        }
    }

    SQL_FreeResult(result);

    return true;
}

bool SaveShelf(int login_id, ServerIDType server_id, Field field, int32_t mutex, std::vector<CItem> new_items, int64_t money, bool shelf_exists, SQLQueryFunction sql_query) {
    std::string items;
    if (field & Field::ITEMS) {
        CItemList item_list{.Items = std::move(new_items)};
        items = SQL_Escape(Login_SerializeItems(item_list));
    }

    std::string query;
    std::string field_values;

    if (shelf_exists) {
        switch (field) {
        case ITEMS:
            field_values = Format("items = '%s'", items.c_str());
            break;
        case MONEY:
            field_values = Format("money = %" PRId64, money);
            break;
        case BOTH:
            field_values = Format("items = '%s', money = %d", items.c_str(), money);
            break;
        }

        query = Format("UPDATE shelf SET mutex = %d, %s WHERE login_id = %d AND server_id = %d AND mutex = %d", mutex + 1, field_values.c_str(), login_id, server_id, mutex);
    } else {
        std::string field_names;
        
        switch (field) {
        case ITEMS:
            field_names = "items";
            field_values = Format("'%s'", items.c_str());
            break;
        case MONEY:
            field_names = "money";
            field_values = Format("%" PRId64, money);
            break;
        case BOTH:
            field_names = "items, money";
            field_values = Format("'%s', %" PRId64, items.c_str(), money);
            break;
        }
        
        query = Format("INSERT INTO shelf (login_id, server_id, mutex, %s) VALUES (%d, %d, %d, %s)", field_names.c_str(), login_id, server_id, mutex, field_values.c_str());
    }

    if (!sql_query(query)) {
        Printf(LOG_Error, "[shelf] Failed to save new shelf content for player %d and server %d: '%s' (shelf existed: %d, new fields: %s)\n", login_id, server_id, SQL_Error().c_str(), shelf_exists, field_values.c_str());
        return false;
    }

    Printf(LOG_Info, "[shelf] Saved new shelf content for login %d on server %d: %s\n", login_id, server_id, field_values.c_str());
    return true;
}

bool ItemsToSavingsBookImpl(int login_id, ServerIDType server_id, std::vector<CItem>& inventory, LoadShelfFunction load_shelf, SQLQueryFunction sql_query) {
    server_id = FixServerID(server_id);

    auto savings_book = FindSavingsBook(inventory);
    if (savings_book == inventory.end()) {
        return true;
    }

    std::vector<CItem> to_shelf(inventory.cbegin(), savings_book);

    int32_t mutex = 0;
    std::string shelf_content;
    bool shelf_exists = false;
    if (!load_shelf(login_id, server_id, Field::ITEMS, &mutex, &shelf_content, nullptr, shelf_exists)) {
        return false;
    }

    std::vector<CItem> current_items = Login_UnserializeItems(shelf_content).Items;

    MergeItemPiles(current_items, to_shelf);

    if (!SaveShelf(login_id, server_id, Field::ITEMS, mutex, std::move(current_items), 0, shelf_exists, sql_query)) {
        return false;
    }

    // +1 to remove the book too.
    inventory.erase(inventory.begin(), savings_book + 1);
    return true;
}

bool ItemsFromSavingsBookImpl(int login_id, ServerIDType server_id, std::vector<CItem>& inventory, LoadShelfFunction load_shelf, SQLQueryFunction sql_query) {
    server_id = FixServerID(server_id);

    auto savings_book = FindSavingsBook(inventory);
    if (savings_book == inventory.end()) {
        return true;
    }

    int32_t mutex = 0;
    std::string shelf_content;
    bool shelf_exists;
    if (!load_shelf(login_id, server_id, Field::ITEMS, &mutex, &shelf_content, nullptr, shelf_exists)) {
        return false;
    }

    std::vector<CItem> shelved_items = Login_UnserializeItems(shelf_content).Items;

    if (!shelf_exists || shelved_items.empty()) {
        return true;
    }

    if (!SaveShelf(login_id, server_id, Field::ITEMS, mutex, {}, 0, shelf_exists, sql_query)) {
        return false;
    }

    Printf(LOG_Info, "[shelf] Loaded items for login %d on server %d: '%s'\n", login_id, server_id, SQL_Escape(shelf_content).c_str());

    // Remove the book.
    inventory.erase(savings_book);

    // Put items from the shelf into the inventory.
    MergeItemPiles(inventory, shelved_items);

    return true;
}

int32_t MoneyToSavingsBookImpl(int login_id, ServerIDType server_id, std::vector<CItem>& inventory, int32_t current_money, int32_t amount, LoadShelfFunction load_shelf, SQLQueryFunction sql_query) {
    server_id = FixServerID(server_id);

    if (amount == 0) {
        return current_money;
    }

    int32_t deposit = amount;
    if (deposit > current_money) {
        deposit = current_money;
    }

    auto savings_book = FindSavingsBook(inventory);
    if (savings_book == inventory.end()) {
        return current_money;
    }

    int32_t mutex = 0;
    int64_t shelved_money = 0;
    bool shelf_exists;
    if (!load_shelf(login_id, server_id, Field::MONEY, &mutex, nullptr, &shelved_money, shelf_exists)) {
        return current_money;
    }

    int64_t money = shelved_money + deposit;

    if (!SaveShelf(login_id, server_id, Field::MONEY, mutex, {}, money, shelf_exists, sql_query)) {
        return current_money;
    }

    Printf(LOG_Info, "[shelf] Stored money for login %d on server %d: %d stashed (tried %d), total %" PRId64 "\n", login_id, server_id, deposit, amount, money);

    // Remove the book.
    inventory.erase(savings_book);

    return current_money - deposit;
}

int32_t MoneyFromSavingsBookImpl(int login_id, ServerIDType server_id, std::vector<CItem>& inventory, int32_t current_money, int32_t amount, LoadShelfFunction load_shelf, SQLQueryFunction sql_query) {
    server_id = FixServerID(server_id);

    if (amount == 0) {
        return current_money;
    }

    auto savings_book = FindSavingsBook(inventory);
    if (savings_book == inventory.end()) {
        return current_money;
    }

    int32_t mutex = 0;
    int64_t shelved_money = 0;
    bool shelf_exists;
    if (!load_shelf(login_id, server_id, Field::MONEY, &mutex, nullptr, &shelved_money, shelf_exists)) {
        return current_money;
    }

    int32_t withdraw = std::min(amount, std::numeric_limits<int32_t>::max() - current_money);
    Printf(0, "I have %d and want %d. Can get up to %d\n", current_money, amount, withdraw);
    if (static_cast<int64_t>(withdraw) > shelved_money) {
        Printf(0, "We have %lld shelved. Now I can withdraw up to %d\n", shelved_money, withdraw);
        withdraw = static_cast<int32_t>(shelved_money);
    }

    if (withdraw == 0) {
        return current_money;
    }

    int64_t money = shelved_money - static_cast<int64_t>(withdraw);
    
    Printf(0, "%lld - %d = %lld\n", shelved_money, withdraw, money);

    if (!SaveShelf(login_id, server_id, Field::MONEY, mutex, {}, money, shelf_exists, sql_query)) {
        return current_money;
    }

    Printf(LOG_Info, "[shelf] Loaded money for login %d on server %d: %d withdrawn (tried %d), left %" PRId64 " stashed\n", login_id, server_id, withdraw, amount, money);

    // Remove the book.
    inventory.erase(savings_book);

    return current_money + withdraw;
}

} // namespace impl
} // namespace shelf
