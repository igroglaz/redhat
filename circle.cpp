#include "circle.h"

#include <unordered_map>

#include "kill_stats.h"
#include "constants.h"
#include "thresholds.h"
#include "utils.hpp"


namespace circle {

int Circle(const CCharacter& chr) {
    char maybe_circle = chr.Nick[0];
    if (IsSolo(chr)) {
        maybe_circle = chr.Nick[1];
    }

    if ('1' <= maybe_circle && maybe_circle <= '9') {
        return maybe_circle - '0';
    }

    return 0;
}

bool Allowed(CCharacter& chr) {
    // Do we have more circles?
    if (Circle(chr) >= 8) {
        return false;
    }

    // Does the player want to circle?
    if (chr.Clan != "circle" && chr.Clan != "hell" && chr.Clan != "miss_hell") {
        return false;
    }

    // Have they maxed out stats? We don't check the body, as the player needs to clear out QUEST_T4 to get everything to 76.
    if (chr.Reaction < thresholds::thresholds.Value("hell.stats.reaction", chr, NIGHTMARE)) {
        Printf(LOG_Info, "[circle] '%s' failed to circle: reaction", chr.GetFullName().c_str());
        return false;
    }
    if (chr.Mind < thresholds::thresholds.Value("hell.stats.mind", chr, NIGHTMARE)) {
        Printf(LOG_Info, "[circle] '%s' failed to circle: mind", chr.GetFullName().c_str());
        return false;
    }
    if (chr.Spirit < thresholds::thresholds.Value("hell.stats.spirit", chr, NIGHTMARE)) {
        Printf(LOG_Info, "[circle] '%s' failed to circle: spirit", chr.GetFullName().c_str());
        return false;
    }

    // Are they experienced enough?
    uint32_t need_experience = thresholds::thresholds.Value("hell.experience", chr, NIGHTMARE);
    if (chr.TotalExperience() < need_experience) {
        Printf(LOG_Info, "[circle] '%s' failed to circle: stats", chr.GetFullName().c_str());
        return false;
    }

    // Do they have the money for the ticket?    
    uint32_t price = thresholds::thresholds.Value("hell.money", chr, NIGHTMARE);
    if (chr.Money < price) {
        Printf(LOG_Info, "[circle] '%s' failed to circle: money", chr.GetFullName().c_str());
        return false;
    }

    // Have they learned enough about the monsters?
    auto* mobs = thresholds::thresholds.Mobs("hell.mobs", chr, NIGHTMARE);
    if (mobs) {
        KillStats stats;
        if (!stats.Unmarshal(chr.Section55555555)) {
            return true; // Should not happen. Return `true` to simplify tests.
        }

        for (auto it = mobs->begin(); it != mobs->end(); ++it) {
            if (stats.by_server_id[it->first] < it->second) {
                Printf(LOG_Info, "[circle/kills] Player %s has %d kills of %d, want %d\n", chr.GetFullName().c_str(), stats.by_server_id[it->first], it->first, it->second);
                return false;
            }
        }
    }

    return true;
}

std::string Rename(const std::string& nick) {
    std::string prefix, suffix;

    char maybe_circle = nick[0];
    if (nick[0] == '_' || nick[0] == '@') {
        prefix = nick[0];
        suffix = nick.substr(1);

        maybe_circle = nick[1];
    } else {
        suffix = nick;
    }

    std::string result;

    if ('1' <= maybe_circle && maybe_circle <= '8') {
        result = prefix + char(maybe_circle + 1) + suffix.substr(1);
    } else {
        result = prefix + "1" + suffix;
    }

    if (result.length() > 11) {
        result.pop_back();
    }
    return result;
}

const std::vector<CEffect> fighter_clothes_effects{
    {stats::body, 2},
    {stats::attack, 200},
    {stats::speed, 1},
    {stats::protection_fire,     6},
    {stats::protection_water,    6},
    {stats::protection_air,      6},
    {stats::protection_earth,    6},
    {stats::protection_astral,   2},
};

const std::vector<CEffect> fighter_bijou_effects{
    {stats::body, 1},
    {stats::mind, 1},
    {stats::reaction, 1},
    {stats::spirit, 1},
    {stats::attack, 200},
    {stats::protection_fire,     6},
    {stats::protection_water,    6},
    {stats::protection_air,      6},
    {stats::protection_earth,    12},
    {stats::protection_astral,   2},
};

const std::vector<CEffect> wizard_clothes_effects{
    {stats::body, 1},
    {stats::mind, 1},
    {stats::reaction, 1},
    {stats::spirit, 1},
    {stats::speed, 1},
    {stats::skill_fire, 18},
    {stats::skill_water, 18},
    {stats::skill_air, 18},
    {stats::skill_earth, 18},
};

const std::vector<CEffect> wizard_hat_effects{
    {stats::body, 1},
    {stats::mind, 1},
    {stats::reaction, 1},
    {stats::spirit, 1},
    {stats::speed, 1},
    {stats::skill_fire, 18},
    {stats::skill_water, 18},
    {stats::skill_air, 18},
    {stats::skill_earth, 18},
    {stats::scan_range, 2},
};

const std::vector<CEffect> wizard_bijou_effects{
    {stats::body, 1},
    {stats::mind, 1},
    {stats::reaction, 1},
    {stats::spirit, 1},
    {stats::speed, 1},
    {stats::skill_fire, 15},
    {stats::skill_water, 15},
    {stats::skill_air, 15},
    {stats::skill_earth, 15},
};

std::unordered_map<int, std::vector<CItem>> awards{
    // Warrior: "Wrath" set.
    {0, {
        CItem{.Id=18515, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_clothes_effects}, // Golden Wrath Plate Cuirass
        CItem{.Id=19550, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_clothes_effects}, // Golden Wrath Plate Boots
        CItem{.Id=19034, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_clothes_effects}, // Golden Wrath Scale Gauntlets
        CItem{.Id=18774, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_clothes_effects}, // Golden Wrath Plate Bracers
        CItem{.Id=18257, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_clothes_effects}, // Golden Wrath Plate Mail
        CItem{.Id=17730, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_bijou_effects}, // Golden Wrath Plate Amulet
        CItem{.Id=17473, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_bijou_effects}, // Golden Wrath Plate Ring
    }},
    // Amazon: "Flamer" set.
    {128, {
        CItem{.Id=55346, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_clothes_effects}, // Flamer Cuirass
        CItem{.Id=56382, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_clothes_effects}, // Flamer Boots
        CItem{.Id=55866, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_clothes_effects}, // Flamer Gauntlets
        CItem{.Id=55606, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_clothes_effects}, // Flamer Plate Bracers
        CItem{.Id=55088, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_clothes_effects}, // Flamer Mail
        CItem{.Id=54562, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_bijou_effects}, // Flamer Amulet
        CItem{.Id=54305, .IsMagic=1, .Price=2, .Count=1, .Effects=fighter_bijou_effects}, // Flamer Ring
    }},
    // Mage: "Sanctuary Red" set.
    {64, {
        CItem{.Id=1934, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_clothes_effects}, // Sanctuary Red Dress
        CItem{.Id=3227, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_clothes_effects}, // Sanctuary Red Magic Shoes
        CItem{.Id=2711, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_clothes_effects}, // Sanctuary Red Gloves
        CItem{.Id=2187, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_clothes_effects}, // Sanctuary Red Cloak
        CItem{.Id=1410, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_bijou_effects}, // Sanctuary Red Amulet
        CItem{.Id=1153, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_bijou_effects}, // Sanctuary Red Ring
        CItem{.Id=1669, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_hat_effects}, // Sanctuary Red Hood
    }},
    // Witch: "Vampic" set.
    {192, {
        CItem{.Id=55150, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_clothes_effects}, // Vampic Dress
        CItem{.Id=56443, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_clothes_effects}, // Vampic Shoes
        CItem{.Id=55927, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_clothes_effects}, // Vampic Gloves
        CItem{.Id=55403, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_clothes_effects}, // Vampic Cloak
        CItem{.Id=54626, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_bijou_effects}, // Vampic Amulet
        CItem{.Id=54369, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_bijou_effects}, // Vampic Ring
        CItem{.Id=54885, .IsMagic=1, .Price=2, .Count=1, .Effects=wizard_hat_effects}, // Vampic Crown
    }},
};

CItem Reward(uint8_t sex, int circle) {
    const auto& items = awards[sex];
    if (circle > 0 && static_cast<int>(items.size()) >= circle) {
        return items[circle - 1];
    }
    return CItem{};
}

void Advance(CCharacter& chr) {
    chr.Nick = Rename(chr.Nick);
}

double TreasureMultiplier(const CCharacter& chr) {
    int circle = Circle(chr);
    return 1.0 + circle * 0.3;
}

} // namespace circle
