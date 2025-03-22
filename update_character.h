#pragma once

#include <cstdint>
#include <unordered_map>

#include "CCharacter.hpp"
#include "server_id.hpp"

namespace update_character {

extern std::unordered_map<ServerIDType, std::unordered_map<uint16_t, uint8_t>> girl_needs_monster_kills;

bool HasKillsForReborn(CCharacter& chr, ServerIDType server_id);

// Reset monster kills for reborn restriction
void ClearMonsterKills(CCharacter& chr);

void TreasureOnNightmare(CCharacter& chr, bool coinflip);

// At QUEST_T1--QUEST_T3 a character can increase stats up to 70.
bool IncreaseUpTo(uint8_t* value, uint8_t increment, uint8_t limit);

void SaveTreasurePoints(int character_id, ServerIDType server_id, unsigned int points);

} // namespace update_character
