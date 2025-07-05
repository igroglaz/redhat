#pragma once

#include "CCharacter.hpp"

namespace circle {

// Price to go for a new circle.
const int price = 1 * 1000 * 1000 * 1000;

// Return the current circle of the character. Everybody starts at circle 0.
int Circle(const CCharacter& chr);

// Can this character go to the next circle?
bool Allowed(CCharacter& chr);

// Advance to a new circle: rename character and award an item.
void Advance(CCharacter& chr);

} // namespace circle
