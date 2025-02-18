#include "update_character.h"

#include "kill_stats.h"
#include "utils.hpp"

namespace update_character {

std::unordered_map<ServerIDType, std::unordered_map<uint16_t, uint8_t>> girl_needs_monster_kills{
    /* Example:
    {EASY, {
        {600, 14}, // Goblin_Pike
        {604, 14}, // Goblin_Sling
        {608, 14}, // Orc_Sword
        {612, 14}, // Orc_Bow
        {657, 14}, // F_Zombie.1
        {661, 14}, // A_Zombie.2
        {664, 14}, // F_Skeleton.1
        {668, 14}, // A_Skeleton.1
    }},
    {KIDS, {
        {600, 14}, // Goblin_Pike
    }},
    {NIVAL, {
        {601, 14}, // Goblin_Pike.2
    }},
    {MEDIUM, {
        {602, 14}, // Goblin_Pike.3
    }},
    {HARD, {
        {603, 14}, // Goblin_Pike.4
    }},
    */
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
