#include "kill_stats.h"
#include "utils.hpp"

bool KillStats::Unmarshal(BinaryStream& section) {
    uint32_t total_size = section.ReadUInt32();
    if (total_size != 2560) {
        Printf(LOG_Warning, "[kill_stats] Failed to unmarshal section: wrong total size: got %d, want 2560\n", total_size);
        return false;
    }

    uint32_t pos = 0;

    uint8_t leading_byte = 0;
    do {
        leading_byte = section.ReadUInt8();
        
        auto new_size = pos + (leading_byte & 0x7F);
        if (new_size > this->by_server_id.size()) {
            Printf(LOG_Warning, "[kill_stats] Failed to unmarshal section: unpacks to more data than 2560 bytes: at least %d\n", new_size);
            return false;
        }

        if (leading_byte & 0x80) { // "Fill" section.
            leading_byte &= 0x7F;
            uint8_t fill_with = section.ReadUInt8();

            for (uint8_t i = 0; i < leading_byte; ++i) {
                this->by_server_id[pos] = fill_with;
                pos++;
            }
        } else { // Verbatim section of length `leading_byte`.
            for (uint8_t i = 0; i < leading_byte; ++i) {
                this->by_server_id[pos] = section.ReadUInt8();
                pos++;
            }
        }
    } while (pos < this->by_server_id.size() && leading_byte);

    return true;
}

bool KillStats::Marshal(BinaryStream& section) {
    section.WriteUInt32(this->by_server_id.size());

    uint32_t pos = 0;
    while (pos < this->by_server_id.size()) {
        if (this->by_server_id[pos] == this->by_server_id[pos+1]) {
            int amount = 2;
            for (int i = 2; i < 127 && pos + i < this->by_server_id.size(); ++i) {
                if (this->by_server_id[pos] != this->by_server_id[pos + i]) {
                    amount = i;
                    break;
                }
                ++amount;
            }

            section.WriteUInt8(amount | 0x80);
            section.WriteUInt8(this->by_server_id[pos]);
            pos += amount;
        } else {
            int amount = 1;
            for (int i = 1; i < 127 && pos + i + 1 < this->by_server_id.size(); ++i) {
                if (this->by_server_id[pos + i] == this->by_server_id[pos + i + 1]) {
                    amount = i;
                    break;
                }
            }
            section.WriteUInt8(amount);
            for (int i = 0; i < amount; ++i) {
                section.WriteUInt8(this->by_server_id[pos]);
                pos++;
            }
        }
    }

    return true;
}
