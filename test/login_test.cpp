#include <stdint.h>
#include <string>
#include <sstream>

#include "UnitTest++.h"

#include "../kill_stats.h"
#include "../login.hpp"
#include "../server_id.hpp"

namespace
{

const std::string empty_bag = "[0,0,0,0]";
const std::string empty_dress = "[0,0,40,12];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";

// A couple structs to conveniently construct a character for tests.
struct Info {
    uint8_t main_skill;
    uint8_t picture;
    uint8_t sex;
    uint32_t deaths;
    uint32_t kills;
    std::string nick;
    std::string clan;
    int login_id;
};

struct Stats {
    uint8_t body;
    uint8_t reaction;
    uint8_t mind;
    uint8_t spirit;
};

struct Skills {
    uint32_t fire;
    uint32_t water;
    uint32_t air;
    uint32_t earth;
    uint32_t astral;
};

struct Items {
    uint32_t money;
    uint32_t spells;
    std::string bag;
    std::string dress;
};

struct CharacterOpts {
    Info info;
    Stats stats;
    Skills skills;
    Items items;
};

bool store_on_shelf_called;
int shelf_login_id;
ServerIDType shelf_server_id;
std::string shelf_items;
int32_t shelf_money;

bool FakeStoreOnShelf(const CCharacter& chr, ServerIDType server_id, std::vector<CItem> inventory, int32_t money) {
    store_on_shelf_called = true;
    shelf_login_id = chr.LoginID;
    shelf_server_id = server_id;
    CItemList item_list{.Items=std::move(inventory)};
    shelf_items = Login_SerializeItems(item_list);
    shelf_money = money;
    return true;
}

CCharacter FakeCharacter(const CharacterOpts& opts) {
    CCharacter chr;

    chr.MainSkill = opts.info.main_skill ? opts.info.main_skill : 1;
    chr.Picture = opts.info.picture;
    chr.Sex = opts.info.sex;
    chr.Deaths = opts.info.deaths;
    chr.MonstersKills = opts.info.kills;
    chr.Nick = opts.info.nick;
    chr.Clan = opts.info.clan;
    chr.LoginID = opts.info.login_id;

    chr.Body = opts.stats.body;
    chr.Reaction = opts.stats.reaction;
    chr.Mind = opts.stats.mind;
    chr.Spirit = opts.stats.spirit;

    chr.ExpFireBlade = opts.skills.fire;
    chr.ExpWaterAxe = opts.skills.water;
    chr.ExpAirBludgeon = opts.skills.air;
    chr.ExpEarthPike = opts.skills.earth;
    chr.ExpAstralShooting = opts.skills.astral;

    chr.Money = opts.items.money;
    chr.Spells = opts.items.spells;
    chr.Bag = Login_UnserializeItems(!opts.items.bag.empty() ? opts.items.bag : empty_bag);
    chr.Dress = Login_UnserializeItems(!opts.items.dress.empty() ? opts.items.dress : empty_dress);

    return chr;
}

// Check that the character is the same as expected.
// This macro is inspired by UNITTEST_CHECK. It's a macro so that __LINE__ works correctly.
#define CHECK_CHARACTER(got, want)                                                                                                          \
   {                                                                                                                                        \
      std::string diff = CharacterDiff(got, want);                                                                                          \
      if (diff.length() > 1)                                                                                                                    \
         UnitTest::CurrentTest::Results()->OnTestFailure(UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__), diff.c_str()); \
   }

