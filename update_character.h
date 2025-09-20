#pragma once

#include <cstdint>
#include <unordered_map>

#include "CCharacter.hpp"
#include "server_id.hpp"
#include "shelf.hpp"

namespace update_character {

extern std::unordered_map<ServerIDType, std::unordered_map<uint16_t, uint8_t>> girl_needs_monster_kills;

bool HasKillsForReborn(CCharacter& chr, ServerIDType server_id);

// Reset monster kills for reborn restriction
void ClearMonsterKills(CCharacter& chr);

void TreasureOnNightmare(CCharacter& chr, bool coinflip);

// Increase stats from getting a treasure on NIGHTMARE+.
void DrinkTreasure(CCharacter& chr, ServerIDType server_id, int treasures);

// Checks if the legend character has completed a certain level.
const char* NightmareCheckpoint(const CCharacter& chr, uint8_t stats_before[4]);

// At QUEST_T1--QUEST_T3 a character can increase stats up to 70.
bool IncreaseUpTo(uint8_t* value, uint8_t increment, uint8_t limit);

void SaveTreasurePoints(int character_id, ServerIDType server_id, unsigned int points);

void VisitShelf(CCharacter& chr, ServerIDType server_id);

// Wipe mage's spells: remove all spells, leave only healing and the main skill arrow.
void WipeSpells(CCharacter& chr);

void StoreOnShelf(ServerIDType server_id, CCharacter& chr, bool store_dress, shelf::StoreOnShelfFunction store_on_shelf);

bool IsAttemptingReborn(const CCharacter& chr, ServerIDType server_id);
bool MeetsRebornCriteria(CCharacter& chr, ServerIDType server_id, int have_treasures);
void FailReborn(CCharacter& chr, ServerIDType server_id);
uint32_t RebornPrice(const CCharacter& chr, ServerIDType server_id);
void PerformReborn(CCharacter& chr, ServerIDType server_id, shelf::StoreOnShelfFunction store_on_shelf);

int ConsumeTreasures(CCharacter& chr, ServerIDType server_id);
void CutOffExperienceOnReborn(CCharacter& chr, ServerIDType server_id);
void ExperienceLimit(CCharacter& chr, ServerIDType server_id);

bool ShouldReclass(const CCharacter& chr, ServerIDType server_id);
void PerformReclass(CCharacter& chr, ServerIDType server_id, shelf::StoreOnShelfFunction store_on_shelf);

bool ShouldAscend(const CCharacter& chr, ServerIDType server_id);
void PerformAscend(CCharacter& chr, ServerIDType server_id, shelf::StoreOnShelfFunction store_on_shelf);

// Allows creating female characters for players who reached NIGHTMARE with 0 deaths and 177777777 experience.
void MaybeAllowFemale(const CCharacter& chr, ServerIDType server_id);

} // namespace update_character
