#include "merge_items.hpp"

#include "login.hpp"
#include "server_id.hpp"

void Improve(CItem& item, ServerIDType server_id);

void MergeItems(CItemList& bag, ServerIDType server_id) {
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
        Improve(*it, server_id);

        // Remove the other two items.
        bag.Items.erase(it + 1, it + MERGE_COUNT);
    }
}

// Improve an item. The improvement depends on the server.
void Improve(CItem& item, ServerIDType server_id) {
    const int max_health = 7;

    uint8_t delta = 0;

    switch (server_id) {
        case EASY:
            delta = 1;
            break;
        case KIDS:
        case NIVAL:
        case MEDIUM:
        case HARD:
        case NIGHTMARE:
            delta = 2;
            break;
        case QUEST_T1:
        case QUEST_T2:
        case QUEST_T3:
        case QUEST_T4:
            delta = 3;
            break;
        default:
            delta = 0;
    }

    item.IsMagic = true;

    for (auto& effect: item.Effects) {
        if (effect.Id1 == max_health && effect.Value1 <= 255 - delta) {
            // Add to the existing `max_health` effect.
            effect.Value1 += delta;
            return;
        }
    }

    // Create a new `max_health` effect.
    item.Effects.push_back(CEffect{.Id1=max_health, .Value1=delta});
}