// Get the diff between two characters. Ihe output always starts with a newline.
std::string CharacterDiff(CCharacter got, CCharacter want) {
    std::stringstream result;

    result << "\n";

    #ifdef A2_TEST_DIFF
    #error Are you kidding me? A2_TEST_DIFF is defined, why?
    #endif // A2_TEST_DIFF

    #define A2_TEST_DIFF(NAME, GOT, WANT) if (GOT != WANT) { result << "  " << NAME << ": got " << GOT << ", want " << WANT << "\n"; }
    #define A2_TEST_DIFF_FIELD(FIELD) A2_TEST_DIFF(#FIELD, (int)got.FIELD, (int)want.FIELD)

    A2_TEST_DIFF_FIELD(MainSkill);
    A2_TEST_DIFF_FIELD(Picture);
    A2_TEST_DIFF_FIELD(Sex);
    A2_TEST_DIFF_FIELD(Deaths);
    A2_TEST_DIFF_FIELD(MonstersKills);
    A2_TEST_DIFF("Nick", got.Nick, want.Nick);
    A2_TEST_DIFF("Clan", got.Clan, want.Clan);
    A2_TEST_DIFF_FIELD(LoginID);

    A2_TEST_DIFF_FIELD(Body);
    A2_TEST_DIFF_FIELD(Reaction);
    A2_TEST_DIFF_FIELD(Mind);
    A2_TEST_DIFF_FIELD(Spirit);

    A2_TEST_DIFF_FIELD(ExpFireBlade);
    A2_TEST_DIFF_FIELD(ExpWaterAxe);
    A2_TEST_DIFF_FIELD(ExpAirBludgeon);
    A2_TEST_DIFF_FIELD(ExpEarthPike);
    A2_TEST_DIFF_FIELD(ExpAstralShooting);

    A2_TEST_DIFF_FIELD(Money);
    A2_TEST_DIFF_FIELD(Spells);
    A2_TEST_DIFF("Bag", Login_SerializeItems(got.Bag), Login_SerializeItems(want.Bag));
    A2_TEST_DIFF("Dress", Login_SerializeItems(got.Dress), Login_SerializeItems(want.Dress));

    #undef A2_TEST_DIFF
    #undef A2_TEST_DIFF_FIELD

    return result.str();
}

TEST(UpdateCharacter_NoChanges) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .stats={.body=12, .reaction=3, .mind=13, .spirit=12},
            .skills={.astral=1234},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, EASY, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .stats={.body=12, .reaction=3, .mind=13, .spirit=12},
            .skills={.astral=1234},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn23_Failed_NoMoney) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.deaths=10},
            .stats={.body=15, .reaction=10, .mind=15, .spirit=15},
            .skills={.astral=1234},
            .items={.money=52, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    store_on_shelf_called = false;
    UpdateCharacter(chr, EASY, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.deaths=10},
            .stats={.body=14, .reaction=10, .mind=14, .spirit=14}, // All that was 15 is reduced to 14.
            .skills={.astral=1234},
            .items={.money=5052, .spells=268385790, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]"}, // Money is untouched, treasure disappears.
        }
    );

    CHECK_CHARACTER(chr, want);

    CHECK_EQUAL(store_on_shelf_called, false);
}

TEST(UpdateCharacter_Reborn23_Failed_NoTreasure) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.deaths=10},
            .stats={.body=15, .reaction=10, .mind=15, .spirit=15},
            .skills={.astral=1234},
            .items={.money=35000, .spells=268385790, .bag="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, EASY, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.deaths=10},
            .stats={.body=14, .reaction=10, .mind=14, .spirit=14}, // All that was 15 is reduced to 14.
            .skills={.astral=1234},
            .items={.money=35000, .spells=268385790, .bag="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn23_Failed_HardCoreNoExp) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.deaths=0},
            .stats={.body=15, .reaction=10, .mind=15, .spirit=15},
            .skills={.astral=30000},
            .items={.money=35000, .spells=268385790, .bag="[0,0,0,2];[3667,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, EASY, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.deaths=0},
            .stats={.body=14, .reaction=10, .mind=14, .spirit=14}, // All that was 15 is reduced to 14.
            .skills={.astral=30000},
            .items={.money=40000, .spells=268385790, .bag="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn23_Success_Mage) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=3, .sex=64, .deaths=10, .login_id=42},
            .stats={.body=15, .reaction=11, .mind=15, .spirit=15},
            .skills={.fire=1000, .water=2000, .air=3000, .earth=4000, .astral=5000},
            .items={.money=35000, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,2];[1000,0,0,1];[0,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    store_on_shelf_called = false;
    UpdateCharacter(chr, EASY, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=3, .picture=64, .sex=64, .deaths=10, .login_id=42},
            .stats={.body=15, .reaction=11, .mind=15, .spirit=15},
            .skills={.fire=500, .water=1000, .air=0, .earth=2000, .astral=0}, // Main skill and astral are cleared, others halved.
            .items={.spells=16778240}, // Money, bag and dress are wiped, spells are reset.
        }
    );
    CHECK_CHARACTER(chr, want);

    CHECK_EQUAL(true, store_on_shelf_called);
    CHECK_EQUAL(42, shelf_login_id);
    CHECK_EQUAL(EASY, shelf_server_id);
    CHECK_EQUAL("[0,0,0,4];[1000,0,0,1];[2000,0,0,2];[1000,0,0,1];[0,0,0,1]", shelf_items);
    CHECK_EQUAL(10000, shelf_money); // 35000 + 5000 (treasure) - 30000 (reborn price)
}

TEST(UpdateCharacter_Reborn45_Success_Warrior) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=3, .sex=0, .deaths=10, .login_id=42},
            .stats={.body=29, .reaction=30, .mind=28, .spirit=27},
            .skills={.fire=1000, .water=2000, .air=3000, .earth=4000, .astral=5000},
            .items={.money=1500003, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    store_on_shelf_called = false;
    UpdateCharacter(chr, NIVAL, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=3, .sex=0, .deaths=10, .login_id=42},
            .stats={.body=29, .reaction=30, .mind=28, .spirit=27},
            .skills={.fire=500, .water=1000, .air=0, .earth=2000, .astral=1250}, // Main skill is cleared, shooting is divided by server number (3), others halved.
            .items={.dress="[0,0,0,1];[1000,0,0,1]"}, // Money and bag are wiped.
        }
    );
    CHECK_CHARACTER(chr, want);

    CHECK_EQUAL(true, store_on_shelf_called);
    CHECK_EQUAL(42, shelf_login_id);
    CHECK_EQUAL(NIVAL, shelf_server_id);
    CHECK_EQUAL("[0,0,0,2];[1000,0,0,1];[2000,0,0,2]", shelf_items);
    CHECK_EQUAL(100003, shelf_money); // +100000 from treasure
}

