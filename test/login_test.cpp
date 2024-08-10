#include <stdint.h>
#include <string>
#include <sstream>

#include "UnitTest++.h"

#include "../login.hpp"

namespace
{

const std::string empty_bag = "[0,0,0,0]";
const std::string empty_dress = "[0,0,40,12];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";
const std::string dress_with_staff = "[0,0,40,12];[53709,1,2,1,{2:3:0:0}];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";

// A couple structs to conveniently construct a character for tests.
struct Info {
    uint8_t main_skill;
    uint8_t picture;
    uint8_t sex;
    uint32_t deaths;
    uint32_t kills;
    std::string clan;
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

CCharacter FakeCharacter(const CharacterOpts& opts) {
    CCharacter chr;

    chr.MainSkill = opts.info.main_skill ? opts.info.main_skill : 1;
    chr.Picture = opts.info.picture;
    chr.Sex = opts.info.sex;
    chr.Deaths = opts.info.deaths;
    chr.MonstersKills = opts.info.kills;
    chr.Clan = opts.info.clan;

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
    A2_TEST_DIFF("Clan", got.Clan, want.Clan);

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

    unsigned int ascended = -1;
    UpdateCharacter(chr, 2, ascended);

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

    unsigned int ascended = -1;
    UpdateCharacter(chr, 2, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.deaths=10, .clan="30k_gold"},
            .stats={.body=14, .reaction=10, .mind=14, .spirit=14}, // All that was 15 is reduced to 14.
            .skills={.astral=1234},
            .items={.money=52, .spells=268385790, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]"}, // Money is untouched, treasure disappears.
        }
    );

    CHECK_CHARACTER(chr, want);
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

    unsigned int ascended = -1;
    UpdateCharacter(chr, 2, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.deaths=10, .clan="treasure"},
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
            .skills={.astral=40000},
            .items={.money=35000, .spells=268385790, .bag="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = -1;
    UpdateCharacter(chr, 2, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.deaths=0, .clan="hc_50k_exp"},
            .stats={.body=14, .reaction=10, .mind=14, .spirit=14}, // All that was 15 is reduced to 14.
            .skills={.astral=40000},
            .items={.money=35000, .spells=268385790, .bag="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn23_Success_Mage) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=3, .sex=64, .deaths=10},
            .stats={.body=15, .reaction=11, .mind=15, .spirit=15},
            .skills={.fire=1000, .water=2000, .air=3000, .earth=4000, .astral=5000},
            .items={.money=35000, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = -1;
    UpdateCharacter(chr, 2, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=3, .sex=64, .deaths=10},
            .stats={.body=15, .reaction=11, .mind=15, .spirit=15},
            .skills={.fire=500, .water=1000, .air=1, .earth=2000, .astral=1}, // Main skill and astral are set to 1, others halved.
            .items={.spells=16778240}, // Money, bag and dress are wiped, spells are reset.
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn45_Success_Warrior) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=3, .sex=0, .deaths=10},
            .stats={.body=29, .reaction=30, .mind=28, .spirit=27},
            .skills={.fire=1000, .water=2000, .air=3000, .earth=4000, .astral=5000},
            .items={.money=1567890, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = -1;
    UpdateCharacter(chr, 4, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=3, .sex=0, .deaths=10},
            .stats={.body=29, .reaction=30, .mind=28, .spirit=27},
            .skills={.fire=500, .water=1000, .air=1, .earth=2000, .astral=1250}, // Main skill is set to 1, shooting is divided by server number (4), others halved.
            .items={.dress="[0,0,0,1];[1000,0,0,1]"}, // Money and bag are wiped.
        }
    );
    CHECK_CHARACTER(chr, want);
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

