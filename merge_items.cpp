#include "merge_items.hpp"

#include "login.hpp"

void Improve(CItem& item);

void MergeItems(CItemList& bag) {
    if (bag.Items.size() < MERGE_COUNT) {
        return;
    }

    for (auto it = bag.Items.begin(); it < bag.Items.end() - (MERGE_COUNT-1); ++it) {
        if (it->Price != 2) {
            continue;
        }

        auto next = it;
        bool match = true;

        for (int i = 0; i < MERGE_COUNT - 1; ++i) {
            ++next;
            if (it->Id != next->Id || it->Effects != next->Effects) {
                match = false;
                break;
            }
        }

        if (!match) {
            continue;
        }

        // We have three consecutive items. Merge them.
        Improve(*it);

        // Remove the other two items.
        bag.Items.erase(it + 1, it + MERGE_COUNT);
    }
}

// Improve an item. Right now it just adds +5 max HP.
void Improve(CItem& item) {
    const int max_health = 7;
    const uint8_t delta = 5;

    item.IsMagic = true;

    for (auto& effect: item.Effects) {
        if (effect.Id1 == max_health && effect.Value1 <= 255 - delta) {
            // Add to the existing `max_health` effect.
            effect.Value1 += delta;
            return;
        }
    }

    // Create a new `max_health` effect.
    item.Effects.push_back(CEffect{.Id1=max_health, .Value1=5});
}
