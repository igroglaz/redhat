#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "CCharacter.hpp"
#include "login.hpp"

namespace checkpoint {

struct Checkpoint {
    bool loaded_from_db;

    uint8_t body;
    uint8_t reaction;
    uint8_t mind;
    uint8_t spirit;
    uint32_t monsters_kills;
    uint32_t players_kills;
    uint32_t frags;
    uint32_t deaths;
    uint32_t exp_fire_blade;
    uint32_t exp_water_axe;
    uint32_t exp_air_bludgeon;
    uint32_t exp_earth_pike;
    uint32_t exp_astral_shooting;
    std::string dress;

    Checkpoint(CCharacter& chr, bool only_stats=false);

    // Loads the checkpoint from DB for the given character.
    Checkpoint(int character_id);

    // Upserts the checkpoint to the DB for the given character.
    bool SaveToDB(int character_id) const;
};

bool UpdateDeaths(int character_id, uint32_t deaths);

} // namespace checkpoint
