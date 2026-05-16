#include "relic.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "circle.h"
#include "client.hpp"
#include "login.hpp"
#include "sql.hpp"
#include "utils.hpp"

// Ascension staff (Poison Cloud Staff): physical damage staff.
CItem ascension_staff{.Id=53709, .IsMagic=0, .Price=2, .Count=1};

// Ascension crown (Good Gold Helm): +3 body +2 scanRange +250 attack.
CItem ascension_crown{.Id=18118, .IsMagic=1, .Price=2, .Count=1, .Effects={
    {stats::body, 3},
    {stats::scan_range, 2},
    {stats::attack, 250},
}};

std::unordered_map<uint16_t, uint8_t> relic_id_to_sex; // Relic item IDs mapped to mage/fighter.
std::unordered_map<uint16_t, CItem> all_relics;

void InitializeRelics() {
    if (!relic_id_to_sex.empty()) {
        return;
    }

    relic_id_to_sex[ascension_crown.Id] = sex::warrior;
    relic_id_to_sex[ascension_staff.Id] = sex::wizard;

    all_relics[ascension_crown.Id] = ascension_crown;
    all_relics[ascension_staff.Id] = ascension_staff;

    for (const auto& [sex, awards] : circle::AllAwards()) {
        for (const auto& award : awards) {
            relic_id_to_sex[award.Id] = sex & sex::mage;

            all_relics[award.Id] = award;
        }
    }
}

bool IsRelic(const CItem& item) {
    InitializeRelics();
    return relic_id_to_sex.count(item.Id) > 0;
}

uint32_t ReadSigil(const CItem& item) {
    if (!IsRelic(item) || item.Effects.size() == 0) {
        return 0;
    }

    uint32_t sigil = 0;
    const uint8_t sex = relic_id_to_sex[item.Id];
    
    for (const auto& effect : item.Effects) {
        if ((sex == sex::warrior && effect.Id1 == stats::skill_fire) || (sex == sex::wizard && effect.Id1 == stats::skill_blade)) {
            sigil |= effect.Value1;
        } else if ((sex == sex::warrior && effect.Id1 == stats::skill_water) || (sex == sex::wizard && effect.Id1 == stats::skill_axe)) {
            sigil |= effect.Value1 << 8;
        } else if ((sex == sex::warrior && effect.Id1 == stats::skill_air) || (sex == sex::wizard && effect.Id1 == stats::skill_bludgeon)) {
            sigil |= effect.Value1 << 16;
        } else if ((sex == sex::warrior && effect.Id1 == stats::skill_earth) || (sex == sex::wizard && effect.Id1 == stats::skill_pike)) {
            sigil |= effect.Value1 << 24;
        }
    }

    return sigil;
}

void WriteSigilByte(CItem& item, uint8_t id, uint8_t value) {
    value &= 0xff;
    if (value != 0) {
        item.Effects.push_back(CEffect(id, value));
    }
}

bool EmbossSigil(CItem& item, uint32_t sigil) {
    if (!IsRelic(item)) {
        Printf(LOG_Warning, "[relic] Attempted to emboss a non-relic item (ID: %u)\n", item.Id);
        return false;
    }

    uint32_t existing_sigil = ReadSigil(item);
    if (existing_sigil != 0) {
        if (existing_sigil != sigil) {
            Printf(LOG_Warning, "[relic] Attempted to emboss a relic that already has a sigil: item ID: %u, existing sigil: %u, attempted new sigil: %u\n", item.Id, existing_sigil, sigil);
            return false;
        }
        return true;
    }

    uint8_t sex = relic_id_to_sex[item.Id];

    if (sex == sex::warrior) {
        item.IsMagic = 1;
        WriteSigilByte(item, stats::skill_fire, sigil);
        WriteSigilByte(item, stats::skill_water, sigil >> 8);
        WriteSigilByte(item, stats::skill_air, sigil >> 16);
        WriteSigilByte(item, stats::skill_earth, sigil >> 24);
    } else {
        item.IsMagic = 1;
        WriteSigilByte(item, stats::skill_blade, sigil);
        WriteSigilByte(item, stats::skill_axe, sigil >> 8);
        WriteSigilByte(item, stats::skill_bludgeon, sigil >> 16);
        WriteSigilByte(item, stats::skill_pike, sigil >> 24);
    }

    return true;
}

bool ShouldHaveSigil(const CCharacter& chr, bool ascended) {
    if (ascended) {
        return true;
    }

    auto circle_number = circle::Circle(chr);
    if (circle_number > 1 || (circle_number == 1 && IsCharacterAllowed(chr, NIGHTMARE))) {
        return true;
    }

    return false;
}

int ClaimedRelics(const CCharacter& chr, int ascended, uint16_t item_id) {
    if (ascended) {
        if (item_id == (chr.Sex & sex::wizard ? ascension_staff.Id : ascension_crown.Id)) {
            return ascended;
        }
    }

    int circle_number = circle::Circle(chr);
    int claimed_circle = IsCharacterAllowed(chr, NIGHTMARE) ? circle_number : circle_number - 1;

    const auto& awards = circle::AllAwards();
    for (int i = 1; i <= claimed_circle; i++) {
        auto it = awards.find(chr.Sex);
        if (it != awards.end() && it->second[i-1].Id == item_id) {
            return 1;
        }
    }

    return 0;
}