TEST(UpdateCharacter_Reborn67_Success) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=2, .sex=64, .deaths=10},
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50},
            .skills={.fire=1000, .water=2000, .air=3000, .earth=4000, .astral=5000},
            .items={.money=50000000, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, HARD, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=2, .sex=64, .deaths=10},
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50},
            .skills={.fire=500, .water=0, .air=1500, .earth=2000, .astral=0}, // Main skill and astral are cleared, others halved.
            .items={.spells=16777248}, // Money, bag and dress are wiped, spells are reset.
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_RebornToNightmare_Stats) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=2, .sex=64, .deaths=10},
            .stats={.body=50, .reaction=50, .mind=49, .spirit=50}, // Mind is not 50, so it's not a reborn attempt.
            .skills={.fire=1000, .water=2000, .air=3000, .earth=4000, .astral=5000},
            .items={.money=50000000, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, HARD, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=2, .sex=64, .deaths=10},
            .stats={.body=50, .reaction=50, .mind=49, .spirit=50},
            .skills={.fire=1000, .water=2000, .air=3000, .earth=4000, .astral=5000},
            .items={.money=51000000, .spells=268385790, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn3_Failed_Amazon_NoExp) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.sex=128, .kills=5000},
            .stats={.body=20, .reaction=20, .mind=20, .spirit=15},
            .skills={.astral=40000},
            .items={.money=350000, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, KIDS, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.sex=128, .kills=5000},
            .stats={.body=19, .reaction=19, .mind=19, .spirit=15},
            .skills={.astral=40000},
            .items={.money=380000, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]"},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn5_Failed_Witch_NoGold) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.sex=192, .kills=5000},
            .stats={.body=40, .reaction=40, .mind=40, .spirit=35},
            .skills={.astral=40000000},
            .items={.money=350000, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, MEDIUM, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.sex=192, .kills=5000},
            .stats={.body=39, .reaction=39, .mind=39, .spirit=35},
            .skills={.astral=15000000}, // Reduce skills to the ceiling.
            .items={.money=850000, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]"},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn45_Failed_Hell_NoExp) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.deaths=10, .nick="3hell"},
            .stats={.body=40, .reaction=40, .mind=40, .spirit=40},
            .skills={.astral=1000}, // Enough for a regular character, not enough for hell character.
            .items={.money=100000000, .bag="[0,0,0,1];[3667,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, MEDIUM, FakeStoreOnShelf, &ascended, &points);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.deaths=10, .nick="3hell"},
            .stats={.body=39, .reaction=39, .mind=39, .spirit=39},
            .skills={.astral=1000},
            .items={.money=100500000, .bag="[0,0,0,0]"},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn45_Failed_Hell_NoMobs) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.deaths=10, .nick="3hell"},
            .stats={.body=40, .reaction=40, .mind=40, .spirit=40},
            .skills={.astral=22000000},
            .items={.money=100000000, .bag="[0,0,0,1];[3667,0,0,1]"},
        }
    );

    KillStats kill_stats;
    kill_stats.by_server_id.fill(3); // Pretend we've killed 3 mobs of each type.
    bool ok = kill_stats.Marshal(chr.Section55555555);
    CHECK_EQUAL(true, ok);

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, MEDIUM, FakeStoreOnShelf, &ascended, &points);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.deaths=10, .nick="3hell"},
            .stats={.body=39, .reaction=39, .mind=39, .spirit=39},
            .skills={.astral=15000000}, // Cut the skills
            .items={.money=100500000, .bag="[0,0,0,0]"},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn23_Success_Witch) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=4, .picture=15, .sex=192, .kills=2000},
            .stats={.body=14, .reaction=12, .mind=15, .spirit=14},
            .skills={.fire=10000, .water=20000, .air=30000, .earth=40000, .astral=50000},
            .items={.money=100000, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, EASY, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);
    CHECK_EQUAL(points, (unsigned int)1);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=4, .picture=15, .sex=192, .kills=2000}, // Doesn't get zombie picture.
            .stats={.body=14, .reaction=12, .mind=15, .spirit=14},
            .skills={}, // Wipe all skills.
            .items={.money=0, .spells=16842752}, // Wipe money, bag and dress, reset spells.
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn23_Success_Warrior) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=4, .sex=0, .kills=2000},
            .stats={.body=14, .reaction=12, .mind=15, .spirit=14},
            .skills={.fire=10000, .water=20000, .air=30000, .earth=40000, .astral=50000},
            .items={.money=100000, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, EASY, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);
    CHECK_EQUAL(points, (unsigned int)1);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=4, .picture=64, .sex=0, .kills=2000}, // Get zombie picture.
            .stats={.body=14, .reaction=12, .mind=15, .spirit=14},
            .skills={.fire=5000, .water=10000, .air=15000, .earth=0, .astral=25000}, // Main skill is cleared, others halved.
            .items={.money=0, .dress="[0,0,0,1];[1000,0,0,1]"}, // Wipe money and bag.
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn23_Failure_Solo_Treasures) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=4, .kills=2000, .nick="@ironman"},
            .stats={.body=14, .reaction=12, .mind=15, .spirit=14},
            .skills={.fire=10000, .water=20000, .air=30000, .earth=40000, .astral=50000},
            .items={.money=100000, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, EASY, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);
    CHECK_EQUAL(points, (unsigned int)1);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=4, .kills=2000, .nick="@ironman"},
            .stats={.body=14, .reaction=12, .mind=14, .spirit=14}, // Reduce mind.
            .skills={.fire=10000, .water=20000, .air=30000, .earth=40000, .astral=50000},
            .items={.money=105000, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"}, // Remove treasure.
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reclass_Success) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .sex=64, .deaths=10, .kills=4200, .clan="reclass", .login_id=42},
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50},
            .skills={.fire=35742359, .water=35742359, .air=35742359, .earth=35742359, .astral=35742359},
            .items={.money=500000000, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    store_on_shelf_called = false;

    UpdateCharacter(chr, NIGHTMARE, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);
    CHECK_EQUAL(points, (unsigned int)1);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .picture=6, .sex=192, .deaths=10, .kills=4200, .clan="reclass", .login_id=42}, // Sex and picture are changed.
            .stats={.body=1, .reaction=1, .mind=1, .spirit=1}, // All stats are set to 1.
            .skills={}, // All skills are cleared.
            .items={.spells=16777218}, // Everything is wiped, spells are reset.
        }
    );
    CHECK_CHARACTER(chr, want);

    CHECK_EQUAL(true, store_on_shelf_called);
    CHECK_EQUAL(42, shelf_login_id);
    CHECK_EQUAL(NIGHTMARE, shelf_server_id);
    CHECK_EQUAL("[0,0,0,3];[1000,0,0,1];[2000,0,0,2];[1000,0,0,1]", shelf_items);
    CHECK_EQUAL(203000000, shelf_money); // Had 500 mil, paid 300 mil for reclass, got 3 mil for treasures.
}

