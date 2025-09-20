#pragma once

#include "CCharacter.hpp"

namespace circle {

// Price to go for a new circle.
const int price = 1 * 1000 * 1000 * 1000;

// Return the current circle of the character. Everybody starts at circle 0.
int Circle(const CCharacter& chr);

// Can this character go to the next circle?
bool Allowed(CCharacter& chr);

// Return the reward item for the given circle.
CItem Reward(uint8_t sex, int circle);

// Advance to a new circle: rename character.
void Advance(CCharacter& chr);

} // namespace circle