std::mutex bestow_mutex;

uint32_t BestowSigil(const CCharacter& chr) {
    std::lock_guard<std::mutex> lock(bestow_mutex);

    uint32_t sigil = 0;

    SimpleSQL existing_sigil(Format("SELECT sigil_id FROM sigil WHERE character_id = %d;", chr.ID));
    if (!existing_sigil) {
        Printf(LOG_Warning, "[relic] Failed to query existing sigil for character %d: %s\n", chr.ID, SQL_Error().c_str());
        return 0;
    }

    if (existing_sigil && SQL_NumRows(existing_sigil.result) > 0) {
        MYSQL_ROW row = SQL_FetchRow(existing_sigil.result);
        return SQL_FetchInt(row, existing_sigil.result, "sigil_id");
    }
    
    SimpleSQL insert_result(Format("INSERT INTO sigil (character_id) VALUES (%d);", chr.ID));
    if (!insert_result) {
        Printf(LOG_Warning, "[relic] Failed to insert new sigil for character %d: %s\n", chr.ID, SQL_Error().c_str());
        return 0;
    }

    SimpleSQL read_sigil(Format("SELECT sigil_id FROM sigil WHERE character_id = %d;", chr.ID));
    if (!read_sigil) {
        Printf(LOG_Warning, "[relic] Failed to query newly created sigil for character %d: %s\n", chr.ID, SQL_Error().c_str());
        return 0;
    }

    MYSQL_ROW row = SQL_FetchRow(read_sigil.result);
    return SQL_FetchInt(row, read_sigil.result, "sigil_id");
}

