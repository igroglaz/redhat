#include "merge_items.hpp"

#include "login.hpp"

void Improve(CItem& item, int server_id);

void MergeItems(CItemList& bag, int server_id) {
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
void Improve(CItem& item, int server_id) {
    const int max_health = 7;
    const uint8_t delta_by_server[] = {
        0, // server 0, unused
        0, // server 1, doesn't happen
        1, // server 2
        2, 2, 2, 2, 2, // servers 3--7
        4, // server 8
        3, 3, 3, // servers 9--11
    };

    if (server_id >= sizeof(delta_by_server) / sizeof(delta_by_server[0])) {
        // Should not be possible, but let's be safe.
        return;
    }

    const uint8_t delta = delta_by_server[server_id];

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
