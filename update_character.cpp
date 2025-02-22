#include "update_character.h"

#include "kill_stats.h"
#include "utils.hpp"

namespace update_character {

std::unordered_map<ServerIDType, std::unordered_map<uint16_t, uint8_t>> girl_needs_monster_kills{

    {EASY, {
        {657, 14}, // F_Zombie.1
        {661, 14}, // A_Zombie.2
        {664, 14}, // F_Skeleton.1
        {668, 14}, // A_Skeleton.1
        {628, 14}, // Ghost
        {629, 14}, // Ghost.2
        {632, 14}, // Bee
        {707, 14}, // Spider
    }},
    {KIDS, {
        ////////////////////// 2
        {617, 2}, // Ogre.2
        {621, 2}, // Troll.2
        {2374, 2}, // Demon
        {711, 2}, // Succubus
        ////////////////////// 14
        {609, 14}, // Orc_Sword.2
        {625, 14}, // Bat_Sonic.2
        {633, 14}, // Bee.2
        {707, 14}, // Spider
    }},
    {NIVAL, {
        ////////////////////// 4
        {711, 4}, // Necro_Leader2
        ////////////////////// 14
        {711, 14}, // Necro_Female2
        {669, 14}, // A_Skeleton.2
        {661, 14}, // A_Zombie.2
        {625, 14}, // Bat_Sonic.2
        {633, 14}, // Bee.2
        {708, 14}, // Spider.2
        {617, 14}, // Ogre.2
        {621, 14}, // Troll.2
    }},
    {MEDIUM, {
        ////////////////////// 14
        {626, 14}, // Bat_Sonic.3
        {634, 14}, // Bee.3
        {709, 14}, // Spider.3
        {717, 14}, // Dino.3
        {666, 14}, // F_Skeleton.3
        {659, 14}, // F_Zombie.3
        {630, 14}, // Ghost.3
        {618, 14}, // Ogre.3
        {622, 14}, // Troll.3
        {713, 14}, // Succubus.3
        {655, 14}, // Orc_Shaman.3
        {610, 14}, // Orc_Sword.3
        {614, 14}, // Orc_Bow.3
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
        {812, 14}, // Turtle.5
        {808, 14}, // Ghost.5
        {714, 14}, // Succubus.4
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

} // namespace update_character
