#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>

#include "UnitTest++.h"

#include "../shelf.hpp"

#include "../CCharacter.hpp"
#include "../login.hpp"

namespace {

const CItem book{.Id=3587, .IsMagic=true, .Count=1, .Effects={{42, 1}}};
const CItem other_book{.Id=3587, .IsMagic=true, .Count=1, .Effects={{42, 2}}};
const CItem cuirass{.Id=6162, .IsMagic=true, .Count=1, .Effects={{2, 2}}};
const CItem helm{.Id=42503, .IsMagic=true, .Count=1, .Effects={{3, 2}, {10, 100}}};
const CItem staff{.Id=53517, .IsMagic=true, .Count=1, .Effects={{17, 50}}};
const CItem potions5{.Id=3649, .Count=5};
const CItem potions10{.Id=3649, .Count=10};
const CItem scrolls20{.Id=3621, .Count=20};

std::ostream& operator<<(std::ostream& out, const std::vector<CItem>& inventory) {
    for (const auto& item: inventory) {
        out << "\n{id=" << item.Id << ", magic=" << item.IsMagic;
        if (item.Count > 1) {
            out << ", count=" << item.Count;
        }

        if (item.Effects.size()) {
            out << ", effects=[";

            for (const auto& effect: item.Effects) {
                out << (int)effect.Id1 << ":" << (int)effect.Value1 << " ";
            }

            out << "]";
        }
        out << "}";
    }
    return out << "\n";
}

bool operator==(const std::vector<CItem>& a, const std::vector<CItem>& b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tie(a[i].Id, a[i].Count, a[i].IsMagic, a[i].Effects) != std::tie(b[i].Id, b[i].Count, b[i].IsMagic, b[i].Effects)) {
            return false;
        }
    }

    return true;
}

TEST(IsSavingsBook_Correct) {
    bool got = shelf::impl::IsSavingsBook(book);
    CHECK_EQUAL(true, got);
}

TEST(IsSavingsBook_OtherBook) {
    bool got = shelf::impl::IsSavingsBook(other_book);
    CHECK_EQUAL(false, got);
}

TEST(MergeItemPiles_StartEmpty) {
    std::vector<CItem> inventory;
    std::vector<CItem> add{cuirass, potions5};
    shelf::impl::MergeItemPiles(inventory, add);
    std::vector<CItem> want{cuirass, potions5};
    CHECK_EQUAL(want, inventory);
}

TEST(MergeItemPiles_AddEmpty) {
    std::vector<CItem> inventory{cuirass, potions5};
    std::vector<CItem> add;
    shelf::impl::MergeItemPiles(inventory, add);
    std::vector<CItem> want{cuirass, potions5};
    CHECK_EQUAL(want, inventory);
}

TEST(MergeItemPiles_Merge) {
    std::vector<CItem> inventory{cuirass, potions5, helm};
    std::vector<CItem> add{staff, potions5, cuirass, scrolls20};
    shelf::impl::MergeItemPiles(inventory, add);
    std::vector<CItem> want{cuirass, potions10, helm, staff, cuirass, scrolls20};
    CHECK_EQUAL(want, inventory);
}

TEST(MergeItemPiles_EmptyDressSlots) {
    std::vector<CItem> inventory{};
    std::vector<CItem> add{CItem{.Id=0, .Count=1}};
    shelf::impl::MergeItemPiles(inventory, add);
    std::vector<CItem> want{};
    CHECK_EQUAL(want, inventory);
}

struct FakeSQLQuery {
    std::unordered_map<std::string, bool> queries;

    // Stupid C++ doesn't let me take a pointer to a member function. I'm just gonna wrap everything in lambda.
    shelf::impl::SQLQueryFunction Bind() {
        return [this] (std::string query) -> bool {
            if (queries.count(query) == 0) {
                std::cerr << "Test failed: unexpected query: " << query << std::endl;
                CHECK(false);
                return false;
            }

            return queries[query];
        };
    }
};

struct FakeLoadFromShelf {
    int want_login_id;
    int want_shelf_number;

    int32_t fake_mutex;
    bool fake_result;
    std::vector<CItem> fake_items;
    int64_t fake_money;
    bool fake_shelf_exists;

