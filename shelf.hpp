#pragma once

#include <functional>

#include "CCharacter.hpp"
#include "server_id.hpp"

namespace shelf {

// Store the items from the player's inventory to the shelf.
//
// Checks if a "savings book" (a magic book of fire missile) is present. If it
// is, takes all items to the left of it and stores them to the database.
// The book gets deleted.
// 
// If DB operation fails, returns `false` and doesn't modify the inventory.
// If there's no book, returns `true` and doesn't modify the inventory.
bool ItemsToSavingsBook(const CCharacter& chr, ServerIDType server_id, std::vector<CItem>& inventory);

// Take the items from the shelf to the player's inventory.
//
// Checks if a "savings book" (a magic book of fire missile) is present. If it
// is, takes all items from the shelf and puts them into the inventory, merging
// the matching item packs.
// The book gets deleted if the shelf existed and wasn't empty.
//
// If DB operation fails, returns `false` and doesn't modify the inventory.
// If there's no book, returns `true` and doesn't modify the inventory.
bool ItemsFromSavingsBook(const CCharacter& chr, ServerIDType server_id, std::vector<CItem>& inventory);

// Store `amount` of gold on the shelf.
//
// Checks if a "savings book" (a magic book of fire missile) is present. If it
// is, takes `amount` of gold, puts it onto the shelf and returns the
// remaining gold total. If the player tries to store more money then they
// have, stores all their money. In the database the gold is stored in an
// int64, so the overflow is improbable.
//
// If DB operation fails or the player has no book, returns `current_money` and
// doesn't remove the book.
int32_t MoneyToSavingsBook(const CCharacter& chr, ServerIDType server_id, std::vector<CItem>& inventory, int32_t current_money, int32_t amount);

// Take up to `amount` of gold on the shelf and give to the player.
//
// Checks if a "savings book" (a magic book of fire missile) is present. If it
// is, takes up to `amount` of gold and adds it to the player's gold total. If
// the player tries to take more gold then there's stored on the shelf, give
// all gold from the shelf. If taking the gold would give player more than the
// max allowed gold (int32 max = 2**31-1), only take up to the limit.
//
// If DB operation fails or the player has no book, returns `current_money` and
// doesn't remove the book.
int32_t MoneyFromSavingsBook(const CCharacter& chr, ServerIDType server_id, std::vector<CItem>& inventory, int32_t current_money, int32_t amount);

// Checks if given character can deposit items on the shelf.
// Giga-characters are not allowed to deposit.
bool CanDeposit(const CCharacter& chr);

// Checks if given character can deposit items on the shelf.
// Giga-characters are not allowed to withdraw.
bool CanWithdraw(const CCharacter& chr);

// Store given items and money on the shelf.
bool StoreOnShelf(const CCharacter& chr, ServerIDType server_id, std::vector<CItem> inventory, int32_t money);

using StoreOnShelfFunction = bool(const CCharacter& chr, ServerIDType server_id, std::vector<CItem> inventory, int32_t money);

// Implementation details. Aren't expected to be used outside of `shelf.cpp`.
namespace impl {
    enum Field {
        ITEMS = 1,
        MONEY = 2,
        BOTH = 3,
    };

    using SQLQueryFunction = std::function<bool(std::string)>;
    // Mocking SQL calls in `LoadShelf` is annoying, so let's stub out the whole function.
    using LoadShelfFunction = std::function<bool(const CCharacter&, int, Field, int32_t*, std::string*, int64_t*, bool&)>;

    bool IsSavingsBook(const CItem& item);
    std::vector<CItem>::const_iterator FindSavingsBook(const std::vector<CItem>& inventory);
    void MergeItemPiles(std::vector<CItem>& items, const std::vector<CItem>& add_items);
    bool LoadShelf(const CCharacter& chr, int shelf_number, Field field, int32_t* mutex, std::string* items_repr, int64_t* money, bool& shelf_exists);
    bool SaveShelf(const CCharacter& chr, int shelf_number, Field field, int32_t mutex, std::vector<CItem> items, int64_t money, bool shelf_exists, SQLQueryFunction sql_query);

    bool ItemsToSavingsBookImpl(const CCharacter& chr, ServerIDType server_id, std::vector<CItem>& inventory, LoadShelfFunction load_from_shelf, SQLQueryFunction sql_query);
    bool ItemsFromSavingsBookImpl(const CCharacter& chr, ServerIDType server_id, std::vector<CItem>& inventory, LoadShelfFunction load_from_shelf, SQLQueryFunction sql_query);

    int32_t MoneyToSavingsBookImpl(const CCharacter& chr, ServerIDType server_id, std::vector<CItem>& inventory, int32_t current_money, int32_t amount, LoadShelfFunction load_shelf, SQLQueryFunction sql_query);
    int32_t MoneyFromSavingsBookImpl(const CCharacter& chr, ServerIDType server_id, std::vector<CItem>& inventory, int32_t current_money, int32_t amount, LoadShelfFunction load_shelf, SQLQueryFunction sql_query);

    bool StoreOnShelfImpl(const CCharacter& chr, ServerIDType server_id, std::vector<CItem> inventory, int32_t money, LoadShelfFunction load_shelf, SQLQueryFunction sql_query);
} // namespace impl
} // namespace shelf
