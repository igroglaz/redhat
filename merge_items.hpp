#pragma once

#include "CCharacter.hpp"

const int MERGE_COUNT = 2;

// Merge identical quest items into a better one.
// The items need to appear in the bag consecutively.
void MergeItems(CItemList& bag, int server_id);
