#include "update_character.h"

#include "circle.h"
#include "constants.h"
#include "kill_stats.h"
#include "shelf.hpp"
#include "sql.hpp"
#include "thresholds.h"
#include "utils.hpp"

namespace update_character {

bool HasKillsForReborn(CCharacter& chr, ServerIDType server_id) {
    auto* mobs = thresholds::thresholds.Mobs("reborn.mobs", chr, server_id);
    if (!mobs) {
        return true;
    }

    KillStats stats;
    if (!stats.Unmarshal(chr.Section55555555)) {
        return true; // Should not happen. Return `true` to simplify tests.
    }

    for (auto it = mobs->begin(); it != mobs->end(); ++it) {
        if (stats.by_server_id[it->first] < it->second) {
            Printf(LOG_Info, "[reborn-kills] Player %s has %d kills of %d, want %d\n", chr.GetFullName().c_str(), stats.by_server_id[it->first], it->first, it->second);
            return false;
        }
    }

    return true;
}

void ClearMonsterKills(CCharacter& chr) {
    KillStats kill_stats;
    kill_stats.by_server_id.fill(0);

    chr.Section55555555.Reset();
    if (!kill_stats.Marshal(chr.Section55555555)) {
        Printf(LOG_Error, "[reborn-kills] failed to marshal zero kill stats\n");
    }
}

uint8_t TreasureOnNightmare(CCharacter& chr, bool coinflip) {
    uint8_t add_body_min = 0;
    uint8_t add_body_max = 1;

    // Witch and amazon increase Body a bit faster as they start from 1 and 25.
    if (chr.IsFemale()) {
        if (chr.Body < 15) {
            add_body_min = 1; // At low body levels, at least +1 is guaranteed.
            add_body_max = 4;
        } else if (chr.Body < 25) {
            add_body_min = 1;
            add_body_max = 3;
        } else if (chr.Body < 35) {
            add_body_min = 1;
            add_body_max = 2;
        } else if (chr.Body < 45) {
            add_body_max = 2;
        }
    }

    return coinflip ? add_body_min : add_body_max;
}

void DrinkTreasure(CCharacter& chr, ServerIDType server_id, int treasures) {
    bool coinflip = std::rand() % 2;
    double circle_multiplier = circle::TreasureMultiplier(chr);
    uint8_t add = 0;
    uint8_t max_body = chr.IsFemale() ? 56 : 55;

    switch (server_id) {
    case NIGHTMARE: // 2 treasures per map
        for (int i = 0; i < treasures; ++i) {
            coinflip = !coinflip; // One treasure is random, two are guaranteed.

            add = TreasureOnNightmare(chr, coinflip);
            update_character::IncreaseUpTo(&chr.Body, static_cast<uint8_t>(add * circle_multiplier), max_body);
        }
        break;
    case QUEST_T1: // 1 treasure. 2x-3x more mind
        for (int i = 0; i < treasures; ++i) {
            coinflip = !coinflip; // One treasure is random, two are guaranteed. This is used in tests.

            if (coinflip) {
                update_character::IncreaseUpTo(&chr.Mind, static_cast<uint8_t>(3 * circle_multiplier), 70);
            } else {
                update_character::IncreaseUpTo(&chr.Mind, static_cast<uint8_t>(2 * circle_multiplier), 70);
            }
        }
        break;
    case QUEST_T2: // here we have 3 treasures per map
        add = treasures + (treasures == 3 ? 1 : 0); // With a bonus for all three treasures.
        update_character::IncreaseUpTo(&chr.Spirit, static_cast<uint8_t>(add * circle_multiplier), 70);
        break;
    case QUEST_T3: // here we have 3 treasures per map
        add = treasures + (treasures == 3 ? 1 : 0); // With a bonus for all three treasures.
        update_character::IncreaseUpTo(&chr.Reaction, static_cast<uint8_t>(add * circle_multiplier), 70);
        break;
    case QUEST_T4: // 1 treasure
        add = static_cast<uint8_t>(treasures * 2 * circle_multiplier);
        for (int j = 0; j < add; ++j) {
            update_character::IncreaseUpTo(&chr.Mind, 1, 76)
                || update_character::IncreaseUpTo(&chr.Spirit, 1, 76)
                || update_character::IncreaseUpTo(&chr.Reaction, 1, 76);
        }
        break;
    }
}

bool IncreaseUpTo(uint8_t* value, uint8_t increment, uint8_t limit) {
    if (*value >= limit) {
        // Already over the limit, leave as is.
        return false;
    }

    *value += increment;

    if (*value >= limit) {
        // Can get only up to the limit, not more.
        *value = limit;
    }

    return true;
}

const char* NightmareCheckpoint(const CCharacter& chr, uint8_t stats_before[4]) {
    uint8_t max_body = chr.IsFemale() ? 56 : 55;
    if (chr.Body == max_body && stats_before[0] < chr.Body) {
        return "nightmare";
    }

    if (chr.Mind == 70 && stats_before[2] < 70) {
        return "quest_1";
    }

    if (chr.Spirit == 70 && stats_before[3] < 70) {
        return "quest_2";
    }

    if (chr.Reaction == 70 && stats_before[1] < 70) {
        return "quest_3";
    }

    if (chr.Reaction + chr.Mind + chr.Spirit == 3*76 && stats_before[1] + stats_before[2] + stats_before[3] < 3*76) {
        return "quest_4";
    }

    return nullptr;
}

void SaveTreasurePoints(int character_id, ServerIDType server_id, unsigned int points) {
    if (points == 0) {
        return;
    }

    auto read = SimpleSQL{Format("SELECT treasure_points FROM treasure WHERE server_id = %d AND character_id = %d;", server_id, character_id)};
    if (!read) {
        return;
    }

    if (SQL_NumRows(read.result) > 0) {
        SimpleSQL{Format("UPDATE treasure SET treasure_points = treasure_points + %d WHERE server_id = %d AND character_id = %d;", points, server_id, character_id)};
    } else {
        SimpleSQL{Format("INSERT INTO treasure (server_id, character_id, treasure_points) VALUES (%d, %d, %d);", server_id, character_id, points)};
    }
}

void VisitShelf(CCharacter& chr, ServerIDType server_id) {
    if (chr.Clan == "d" || chr.Clan == "deposit") { // Deposit items.
        shelf::ItemsToSavingsBook(chr, server_id, chr.Bag.Items);
    } else if (chr.Clan == "w" || chr.Clan == "withdraw") { // Withdraw items.
        shelf::ItemsFromSavingsBook(chr, server_id, chr.Bag.Items);
    } else if (chr.Clan.find("dg") == 0) { // Deposit gold.
        if (chr.Clan == "dg") { // Default: 90% of total gold.
            chr.Money = shelf::MoneyToSavingsBook(chr, server_id, chr.Bag.Items, chr.Money, chr.Money - (chr.Money / 10));
        } else {
            std::string percentage_str = chr.Clan.substr(2);
            if (CheckInt(percentage_str)) { // Deposit given percentage of total gold.
                int percentage = StrToInt(percentage_str);
                if (0 < percentage && percentage <= 100) {
                    double ratio = percentage / 100.0;
                    chr.Money = shelf::MoneyToSavingsBook(chr, server_id, chr.Bag.Items, chr.Money, static_cast<int32_t>(chr.Money * ratio));
                }
            }
        }
    } else if (chr.Clan == "wg") { // Withdraw gold.
        chr.Money = shelf::MoneyFromSavingsBook(chr, server_id, chr.Bag.Items, chr.Money, std::numeric_limits<int32_t>::max());
    }
}

void WipeSpells(CCharacter& chr) {
    switch (chr.MainSkill) {
        case 1: chr.Spells = 16777218; break; // fire
        case 2: chr.Spells = 16777248; break; // water
        case 3: chr.Spells = 16778240; break; // air
        case 4: chr.Spells = 16842752; break; // earth
    }
}

void StoreOnShelf(ServerIDType server_id, CCharacter& chr, bool store_dress, shelf::StoreOnShelfFunction store_on_shelf) {
    std::vector<CItem> items = std::move(chr.Bag.Items);
    chr.Bag.Items.clear();

    if (store_dress) {
        // Mage and witch lose the dress in addition to the inventory.
        items.insert(items.end(), chr.Dress.Items.begin(), chr.Dress.Items.end());

        CItem nothing{.Id=0, .IsMagic=0, .Price=0, .Count=1};
        chr.Dress = CItemList{
            .UnknownValue2=40, // Why? But I ask again, why?
            .Items={nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing},
        };
    }

    if (!store_on_shelf(chr, server_id, std::move(items), chr.Money)) {
        Printf(LOG_Warning, "Failed to store items on shelf upon reborn for character %s at server %d\n", chr.GetFullName(), server_id);
    }

    chr.Money = 0;
}

bool IsAttemptingReborn(const CCharacter& chr, ServerIDType server_id) {
    uint32_t mind = thresholds::thresholds.Value("reborn.stats.mind", chr, server_id);
    if (mind != 0 && chr.Mind < mind) {
        return false;
    }

    uint32_t reaction = thresholds::thresholds.Value("reborn.stats.reaction", chr, server_id);
    if (reaction != 0 && chr.Reaction < reaction) {
        return false;
    }

    uint32_t spirit = thresholds::thresholds.Value("reborn.stats.spirit", chr, server_id);
    if (spirit != 0 && chr.Spirit < spirit) {
        return false;
    }

    return mind > 0 || reaction > 0 || spirit > 0;
}

bool MeetsRebornCriteria(CCharacter& chr, ServerIDType server_id, int have_treasures) {
    int need_treasures = (int)thresholds::thresholds.Value("reborn.treasures", chr, server_id);
    if (have_treasures < need_treasures) {
        return false;
    }

    uint32_t price = RebornPrice(chr, server_id);
    if (chr.Money < price) {
        return false;
    }

    uint32_t need_experience = thresholds::thresholds.Value("reborn.experience", chr, server_id);
    if (chr.TotalExperience() < need_experience) {
        return false;
    }

    return HasKillsForReborn(chr, server_id);
}

void FailReborn(CCharacter& chr, ServerIDType server_id) {
    uint8_t stat_ceiling = (uint8_t)thresholds::thresholds.Value("reborn.failure.stat_ceiling", chr, server_id);
    if (stat_ceiling == 0) {
        return;
    }

    // Make stats at most "max-1".
    chr.Body = std::min(chr.Body, stat_ceiling);
    chr.Reaction = std::min(chr.Reaction, stat_ceiling );
    chr.Mind = std::min(chr.Mind, stat_ceiling);
    chr.Spirit = std::min(chr.Spirit, stat_ceiling);
}

uint32_t RebornPrice(const CCharacter& chr, ServerIDType server_id) {
    return thresholds::thresholds.Value("reborn.money", chr, server_id);
}

int ConsumeTreasures(CCharacter& chr, ServerIDType server_id) {
    const unsigned long treasure_item_id = 3667;  // "Quest Treasure".
    int treasures = 0;

    for (const auto &item: chr.Bag.Items) {
        if (item.Id == treasure_item_id) {
            treasures += static_cast<int>(item.Count);
        }
    }

    if (treasures > 0) {
        // Remove all quest treasures from the player's bag.
        std::vector<CItem> new_items;
        new_items.reserve(chr.Bag.Items.size());

        for (const auto& item: chr.Bag.Items) {
            if (item.Id != treasure_item_id) {
                new_items.push_back(item);
            }
        }

        chr.Bag.Items = new_items;

        // Award player some gold for the treasure.
        uint64_t new_money = static_cast<uint64_t>(chr.Money);
        new_money += thresholds::thresholds.Value("treasure_award", chr, server_id);

        chr.Money = (uint32_t)std::min(new_money, (uint64_t)std::numeric_limits<int32_t>::max());
    }

    return treasures;
}

void PerformReborn(CCharacter& chr, ServerIDType server_id, shelf::StoreOnShelfFunction store_on_shelf) {
    // Save to shelf upon reborn. Note that this function empties the bag and money (and dress for mage/witch).
    chr.Money -= RebornPrice(chr, server_id);
    StoreOnShelf(server_id, chr, chr.IsWizard(), store_on_shelf);

    // Simple characters: male (no reclass/ascend), not on circles.
    if (!chr.IsFemale() && circle::Circle(chr) == 0) {
        // Wipe experience for the main skill
        switch (chr.MainSkill) {
            case 1: chr.ExpFireBlade = 0; break;
            case 2: chr.ExpWaterAxe = 0; break;
            case 3: chr.ExpAirBludgeon = 0; break;
            case 4: chr.ExpEarthPike = 0; break;
        }

        // Reduce all other skills in 2 times...
        if (chr.MainSkill != 1) { chr.ExpFireBlade /= 2; }
        if (chr.MainSkill != 2) { chr.ExpWaterAxe /= 2; }
        if (chr.MainSkill != 3) { chr.ExpAirBludgeon /= 2; }
        if (chr.MainSkill != 4) { chr.ExpEarthPike /= 2; }

        // Mage loses all spells but the basic arrow.
        if (chr.IsMage()) {
            WipeSpells(chr);
        }

        // astral/shooting skill
        if (chr.Deaths == 0) {
            chr.ExpAstralShooting /= 2; // (hardcore character only 2x times)
        } else if (chr.IsWarrior()) {
            chr.ExpAstralShooting /= static_cast<int>(server_id + 1); // WARR divide in srvid times
        } else if (chr.IsMage()) {
            chr.ExpAstralShooting = 0; // MAGE wipe astral skill
        }
    } else {
        // Females characters (amazon/witch) and characters on circles.
        update_character::ClearMonsterKills(chr);

        // wipe ALL exp
        chr.ExpFireBlade = chr.ExpWaterAxe = chr.ExpAirBludgeon = chr.ExpEarthPike = chr.ExpAstralShooting = 0;

        // Wizards lose all spells but the basic arrow.
        if (chr.IsWizard()) {
            WipeSpells(chr);
        }

        // reset BODY for ama/witch upon reborn when moving from HARD to NIGHTMARE
        if (server_id == HARD) {
            if (chr.IsAmazon()) {
                chr.Body = 25;
            } else if (chr.IsWitch()) {
                chr.Body = 1;
            }
        }
    }

    // Prevent preserving after reborn too high non-main skill.
    CutOffExperienceOnReborn(chr, server_id);

    // Upon leaving HARD, give the circle rewards.
    if (server_id == HARD) {
        int circle_number = circle::Circle(chr);

        if (circle_number > 0) {
            if (IsLegend(chr)) {
                // Legends get all rewards from the first circle till the current one.
                for (int i = 1; i <= circle_number; ++i) {
                    auto item = circle::Reward(chr.Sex, i);
                    Printf(LOG_Info, "[circle] Giving reward id=%d to legend character '%s'\n", item.Id, chr.GetFullName().c_str());
                    if (item.Id) {
                        chr.Bag.Items.push_back(std::move(item));
                    }
                }

                // Also should give the crown, but I'd need to pick `ascended` from DB. Can do it when we have ascended legends.
            } else {
                auto item = circle::Reward(chr.Sex, circle_number);
                Printf(LOG_Info, "[circle] Giving reward id=%d to character '%s'\n", item.Id, chr.GetFullName().c_str());
                if (item.Id) {
                    chr.Bag.Items.push_back(std::move(item));
                }
            }
        }
    }
}

void CutOffExperienceOnReborn(CCharacter& chr, ServerIDType server_id) {
    uint32_t limit = thresholds::thresholds.Value("reborn.experience_cutoff", chr, server_id);
    if (limit == 0) {
        return;
    }

    chr.ExpFireBlade = std::min(chr.ExpFireBlade, limit);
    chr.ExpWaterAxe = std::min(chr.ExpWaterAxe, limit);
    chr.ExpAirBludgeon = std::min(chr.ExpAirBludgeon, limit);
    chr.ExpEarthPike = std::min(chr.ExpEarthPike, limit);
    chr.ExpAstralShooting = std::min(chr.ExpAstralShooting, limit);
}

void ExperienceLimit(CCharacter& chr, ServerIDType server_id) {
    uint32_t limit_main = thresholds::thresholds.Value("experience_limit.main_skill", chr, server_id);
    if (limit_main == 0) {
        return;
    }
    uint32_t limit_secondary = thresholds::thresholds.Value("experience_limit.secondary", chr, server_id);
    if (limit_secondary == 0) {
        return;
    }

    chr.ExpFireBlade = std::min(chr.ExpFireBlade, chr.MainSkill == 1 ? limit_main : limit_secondary);
    chr.ExpWaterAxe = std::min(chr.ExpWaterAxe, chr.MainSkill == 2 ? limit_main : limit_secondary);
    chr.ExpAirBludgeon = std::min(chr.ExpAirBludgeon, chr.MainSkill == 3 ? limit_main : limit_secondary);
    chr.ExpEarthPike = std::min(chr.ExpEarthPike, chr.MainSkill == 4 ? limit_main : limit_secondary);
    chr.ExpAstralShooting = std::min(chr.ExpAstralShooting, limit_main);
}

bool ShouldReclass(const CCharacter& chr, ServerIDType server_id) {
    if (chr.IsFemale() || chr.Clan != "reclass" || circle::Circle(chr) != 0) {
        return false;
    }

    uint32_t need_money = thresholds::thresholds.Value("reclass.money", chr, server_id);
    if (chr.Money < need_money) {
        return false;
    }

    uint32_t need_experience = thresholds::thresholds.Value("reclass.experience", chr, server_id);
    if (chr.TotalExperience() < need_experience) {
        return false;
    }

    return true;
}

void PerformReclass(CCharacter& chr, ServerIDType server_id, shelf::StoreOnShelfFunction store_on_shelf) {
    ClearMonsterKills(chr);

    // Save to shelf upon reclass. Note that this function removes money, empties the bag and dress.
    chr.Money -= thresholds::thresholds.Value("reclass.money", chr, server_id);;
    StoreOnShelf(server_id, chr, true, store_on_shelf);

    if (chr.Deaths == 0) {
        chr.Money = 133; // HC: leave 133 gold to buy a bow
    }

    // stats
    chr.Body = chr.Reaction = chr.Mind = chr.Spirit = 1;
    // exp
    chr.ExpFireBlade = chr.ExpWaterAxe = chr.ExpAirBludgeon = chr.ExpEarthPike = chr.ExpAstralShooting = 0;

    // reclass: war/mage change class
    if (chr.IsWarrior()) { // warr become ama
        chr.Sex = sex::amazon;
        chr.Picture = 11; // and become human
    }
    else if (chr.IsMage()) { // mage becomes witch
        chr.Sex = sex::witch;
        chr.Picture = 6; // and become human

        WipeSpells(chr);
    }
}

bool ShouldAscend(const CCharacter& chr, ServerIDType server_id) {
    if (!chr.IsFemale() || chr.Clan != "ascend" || circle::Circle(chr) != 0) {
        return false;
    }

    uint32_t need_money = thresholds::thresholds.Value("ascend.money", chr, server_id);
    if (chr.Money < need_money) {
        return false;
    }

    uint32_t need_experience = thresholds::thresholds.Value("ascend.experience", chr, server_id);
    if (chr.TotalExperience() < need_experience) {
        return false;
    }

    if (chr.Body < thresholds::thresholds.Value("ascend.stats.body", chr, server_id)) {
        return false;
    }
    if (chr.Reaction < thresholds::thresholds.Value("ascend.stats.reaction", chr, server_id)) {
        return false;
    }
    if (chr.Mind < thresholds::thresholds.Value("ascend.stats.mind", chr, server_id)) {
        return false;
    }
    if (chr.Spirit < thresholds::thresholds.Value("ascend.stats.spirit", chr, server_id)) {
        return false;
    }

    return true;
}

void PerformAscend(CCharacter& chr, ServerIDType server_id, shelf::StoreOnShelfFunction store_on_shelf) {
    chr.Money -= thresholds::thresholds.Value("ascend.money", chr, server_id);

    // Loose some stats as price for ascend,
    // but still save some be able stay on #7;
    // otherwise (eg if we reset stats to 1)...
    // ...reborn will cause mage staff to dissapear
    chr.Body = 50;
    chr.Reaction = 50;
    chr.Mind = 50;
    chr.Spirit = 50;

    // pay with exp too
    chr.ExpFireBlade = chr.ExpWaterAxe = chr.ExpAirBludgeon = chr.ExpEarthPike = chr.ExpAstralShooting = 0;

    // We do not wipe inventory for ascension.
    // The prize item is inserted into the beginning of the inventory.
    if (chr.IsAmazon()) { // amazon becomes warrior and gets CROWN (Good Gold Helm) +3 body +2 scanRange +250 attack
        chr.Sex = sex::warrior;
        chr.Picture = 32;

        CItem crown{.Id=18118, .IsMagic=1, .Price=2, .Count=1, .Effects={
            {stats::body, 3},
            {stats::scan_range, 2},
            {stats::attack, 250},
        }};
        chr.Bag.Items.insert(chr.Bag.Items.begin(), crown);
    } else if (chr.IsWitch()) { // witch becomes mage and gets physical damage staff
        chr.Sex = sex::mage;
        chr.Picture = 15;

        CItem staff{.Id=53709, .IsMagic=0, .Price=2, .Count=1};
        chr.Bag.Items.insert(chr.Bag.Items.begin(), staff);
    }
}

void MaybeAllowFemale(const CCharacter& chr, ServerIDType server_id) {
    if (chr.Deaths <= 1 && server_id >= NIGHTMARE) {
        // HC warriors on NIGHTMARE need to max only main and secondary skill
        // to be able to reclass (110+110 is 72m.. which we will make to 77m)
        uint32_t exp_requirement = chr.IsWarrior() ? 77777777 : 177777777;
        if (chr.TotalExperience() >= exp_requirement) {
            int allowed = chr.Nick[0] == '_' ? 2 : chr.Nick[0] == '@' ? 1 : 0;
            SimpleSQL{Format("UPDATE logins SET allow_female = %d WHERE id = %d AND allow_female < %d;", allowed, chr.LoginID, allowed)};
            Printf(LOG_Info, "allow female: allowing player '%s' with login %d to create females of kind %d: %d rows affected\n", chr.GetFullName().c_str(), chr.LoginID, allowed, SQL_AffectedRows());
        }
    }
}

} // namespace update_character
