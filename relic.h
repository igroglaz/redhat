#pragma once

#include <functional>

#include "CCharacter.hpp"

extern CItem ascension_staff;
extern CItem ascension_crown;

bool IsRelic(const CItem& item);

uint32_t ReadSigil(const CItem& item);
bool EmbossSigil(CItem& item, uint32_t sigil);

// ClaimedRelics checks whether the character has achieved a given relic at least once before.
// Ascension can be awarded multiple times, so we return an int. Circles can only be awarded once.
int ClaimedRelics(const CCharacter& chr, int ascended, uint16_t item_id);

uint32_t BestowSigil(const CCharacter& chr);

bool MigrateRelics(bool dry_run);
void RestoreRelics(bool dry_run);
