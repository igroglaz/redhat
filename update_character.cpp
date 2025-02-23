#include "update_character.h"

#include "kill_stats.h"
#include "utils.hpp"

namespace update_character {

std::unordered_map<ServerIDType, std::unordered_map<uint16_t, uint8_t>> girl_needs_monster_kills{

    {EASY, {
        ////////////////////// 1
        {692, 1},  // Necro_Female1
        {616, 1},  // Ogre
        {620, 1},  // Troll
        ////////////////////// 14
        {657, 14}, // F_Zombie.1
        {664, 14}, // F_Skeleton.1
        {668, 14}, // A_Skeleton.1
        {629, 14}, // Ghost.2
        {715, 14}, // Dino
        {632, 14}, // Bee
        {707, 14}, // Spider
    }},
    {KIDS, {
        ////////////////////// 1
        {617, 1},  // Ogre.2
        {621, 1},  // Troll.2
        {2374, 1}, // Demon
        {711, 1},  // Succubus
        ////////////////////// 14
        {630, 14}, // Ghost.3
        {609, 14}, // Orc_Sword.2
        {633, 14}, // Bee.2
        {707, 14}, // Spider
    }},
    {NIVAL, {
        ////////////////////// 1
        {696, 1},  // Necro_Leader2
        {695, 1},  // Necro_Female2
        {694, 1},  // Necro_Male2
        {712, 1},  // Succubus.2
        ////////////////////// 14
        {669, 14}, // A_Skeleton.2
        {661, 14}, // A_Zombie.2
        {617, 14}, // Ogre.2
        {621, 14}, // Troll.2
        {625, 14}, // Bat_Sonic.2
        {708, 14}, // Spider.2
    }},
    {MEDIUM, {
        ////////////////////// 1
        {701, 1},  // Necro_Female4
        {671, 1},  // A_Skeleton.4
        {667, 1},  // F_Skeleton.4
        ////////////////////// 14
        {666, 14}, // F_Skeleton.3
        {659, 14}, // F_Zombie.3
        {618, 14}, // Ogre.3
        {622, 14}, // Troll.3
        {610, 14}, // Orc_Sword.3
        {614, 14}, // Orc_Bow.3
        {631, 14}, // Ghost.4
        {603, 14}, // Goblin_Pike.4
        {717, 14}, // Dino.3
        {626, 14}, // Bat_Sonic.3
        {634, 14}, // Bee.3
        {709, 14}, // Spider.3
    }},
    {HARD, {
        {2132, 14}, // 2F_KnightLeader4
        {2130, 14}, // 2H_Knight4
        {671, 14}, // A_Skeleton.4
        {663, 14}, // A_Zombie.4
        {667, 14}, // F_Skeleton.4
        {660, 14}, // F_Zombie.4
        {718, 14}, // Dino.4
        {673, 14}, // M_Skeleton.4
        {701, 14}, // Necro_Female4
        {702, 14}, // Necro_Leader4
        {700, 14}, // Necro_Male4
        {619, 14}, // Ogre.4
        {623, 14}, // Troll.4
        {656, 14}, // Orc_Shaman.4
        {611, 14}, // Orc_Sword.4
        {615, 14}, // Orc_Bow.4
        {627, 14}, // Bat_Sonic.4
        {635, 14}, // Bee.4
        {710, 14}, // Spider.4
        {714, 14}, // Succubus.4
        {812, 14}, // Turtle.5
        {808, 14}, // Ghost.5
    }},
};

bool HasKillsForReborn(CCharacter& chr, ServerIDType server_id) {
    if (chr.Sex == 0 || chr.Sex == 64) {
        return true;
    }

    auto requirements = girl_needs_monster_kills.find(server_id);
    if (requirements == girl_needs_monster_kills.end()) {
        return true;
    }

    KillStats stats;
    if (!stats.Unmarshal(chr.Section55555555)) {
        return true; // Should not happen. Return `true` to simplify tests.
    }

    auto map = requirements->second;
    for (auto it = map.begin(); it != map.end(); ++it) {
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

} // namespace update_character