    shelf::impl::LoadShelfFunction Bind() {
        return [this] (const CCharacter& chr, int shelf_number, shelf::impl::Field field, int32_t* mutex, std::string* items_repr, int64_t* money, bool& shelf_exists) -> bool {
            CHECK_EQUAL(want_login_id, chr.LoginID);
            CHECK_EQUAL(want_shelf_number, shelf_number);

            if (!fake_result) {
                return false;
            }

            *mutex = fake_mutex;

            if (field & shelf::impl::Field::ITEMS) {
                CItemList item_list{.Items=fake_items};
                *items_repr = Login_SerializeItems(item_list);
            }

            if (field & shelf::impl::Field::MONEY) {
                *money = fake_money;
            }

            shelf_exists = fake_shelf_exists;
            return true;
        };
    }
};

CCharacter FakeChar(int login_id, std::string nick="") {
    CCharacter chr;
    chr.LoginID = login_id;
    chr.Nick = nick;
    return chr;
}

TEST(ItemsToSavingsBook_NoBook) {
    FakeSQLQuery sql_query{.queries={
        {},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=EASY,
        .fake_result=true,
        .fake_shelf_exists=false,
    };

    std::vector<CItem> inventory{cuirass, potions5, other_book, helm};
    shelf::impl::ItemsToSavingsBookImpl(FakeChar(42), EASY, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions5, other_book, helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsToSavingsBook_Normal) {
    FakeSQLQuery sql_query{.queries={
        {"INSERT INTO shelf (login_id, server_id, cabinet, mutex, items) VALUES (42, 6, 0, 0, '[0,0,0,2];[6162,1,0,1,{2:2:0:0}];[3649,0,0,5]')", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_shelf_exists=false,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsToSavingsBookImpl(FakeChar(42), NIGHTMARE, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsToSavingsBook_WithExistingShelf) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, items = '[0,0,0,3];[3649,0,0,10];[53517,1,0,1,{17:50:0:0}];[6162,1,0,1,{2:2:0:0}]' WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_items={potions5, staff},
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{potions5, cuirass, book, helm};
    shelf::impl::ItemsToSavingsBookImpl(FakeChar(42), QUEST_T1, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsToSavingsBook_FailedQuerySelect) {
    FakeSQLQuery sql_query{.queries={
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=false,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsToSavingsBookImpl(FakeChar(42), QUEST_T2, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions5, book, helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsToSavingsBook_FailedQuerySave) {
    FakeSQLQuery sql_query{.queries={
        {"INSERT INTO shelf (login_id, server_id, cabinet, mutex, items) VALUES (42, 6, 0, 0, '[0,0,0,2];[6162,1,0,1,{2:2:0:0}];[3649,0,0,5]')", false},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_shelf_exists=false,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsToSavingsBookImpl(FakeChar(42), QUEST_T2, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions5, book, helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsToSavingsBook_Solo) {
    FakeSQLQuery sql_query{.queries={
        {"INSERT INTO shelf (login_id, server_id, cabinet, mutex, items) VALUES (42, 6, 0, 0, '[0,0,0,2];[6162,1,0,1,{2:2:0:0}];[3649,0,0,5]')", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_shelf_exists=false,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsToSavingsBookImpl(FakeChar(42, "@solo"), QUEST_T4, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsToSavingsBook_Giga) {
    FakeSQLQuery sql_query{.queries={}};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_shelf_exists=false,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsToSavingsBookImpl(FakeChar(42, "_giga"), QUEST_T4, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions5, book, helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsFromSavingsBook_NoBook) {
    FakeSQLQuery sql_query{.queries={
        {},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=EASY,
        .fake_result=true,
        .fake_items={staff, potions5},
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{cuirass, potions5, other_book, helm};
    shelf::impl::ItemsFromSavingsBookImpl(FakeChar(42), EASY, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions5, other_book, helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsFromSavingsBook_EmptyShelf) {
    FakeSQLQuery sql_query{.queries={
        {},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=EASY,
        .fake_result=true,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsFromSavingsBookImpl(FakeChar(42), EASY, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions5, book, helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsFromSavingsBook_Normal) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, items = '[0,0,0,0]' WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_items={staff, potions5},
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsFromSavingsBookImpl(FakeChar(42), NIGHTMARE, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions10, helm, staff};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsFromSavingsBook_WithoutExistingShelf) {
    FakeSQLQuery sql_query{.queries={
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_shelf_exists=false,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsFromSavingsBookImpl(FakeChar(42), QUEST_T1, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions5, book, helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsFromSavingsBook_FailedQuerySelect) {
    FakeSQLQuery sql_query{.queries={
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=false,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsFromSavingsBookImpl(FakeChar(42), QUEST_T2, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions5, book, helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsFromSavingsBook_FailedQuerySave) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, items = '[0,0,0,0]' WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", false},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_items={staff, potions5},
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsFromSavingsBookImpl(FakeChar(42), QUEST_T4, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions5, book, helm};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsFromSavingsBook_Solo) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, items = '[0,0,0,0]' WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_items={staff, potions5},
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsFromSavingsBookImpl(FakeChar(42, "@solo"), QUEST_T1, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions10, helm, staff};
    CHECK_EQUAL(want, inventory);
}

TEST(ItemsFromSavingsBook_Giga) {
    FakeSQLQuery sql_query{.queries={}};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_items={staff, potions5},
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{cuirass, potions5, book, helm};
    shelf::impl::ItemsFromSavingsBookImpl(FakeChar(42, "_giga"), QUEST_T4, inventory, load_from_shelf.Bind(), sql_query.Bind());
    std::vector<CItem> want{cuirass, potions5, book, helm};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyToSavingsBook_NewShelf) {
    FakeSQLQuery sql_query{.queries={
        {"INSERT INTO shelf (login_id, server_id, cabinet, mutex, money) VALUES (42, 6, 0, 0, 200)", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_shelf_exists=false,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyToSavingsBookImpl(FakeChar(42), QUEST_T1, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(800, got);
    std::vector<CItem> want{};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyToSavingsBook_EmptyShelf) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 200 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=0,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyToSavingsBookImpl(FakeChar(42), QUEST_T2, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(800, got);
    std::vector<CItem> want{};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyToSavingsBook_ExistingShelf) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 250 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=50,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyToSavingsBookImpl(FakeChar(42), QUEST_T3, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(800, got);
    std::vector<CItem> want{};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyToSavingsBook_OverCurrent) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 110 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=100,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyToSavingsBookImpl(FakeChar(42), QUEST_T3, inventory, 10, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(0, got);
    std::vector<CItem> want{};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyToSavingsBook_OverInt32) {
    int32_t billion = 1000000000;

    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 6000000000 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=5 * static_cast<int64_t>(billion),
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{helm, book};
    auto got = shelf::impl::MoneyToSavingsBookImpl(FakeChar(42), QUEST_T3, inventory, 2*billion, billion, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(billion, got);
    std::vector<CItem> want{helm};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyToSavingsBook_FailedToQuery) {
    FakeSQLQuery sql_query{.queries={
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIVAL,
        .fake_result=false,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyToSavingsBookImpl(FakeChar(42), NIVAL, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(1000, got);
    std::vector<CItem> want{book};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyToSavingsBook_FailedToSave) {
    FakeSQLQuery sql_query{.queries={
        {"INSERT INTO shelf (login_id, server_id, cabinet, mutex, money) VALUES (42, 2, 0, 0, 200)", false},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=KIDS,
        .fake_result=true,
        .fake_shelf_exists=false,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyToSavingsBookImpl(FakeChar(42), KIDS, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(1000, got);
    std::vector<CItem> want{book};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyToSavingsBook_Solo) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 250 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=50,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyToSavingsBookImpl(FakeChar(42, "@solo"), QUEST_T4, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(800, got);
    std::vector<CItem> want{};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyFromSavingsBook_NewShelf) {
    FakeSQLQuery sql_query{.queries={
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_shelf_exists=false,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyFromSavingsBookImpl(FakeChar(42), QUEST_T1, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(1000, got);
    std::vector<CItem> want{book};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyFromSavingsBook_EmptyShelf) {
    FakeSQLQuery sql_query{.queries={
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=0,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyFromSavingsBookImpl(FakeChar(42), QUEST_T2, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(1000, got);
    std::vector<CItem> want{book};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyFromSavingsBook_ExistingShelf) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 100 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=300,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyFromSavingsBookImpl(FakeChar(42), QUEST_T3, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(1200, got);
    std::vector<CItem> want{};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyFromSavingsBook_OverShelved) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 0 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=100,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyFromSavingsBookImpl(FakeChar(42), QUEST_T3, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(1100, got);
    std::vector<CItem> want{};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyFromSavingsBook_WithdrawALot) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 4000001000 WHERE login_id = 42 AND server_id = 4 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=MEDIUM,
        .fake_result=true,
        .fake_money=4000000000LL + std::numeric_limits<int32_t>::max(),
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyFromSavingsBookImpl(FakeChar(42), MEDIUM, inventory, 1000, std::numeric_limits<int32_t>::max(), load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(std::numeric_limits<int32_t>::max(), got);
    std::vector<CItem> want{};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyFromSavingsBook_OverInt32) {
    int32_t billion = 1000000000;

    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 3000000000 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=std::numeric_limits<int32_t>::max() + 1 * static_cast<int64_t>(billion),
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{helm, book};
    auto got = shelf::impl::MoneyFromSavingsBookImpl(FakeChar(42), QUEST_T3, inventory, 2*billion, billion, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(std::numeric_limits<int32_t>::max(), got);
    std::vector<CItem> want{helm};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyFromSavingsBook_FailedToQuery) {
    FakeSQLQuery sql_query{.queries={
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIVAL,
        .fake_result=false,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyFromSavingsBookImpl(FakeChar(42), NIVAL, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(1000, got);
    std::vector<CItem> want{book};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyFromSavingsBook_FailedToSave) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 300 WHERE login_id = 42 AND server_id = 2 AND cabinet = 0 AND mutex = 0", false},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=KIDS,
        .fake_result=true,
        .fake_money=500,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyFromSavingsBookImpl(FakeChar(42), KIDS, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(1000, got);
    std::vector<CItem> want{book};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyFromSavingsBook_Solo) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, money = 100 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=300,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyFromSavingsBookImpl(FakeChar(42, "@solo"), QUEST_T4, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(1200, got);
    std::vector<CItem> want{};
    CHECK_EQUAL(want, inventory);
}

TEST(MoneyFromSavingsBook_Giga) {
    FakeSQLQuery sql_query{.queries={}};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_money=300,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::MoneyFromSavingsBookImpl(FakeChar(42, "_giga"), QUEST_T4, inventory, 1000, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(1000, got);
    std::vector<CItem> want{book};
    CHECK_EQUAL(want, inventory);
}

TEST(StoreOnShelf_NoShelf) {
    FakeSQLQuery sql_query{.queries={
        {"INSERT INTO shelf (login_id, server_id, cabinet, mutex, items, money) VALUES (42, 2, 0, 0, '[0,0,0,1];[3587,1,0,1,{42:1:0:0}]', 200)", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=KIDS,
        .fake_result=true,
        .fake_shelf_exists=false,
    };

    std::vector<CItem> inventory{book};
    auto got = shelf::impl::StoreOnShelfImpl(FakeChar(42), KIDS, inventory, 200, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(true, got);
}

TEST(StoreOnShelf_FilledShelf) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, items = '[0,0,0,3];[53517,1,0,1,{17:50:0:0}];[3649,0,0,10];[42503,1,0,1,{3:2:0:0},{10:100:0:0}]', money = 200 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }};

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_items={staff, potions5},
        .fake_money=100,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{potions5, helm};
    auto got = shelf::impl::StoreOnShelfImpl(FakeChar(42), QUEST_T1, inventory, 100, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(true, got);
}

TEST(StoreOnShelf_Giga) {
    FakeSQLQuery sql_query{.queries={
        {"UPDATE shelf SET mutex = 1, items = '[0,0,0,3];[53517,1,0,1,{17:50:0:0}];[3649,0,0,10];[42503,1,0,1,{3:2:0:0},{10:100:0:0}]', money = 200 WHERE login_id = 42 AND server_id = 6 AND cabinet = 0 AND mutex = 0", true},
    }}; // Notice that cabinet is 0 for a legend character.

    FakeLoadFromShelf load_from_shelf{
        .want_login_id=42,
        .want_shelf_number=NIGHTMARE,
        .fake_result=true,
        .fake_items={staff, potions5},
        .fake_money=100,
        .fake_shelf_exists=true,
    };

    std::vector<CItem> inventory{potions5, helm};
    auto got = shelf::impl::StoreOnShelfImpl(FakeChar(42, "_giga"), QUEST_T1, inventory, 100, load_from_shelf.Bind(), sql_query.Bind());
    CHECK_EQUAL(true, got);
}

}