TEST(UpdateCharacter_Ascend_Witch_Success) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .picture=6, .sex=192, .deaths=10, .kills=4200, .clan="ascend"},
            .stats={.body=56, .reaction=76, .mind=76, .spirit=76},
            .skills={.fire=35742359, .water=35742359, .air=35742359, .earth=35742359, .astral=35742359},
            .items={.money=2147483647, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, NIGHTMARE, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)1);
    CHECK_EQUAL(points, (unsigned int)1);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .picture=15, .sex=64, .deaths=10, .kills=4200, .clan="ascend"}, // Sex and picture are changed.
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50}, // All stats are set to 50.
            .skills={}, // All skills are cleared.
            .items={.money=483647, .spells=268385790, .bag="[0,0,0,3];[53709,0,2,1];[1000,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"}, // Inventory has the prize.
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Ascend_Amazon_Success) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .picture=6, .sex=128, .deaths=10, .kills=4200, .clan="ascend"},
            .stats={.body=56, .reaction=76, .mind=76, .spirit=76},
            .skills={.fire=35742359, .water=35742359, .air=35742359, .earth=35742359, .astral=35742359},
            .items={.money=2147000001, .spells=0, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, NIGHTMARE, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)1);
    CHECK_EQUAL(points, (unsigned int)1);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .picture=32, .sex=0, .deaths=10, .kills=4200, .clan="ascend"}, // Sex and picture are changed.
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50}, // All stats are set to 50.
            .skills={}, // All skills are cleared.
            .items={.money=483647, .spells=0, .bag="[0,0,0,3];[18118,1,2,1,{2:3:0:0},{19:2:0:0},{12:250:0:0}];[1000,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"}, // Inventory has the prize.
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_NoChanges_Nightmare) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .sex=64, .deaths=10, .kills=4200},
            .stats={.body=51, .reaction=52, .mind=53, .spirit=54},
            .skills={.fire=1000, .water=2000, .air=3000, .earth=4000, .astral=5000},
            .items={.money=60000000, .spells=268385790},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, NIGHTMARE, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);
    CHECK_EQUAL(points, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .sex=64, .deaths=10, .kills=4200},
            .stats={.body=51, .reaction=52, .mind=53, .spirit=54},
            .skills={.fire=1000, .water=2000, .air=3000, .earth=4000, .astral=5000},
            .items={.money=60000000, .spells=268385790},
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_TreasureOnNightmare) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50},
            .skills={.astral=1000},
            .items={.money=1337, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,2];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, NIGHTMARE, FakeStoreOnShelf, &ascended, &points);
    CHECK_EQUAL(points, (unsigned int)2);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .stats={.body=51, .reaction=50, .mind=50, .spirit=50}, // Treasure on 7 adds body. Two treasures --- one guaranteed body.
            .skills={.astral=1000},
            .items={.money=3001337, .spells=268385790, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_TreasureOnQuestT1) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50},
            .skills={.astral=1000},
            .items={.bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,2];[2000,0,0,2]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, QUEST_T1, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);
    CHECK_EQUAL(points, (unsigned int)2);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=55, .spirit=50}, // Mind is increased, 2+3.
            .skills={.astral=1000},
            .items={.money=5000000, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]"},
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_TreasureLimitOnQuestT1) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=69, .spirit=50},
            .skills={.astral=1000},
            .items={.bag="[0,0,0,1];[3667,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, QUEST_T1, FakeStoreOnShelf, &ascended, &points);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=70, .spirit=50}, // Mind is increased up to 70.
            .skills={.astral=1000},
            .items={.money=5000000, .bag="[0,0,0,0]"},
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_TreasureBonusOnQuestT2) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50},
            .skills={.astral=1000},
            .items={.bag="[0,0,0,1];[3667,0,0,3]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, QUEST_T2, FakeStoreOnShelf, &ascended, &points);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=50, .spirit=54}, // Spirit += 3 + 1
            .skills={.astral=1000},
            .items={.money=7000000, .bag="[0,0,0,0]"},
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_TreasureOnQuestT4) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=72, .mind=73, .spirit=74},
            .skills={.astral=1000},
            .items={.money=2147483647, .bag="[0,0,0,1];[3667,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;

    // First, mind is increased.
    UpdateCharacter(chr, QUEST_T4, FakeStoreOnShelf, &ascended, &points);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=72, .mind=75, .spirit=74},
            .skills={.astral=1000},
            .items={.money=2147483647, .bag="[0,0,0,0]"},
        }
    );
    CHECK_CHARACTER(chr, want);

    // Mind is increased by 1 to the limit of 76, one point goes to spirit.
    chr.Bag.Items.push_back(CItem{.Id=3667, .Count=1});
    UpdateCharacter(chr, QUEST_T4, FakeStoreOnShelf, &ascended, &points);

    want = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=72, .mind=76, .spirit=75},
            .skills={.astral=1000},
            .items={.money=2147483647, .bag="[0,0,0,0]"},
        }
    );
    CHECK_CHARACTER(chr, want);

    // Spirit is increased by 1 to the limit of 76, one point goes to reaction.
    chr.Bag.Items.push_back(CItem{.Id=3667, .Count=1});
    UpdateCharacter(chr, QUEST_T4, FakeStoreOnShelf, &ascended, &points);

    want = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=73, .mind=76, .spirit=76},
            .skills={.astral=1000},
            .items={.money=2147483647, .bag="[0,0,0,0]"},
        }
    );
    CHECK_CHARACTER(chr, want);

    // Two points to reaction.
    chr.Bag.Items.push_back(CItem{.Id=3667, .Count=1});
    UpdateCharacter(chr, QUEST_T4, FakeStoreOnShelf, &ascended, &points);

    want = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=75, .mind=76, .spirit=76},
            .skills={.astral=1000},
            .items={.money=2147483647, .bag="[0,0,0,0]"},
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Circle) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=2, .sex=64, .deaths=10, .nick="@acharacter", .clan="miss_hell", .login_id=100},
            .stats={.body=55, .reaction=76, .mind=76, .spirit=76},
            .skills={.astral=200000000},
            .items={.money=1234567890, .spells=268385790, .bag="[0,0,0,1];[1000,0,0,1]", .dress="[0,0,0,2];[1000,0,0,1];[0,0,0,1]"},
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, NIGHTMARE, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=2, .picture=6, .sex=192, .deaths=10, .nick="@1acharacte", .clan="miss_hell", .login_id=100},
            .stats={.body=1, .reaction=1, .mind=1, .spirit=1},
            .items={.money=0, .spells=16777248, .bag="[0,0,0,0]"},
        }
    );

    CHECK_CHARACTER(chr, want);

    CHECK_EQUAL(true, store_on_shelf_called);
    CHECK_EQUAL(100, shelf_login_id);
    CHECK_EQUAL(NIGHTMARE, shelf_server_id);
    CHECK_EQUAL("[0,0,0,3];[1000,0,0,1];[1000,0,0,1];[0,0,0,1]", shelf_items);
    CHECK_EQUAL(234567890, shelf_money); // 1 billion paid for circle.
}