void RestoreRelics(bool dry_run) {
    InitializeRelics();

    // First, load all sigils.
    SimpleSQL sigils_query("SELECT character_id, sigil_id FROM sigil;");
    if (!sigils_query) {
        Printf(LOG_Error, "[relic] Failed to query characters for relic restoration: %s\n", SQL_Error().c_str());
        return;
    }

    std::unordered_map<int, uint32_t> id_to_sigil;
    
    int sigil_rows = SQL_NumRows(sigils_query.result);
    for (int i = 0; i < sigil_rows; i++) {
        auto row = SQL_FetchRow(sigils_query.result);
        int character_id = SQL_FetchInt(row, sigils_query.result, "character_id");
        uint32_t sigil_id = SQL_FetchInt(row, sigils_query.result, "sigil_id");
        id_to_sigil[character_id] = sigil_id;
    }

    // Second, load all characters and compute claimed relics.
    SimpleSQL query("SELECT id, login_id, nick, class, ascended, dress, bag FROM characters WHERE deleted = 0 ORDER BY id ASC;");
    if (!query) {
        Printf(LOG_Error, "[relic] Failed to query characters for relic restoration: %s\n", SQL_Error().c_str());
        return;
    }

    std::vector<CCharacter> all_characters;
    std::unordered_map<int, std::unordered_map<uint16_t, int>> sigil_id_to_claimed_relics;

    int character_rows = SQL_NumRows(query.result);
    for (int i = 0; i < character_rows; i++) {
        auto row = SQL_FetchRow(query.result);

        CCharacter chr;
        chr.ID = SQL_FetchInt(row, query.result, "id");
        chr.LoginID = SQL_FetchInt(row, query.result, "login_id");
        chr.Nick = SQL_FetchString(row, query.result, "nick");
        chr.Sex = static_cast<uint8_t>(SQL_FetchInt(row, query.result, "class"));
        int ascended = SQL_FetchInt(row, query.result, "ascended");
        chr.Dress = Login_UnserializeItems(SQL_FetchString(row, query.result, "dress"));
        chr.Bag = Login_UnserializeItems(SQL_FetchString(row, query.result, "bag"));

        all_characters.push_back(chr);

        if (!ShouldHaveSigil(chr, ascended)) {
            continue;
        }

        uint32_t sigil = id_to_sigil[chr.ID];
        if (sigil == 0) {
            Printf(LOG_Warning, "[relic] Character %d (%s) should have a sigil but doesn't have one in the database, relics won't be restored for this character\n", chr.ID, chr.Nick.c_str());
            continue;
        }

        for (auto& relic_id: relic_id_to_sex) {
            int amount = ClaimedRelics(chr, ascended, relic_id.first);
            if (amount > 0) {
                sigil_id_to_claimed_relics[sigil][relic_id.first] += amount;
            }
        }

        if (!sigil_id_to_claimed_relics[sigil].empty()) {
            std::string claimed_relics_str;
            for (auto& [relic_id, amount] : sigil_id_to_claimed_relics[sigil]) {
                if (!claimed_relics_str.empty()) {
                    claimed_relics_str += ", ";
                }
                claimed_relics_str += Format("%u (x%d)", relic_id, amount);
            }
            Printf(LOG_Info, "[relic] Character %d (%s) has claimed relics: %s\n", chr.ID, chr.Nick.c_str(), claimed_relics_str.c_str());
        }
    }

    // Third, subtract equipped/carried relics.
    for (auto& chr : all_characters) {
        for (auto& item : chr.Dress.Items) {
            if (IsRelic(item)) {
                auto sigil = ReadSigil(item);
                if (sigil == 0) {
                    Printf(LOG_Warning, "[relic] Found an unembossed relic (item ID %u) in the dress of character %d during relic restoration\n", item.Id, chr.ID);
                    continue;
                }
                if (--sigil_id_to_claimed_relics[sigil][item.Id] == 0) {
                    sigil_id_to_claimed_relics[sigil].erase(item.Id);
                }
            }
        }
        for (auto& item : chr.Bag.Items) {
            if (IsRelic(item)) {
                auto sigil = ReadSigil(item);
                if (sigil == 0) {
                    Printf(LOG_Warning, "[relic] Found an unembossed relic (item ID %u) in the bag of character %d during relic restoration\n", item.Id, chr.ID);
                    continue;
                }
                if (--sigil_id_to_claimed_relics[sigil][item.Id] == 0) {
                    sigil_id_to_claimed_relics[sigil].erase(item.Id);
                }
            }
        }
    }

    // Fourth, subtract shelved relics. Those can only be at NIGHTMARE server.
    SimpleSQL shelf_query("SELECT login_id, server_id, cabinet, items FROM shelf WHERE server_id = 6 ORDER BY login_id ASC, server_id ASC, cabinet ASC;");
    if (!shelf_query) {
        Printf(LOG_Error, "[relic] Failed to query shelved items for relic restoration: %s\n", SQL_Error().c_str());
        return;
    }

    int shelf_rows = SQL_NumRows(shelf_query.result);

    for (int i = 0; i < shelf_rows; i++) {
        auto row = SQL_FetchRow(shelf_query.result);
        int login_id = SQL_FetchInt(row, shelf_query.result, "login_id");
        int server_id = SQL_FetchInt(row, shelf_query.result, "server_id");
        int cabinet = SQL_FetchInt(row, shelf_query.result, "cabinet");
        auto items = Login_UnserializeItems(SQL_FetchString(row, shelf_query.result, "items"));

        for (auto& item : items.Items) {
            if (IsRelic(item)) {
                auto sigil = ReadSigil(item);
                if (sigil == 0) {
                    Printf(LOG_Warning, "[relic] Found an unembossed relic (item ID %u) on the shelf at login ID %d, server ID %d, cabinet %d during relic restoration\n", item.Id, login_id, server_id, cabinet);
                    continue;
                }
                if (--sigil_id_to_claimed_relics[sigil][item.Id] == 0) {
                    sigil_id_to_claimed_relics[sigil].erase(item.Id);
                }
            }
        }
    }

    // Finally, we have all the items that need to be restored.
    std::unordered_map<uint32_t, CCharacter> sigil_to_character;
    for (auto& chr : all_characters) {
        uint32_t sigil = id_to_sigil[chr.ID];
        if (sigil == 0) {
            continue;
        }
        sigil_to_character[sigil] = chr;
    }

    for (auto& [sigil, relics] : sigil_id_to_claimed_relics) {
        if (relics.empty()) {
            continue;
        }

        auto character_it = sigil_to_character.find(sigil);
        if (character_it == sigil_to_character.end()) {
            Printf(LOG_Warning, "[relic] Got restorable relics for sigil %u but couldn't find a character with that sigil during relic restoration\n", sigil);
            continue;
        }
        auto& chr = character_it->second;

        for (auto& [relic_id, amount] : relics) {
            for (int i = 0; i < amount; i++) {
                Printf(LOG_Info, "[relic] Restoring relic (item ID %u) for character %d with sigil %u\n", relic_id, chr.ID, sigil);
                CItem relic = all_relics[relic_id];
                if (!relic.Id) {
                    Printf(LOG_Warning, "[relic] Relic with item ID %u is not a relic; for character %d during relic restoration\n", relic_id, chr.ID);
                    continue;
                }

                if (!EmbossSigil(relic, sigil)) {
                    continue;
                }

                chr.Bag.Items.push_back(relic);
            }
        }

        if (dry_run) {
            Printf(LOG_Info, "[relic] Dry-run: not saving restored relics for character %d\n", chr.ID);
            continue;
        }

        SimpleSQL update_query(Format("UPDATE characters SET bag = '%s' WHERE id = %d;", SQL_Escape(Login_SerializeItems(chr.Bag)).c_str(), chr.ID));
        if (!update_query) {
            Printf(LOG_Error, "[relic] Failed to update character %d with restored relics: %s\n", chr.ID, SQL_Error().c_str());
            continue;
        }
        int affected_rows = SQL_AffectedRows();
        if (affected_rows != 1) {
            Printf(LOG_Warning, "[relic] Update affected %d rows when saving restored relics for character %d\n", affected_rows, chr.ID);
        }
    }
}
