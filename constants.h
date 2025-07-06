#pragma once

#include <cstdint>

namespace stats {
    // 1 unused
    const uint8_t body = 2;
    const uint8_t mind = 3;
    const uint8_t reaction = 4;
    const uint8_t spirit = 5;
    const uint8_t hp = 6;
    const uint8_t hp_max = 7;
    const uint8_t hp_regen = 8;
    const uint8_t mp = 9;
    const uint8_t mp_max = 10;
    const uint8_t mp_regen = 11;
    const uint8_t attack = 12;
    const uint8_t damage_min = 13;
    const uint8_t damage_max = 14;
    const uint8_t defence = 15;
    const uint8_t absorption = 16;
    const uint8_t speed = 17;
    const uint8_t rotation_speed = 18;
    const uint8_t scan_range = 19;
    // 20 unused; "protection0"
    const uint8_t protection_fire = 21;
    const uint8_t protection_water = 22;
    const uint8_t protection_air = 23;
    const uint8_t protection_earth = 24;
    const uint8_t protection_astral = 25;
    const uint8_t damage_bonus_percent = 26; // Custom logic in server.
    const uint8_t skill_blade = 27;
    const uint8_t skill_axe = 28;
    const uint8_t skill_bludgeon = 29;
    const uint8_t skill_pike = 30;
    const uint8_t skill_shooting = 31;
    // 32 unused; "mageskill0"
    const uint8_t skill_fire = 33;
    const uint8_t skill_water = 34;
    const uint8_t skill_air = 35;
    const uint8_t skill_earth = 36;
    const uint8_t skill_astral = 37;
    // 38 unused; "item lore"
    // 39 unused; "magic lore"
    // 40 unused; "creature lore"
    const uint8_t damage_bonus = 41;
}