TEST(UpdateCharacter_Circle_Fail_NoMobs) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=2, .sex=64, .deaths=10, .nick="@acharacter", .clan="miss_hell", .login_id=100},
            .stats={.body=55, .reaction=76, .mind=76, .spirit=76},
            .skills={.astral=200000000},
            .items={.money=1234567890, .spells=268385790, .bag="[0,0,0,1];[1000,0,0,1]", .dress="[0,0,0,2];[1000,0,0,1];[0,0,0,1]"},
        }
    );

    KillStats kill_stats;
    kill_stats.by_server_id.fill(4); // Pretend we've killed 4 mobs of each type.
    bool ok = kill_stats.Marshal(chr.Section55555555);
    CHECK_EQUAL(true, ok);

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, NIGHTMARE, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=2, .sex=64, .deaths=10, .nick="@acharacter", .clan="miss_hell", .login_id=100},
            .stats={.body=55, .reaction=76, .mind=76, .spirit=76},
            .skills={.astral=200000000},
            .items={.money=1234567890, .spells=268385790, .bag="[0,0,0,1];[1000,0,0,1]", .dress="[0,0,0,2];[1000,0,0,1];[0,0,0,1]"},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_TreasureMoneyOverflow) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50},
            .items={.money=2147400000, .bag="[0,0,0,1];[3667,0,0,2]"}, // Two treasures to have guaranteed +1 body.
        }
    );

    unsigned int ascended = 0;
    unsigned int points = 0;
    UpdateCharacter(chr, NIGHTMARE, FakeStoreOnShelf, &ascended, &points);

    CHECK_EQUAL(ascended, (unsigned int)0);
    CHECK_EQUAL(points, (unsigned int)2);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .stats={.body=51, .reaction=50, .mind=50, .spirit=50},
            .items={.money=2147483647, .bag="[0,0,0,0]"},
        }
    );
    CHECK_CHARACTER(chr, want);
}

}
