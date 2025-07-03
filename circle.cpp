#include "circle.h"

#include <unordered_map>

#include "kill_stats.h"
#include "utils.hpp"


namespace circle {

std::unordered_map<uint16_t, uint8_t> circling_needs_monster_kills{
    {803, 14}, // Orc_Sword.5
    {804, 14}, // Orc_Bow.5
    {813, 14}, // Dragon.5
    {814, 14}, // Orc_Shaman.5
    {815, 14}, // F_Zombie.5
    {816, 14}, // A_Zombie.5
    {817, 14}, // F_Skeleton.5
    {818, 14}, // A_Skeleton.5
    {819, 14}, // M_Skeleton.5
};

int Circle(const CCharacter& chr) {
    char maybe_circle = chr.Nick[0];
    if (chr.Nick[0] == '_' || chr.Nick[0] == '@') {
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
    if (chr.Reaction < 76 || chr.Mind < 76 || chr.Spirit < 76) {
        Printf(LOG_Info, "[circle] '%s' failed to circle: stats", chr.GetFullName().c_str());
        return false;
    }

    // Are they experienced enough?
    if (chr.ExpFireBlade + chr.ExpWaterAxe + chr.ExpAirBludgeon + chr.ExpEarthPike + chr.ExpAstralShooting < 177777777) {
        Printf(LOG_Info, "[circle] '%s' failed to circle: stats", chr.GetFullName().c_str());
        return false;
    }

    // Do they have the money for the ticket?
    if (chr.Money < price) {
        Printf(LOG_Info, "[circle] '%s' failed to circle: money", chr.GetFullName().c_str());
        return false;
    }

    // Have they learned enough about the monsters?
    KillStats stats;
    if (!stats.Unmarshal(chr.Section55555555)) {
        return true; // Should not happen. Return `true` to simplify tests.
    }

    for (auto it = circling_needs_monster_kills.begin(); it != circling_needs_monster_kills.end(); ++it) {
        if (stats.by_server_id[it->first] < it->second) {
            Printf(LOG_Info, "[circle/kills] Player %s has %d kills of %d, want %d\n", chr.GetFullName().c_str(), stats.by_server_id[it->first], it->first, it->second);
            return false;
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

std::unordered_map<int, std::vector<CItem>> awards{
    // Warrior: "Wrath" set.
    {0, {
        CItem{.Id=18515, .IsMagic=0, .Price=2, .Count=1}, // Golden Wrath Plate Cuirass
        CItem{.Id=19550, .IsMagic=0, .Price=2, .Count=1}, // Golden Wrath Plate Boots
        CItem{.Id=19034, .IsMagic=0, .Price=2, .Count=1}, // Golden Wrath Scale Gauntlets
        CItem{.Id=18774, .IsMagic=0, .Price=2, .Count=1}, // Golden Wrath Plate Bracers
        CItem{.Id=18257, .IsMagic=0, .Price=2, .Count=1}, // Golden Wrath Plate Mail
        CItem{.Id=17730, .IsMagic=0, .Price=2, .Count=1}, // Golden Wrath Plate Amulet
        CItem{.Id=17473, .IsMagic=0, .Price=2, .Count=1}, // Golden Wrath Plate Ring
    }},
    // Amazon: "Flamer" set.
    {128, {
        CItem{.Id=55346, .IsMagic=0, .Price=2, .Count=1}, // Flamer Cuirass
        CItem{.Id=56382, .IsMagic=0, .Price=2, .Count=1}, // Flamer Boots
        CItem{.Id=55866, .IsMagic=0, .Price=2, .Count=1}, // Flamer Gauntlets
        CItem{.Id=55606, .IsMagic=0, .Price=2, .Count=1}, // Flamer Plate Bracers
        CItem{.Id=55088, .IsMagic=0, .Price=2, .Count=1}, // Flamer Mail
        CItem{.Id=54562, .IsMagic=0, .Price=2, .Count=1}, // Flamer Amulet
        CItem{.Id=54305, .IsMagic=0, .Price=2, .Count=1}, // Flamer Ring
    }},
    // Mage: "Sanctuary Red" set.
    {64, {
        CItem{.Id=1934, .IsMagic=0, .Price=2, .Count=1}, // Sanctuary Red Dress
        CItem{.Id=1669, .IsMagic=0, .Price=2, .Count=1}, // Sanctuary Red Hood
        CItem{.Id=3227, .IsMagic=0, .Price=2, .Count=1}, // Sanctuary Red Magic Shoes
        CItem{.Id=2711, .IsMagic=0, .Price=2, .Count=1}, // Sanctuary Red Gloves
        CItem{.Id=2187, .IsMagic=0, .Price=2, .Count=1}, // Sanctuary Red Cloak
        CItem{.Id=1410, .IsMagic=0, .Price=2, .Count=1}, // Sanctuary Red Amulet
        CItem{.Id=1153, .IsMagic=0, .Price=2, .Count=1}, // Sanctuary Red Ring
    }},
    // Witch: "Vampic" set.
    {192, {
        CItem{.Id=55150, .IsMagic=0, .Price=2, .Count=1}, // Vampic Dress
        CItem{.Id=54885, .IsMagic=0, .Price=2, .Count=1}, // Vampic Crown
        CItem{.Id=56443, .IsMagic=0, .Price=2, .Count=1}, // Vampic Shoes
        CItem{.Id=55927, .IsMagic=0, .Price=2, .Count=1}, // Vampic Gloves
        CItem{.Id=55403, .IsMagic=0, .Price=2, .Count=1}, // Vampic Cloak
        CItem{.Id=54626, .IsMagic=0, .Price=2, .Count=1}, // Vampic Amulet
        CItem{.Id=54369, .IsMagic=0, .Price=2, .Count=1}, // Vampic Ring
    }},
};

CItem Award(const CCharacter& chr, int circle) {
    const auto& items = awards[chr.Sex];
    if (circle > 0 && static_cast<int>(items.size()) >= circle) {
        return items[circle - 1];
    }
    return CItem{};
}

void Advance(CCharacter& chr) {
    const int leaving_circle = Circle(chr);

    chr.Nick = Rename(chr.Nick);

    if (leaving_circle != 0) {
        const CItem item = Award(chr, leaving_circle);

        if (item.Id) {
            chr.Bag.Items.insert(chr.Bag.Items.begin(), item);
        }
    }
}

} // namespace circle