    unsigned int ascended = -1;
    UpdateCharacter(chr, 6, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=2, .sex=64, .deaths=10},
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50},
            .skills={.fire=500, .water=1, .air=1500, .earth=2000, .astral=1}, // Main skill and astral are set to 1, others halved.
            .items={.spells=16777248}, // Money, bag and dress are wiped, spells are reset.
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

    unsigned int ascended = -1;
    UpdateCharacter(chr, 3, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.sex=128, .kills=5000, .clan="500k_exp"},
            .stats={.body=19, .reaction=19, .mind=19, .spirit=15},
            .skills={.astral=40000},
            .items={.money=350000, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]"},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn4_Failed_Amazon_NoKills) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.sex=128, .kills=1000},
            .stats={.body=30, .reaction=30, .mind=30, .spirit=25},
            .skills={.astral=6000000},
            .items={.money=350000, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]"},
        }
    );

    unsigned int ascended = -1;
    UpdateCharacter(chr, 4, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.sex=128, .kills=1000, .clan="1500_kills"},
            .stats={.body=29, .reaction=29, .mind=29, .spirit=25},
            .skills={.astral=6000000},
            .items={.money=350000, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]"},
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

    unsigned int ascended = -1;
    UpdateCharacter(chr, 5, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.sex=192, .kills=5000, .clan="30m_gold"},
            .stats={.body=39, .reaction=39, .mind=39, .spirit=35},
            .skills={.astral=40000000},
            .items={.money=350000, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]"},
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reborn2_Success_Witch) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=4, .sex=192, .kills=2000},
            .stats={.body=14, .reaction=12, .mind=15, .spirit=14},
            .skills={.fire=10000, .water=20000, .air=30000, .earth=40000, .astral=50000},
            .items={.money=100000, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = -1;
    UpdateCharacter(chr, 2, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info{.main_skill=4, .sex=192, .kills=0}, // Wipe kills.
            .stats={.body=14, .reaction=12, .mind=15, .spirit=14},
            .skills={.fire=1, .water=1, .air=1, .earth=1, .astral=1}, // Wipe all skills.
            .items={.money=0, .spells=16842752}, // Wipe money, bag and dress, reset spells.
        }
    );

    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Reclass_Success) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .sex=64, .deaths=10, .kills=4200, .clan="reclass"},
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50},
            .skills={.fire=35742359, .water=35742359, .air=35742359, .earth=35742359, .astral=35742359},
            .items={.money=50000000, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = -1;
    UpdateCharacter(chr, 7, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .picture=6, .sex=192, .deaths=10, .kills=0, .clan="reclass"}, // Sex and picture are changed, kills are reset.
            .stats={.body=1, .reaction=1, .mind=1, .spirit=1}, // All stats are set to 1.
            .skills={.fire=1, .water=1, .air=1, .earth=1, .astral=1}, // All skills are set to 1.
            .items={.spells=16777218}, // Everything is wiped, spells are reset.
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_Ascend_Success) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .picture=6, .sex=192, .deaths=10, .kills=4200, .clan="ascend"},
            .stats={.body=56, .reaction=76, .mind=76, .spirit=76},
            .skills={.fire=35742359, .water=35742359, .air=35742359, .earth=35742359, .astral=35742359},
            .items={.money=90000000, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = -1;
    UpdateCharacter(chr, 7, ascended);

    CHECK_EQUAL(ascended, (unsigned int)1);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .picture=15, .sex=64, .deaths=10, .kills=4200, .clan="ascend"}, // Sex and picture are changed.
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50}, // All stats are set to 50.
            .skills={.fire=1, .water=1, .air=1, .earth=1, .astral=1}, // All skills are set to 1.
            .items={.money=90000000, .spells=268385790, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]", .dress=dress_with_staff}, // Dress is reset to get a prize staff.
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_NoChanges_7) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .info={.main_skill=1, .sex=64, .deaths=10, .kills=4200},
            .stats={.body=51, .reaction=52, .mind=53, .spirit=54},
            .skills={.fire=1000, .water=2000, .air=3000, .earth=4000, .astral=5000},
            .items={.money=60000000, .spells=268385790},
        }
    );

    unsigned int ascended = -1;
    UpdateCharacter(chr, 7, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

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

TEST(UpdateCharacter_TreasureOn7_NoChanges) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50},
            .skills={.astral=1000},
            .items={.money=1337, .spells=268385790, .bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );

    unsigned int ascended = -1;
    UpdateCharacter(chr, 7, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=50, .spirit=50}, // Treasure on 7 does nothing.
            .skills={.astral=1000},
            .items={.money=1337, .spells=268385790, .bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]", .dress="[0,0,0,1];[1000,0,0,1]"},
        }
    );
    CHECK_CHARACTER(chr, want);
}

TEST(UpdateCharacter_TreasureOn8) {
    CCharacter chr = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=50, .spirit=76},
            .skills={.astral=1000},
            .items={.bag="[0,0,0,3];[1000,0,0,1];[3667,0,0,1];[2000,0,0,2]"},
        }
    );

    unsigned int ascended = -1;
    UpdateCharacter(chr, 8, ascended);

    CHECK_EQUAL(ascended, (unsigned int)0);

    CCharacter want = FakeCharacter(
        CharacterOpts{
            .stats={.body=50, .reaction=50, .mind=50, .spirit=77}, // Spirit is increased even over the limit.
            .skills={.astral=1000},
            .items={.bag="[0,0,0,2];[1000,0,0,1];[2000,0,0,2]"},
        }
    );
    CHECK_CHARACTER(chr, want);
}

}
