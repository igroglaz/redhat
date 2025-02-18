#pragma once

#include <cstdint>
#include <unordered_map>

#include "CCharacter.hpp"
#include "server_id.hpp"

namespace update_character {

extern std::unordered_map<ServerIDType, std::unordered_map<uint16_t, uint8_t>> girl_needs_monster_kills;

bool HasKillsForReborn(CCharacter& chr, ServerIDType server_id);

} // namespace update_character
