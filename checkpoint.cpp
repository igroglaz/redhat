#include "checkpoint.h"
#include "sql.hpp"
#include "utils.hpp"

namespace checkpoint {

namespace {

unsigned CountItems(const CItemList& list) {
    unsigned count = 0;
    for (const auto& item : list.Items) {
        if (item.Id != 0) {
            ++count;
        }
    }
    return count;
}

} // namespace

Checkpoint::Checkpoint(CCharacter& chr, bool only_stats, bool save_bag) {
    this->loaded_from_db = false;
    this->load_failed = false;

    this->body = chr.Body;
    this->reaction = chr.Reaction;
    this->mind = chr.Mind;
    this->spirit = chr.Spirit;

    if (only_stats) {
        this->monsters_kills = 0;
        this->players_kills = 0;
        this->frags = 0;
        this->deaths = chr.Deaths;
        this->exp_fire_blade = 0;
        this->exp_water_axe = 0;
        this->exp_air_bludgeon = 0;
        this->exp_earth_pike = 0;
        this->exp_astral_shooting = 0;
        this->dress = "[0,0,40,12];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";
        this->bag = "[0,0,0,0]";
        return;
    }

    this->monsters_kills = chr.MonstersKills;
    this->players_kills = chr.PlayersKills;
    this->frags = chr.Frags;
    this->deaths = chr.Deaths;
    this->exp_fire_blade = chr.ExpFireBlade;
    this->exp_water_axe = chr.ExpWaterAxe;
    this->exp_air_bludgeon = chr.ExpAirBludgeon;
    this->exp_earth_pike = chr.ExpEarthPike;
    this->exp_astral_shooting = chr.ExpAstralShooting;
    this->dress = Login_SerializeItems(chr.Dress);

    CItemList saved_bag{};
    if (save_bag) {
        int non_quest_magic_items = 0;
        for (const auto& item : chr.Bag.Items) {
            if (!item.IsMagic) {
                continue;
            }
            if (item.Price == 2) {
                saved_bag.Items.push_back(item);
            } else if ((item.Id & 0xFF00) != 0x0E00 && non_quest_magic_items < 10) {
                saved_bag.Items.push_back(item);
                ++non_quest_magic_items;
            }
        }
    }
    this->bag = Login_SerializeItems(saved_bag);
}
    
Checkpoint::Checkpoint(int character_id) {
    this->loaded_from_db = false;
    this->load_failed = false;

    SimpleSQL data(Format("SELECT * FROM `checkpoint` WHERE `id` = %d;", character_id));
    if (!data) {
        Printf(LOG_Warning, "[checkpoint] failed to select checkpoint data for %d\n", character_id);
        this->load_failed = true;
        return;
    }

    if (SQL_NumRows(data.result) == 0) {
        return;
    }

    this->loaded_from_db = true;
    
    MYSQL_ROW row = SQL_FetchRow(data.result);

    this->body = static_cast<uint8_t>(SQL_FetchInt(row, data.result, "body"));
    this->reaction = static_cast<uint8_t>(SQL_FetchInt(row, data.result, "reaction"));
    this->mind = static_cast<uint8_t>(SQL_FetchInt(row, data.result, "mind"));
    this->spirit = static_cast<uint8_t>(SQL_FetchInt(row, data.result, "spirit"));
    this->monsters_kills = SQL_FetchInt(row, data.result, "monsters_kills");
    this->players_kills = SQL_FetchInt(row, data.result, "players_kills");
    this->frags = SQL_FetchInt(row, data.result, "frags");
    this->deaths = SQL_FetchInt(row, data.result, "deaths");
    this->exp_fire_blade = SQL_FetchInt(row, data.result, "exp_fire_blade");
    this->exp_water_axe = SQL_FetchInt(row, data.result, "exp_water_axe");
    this->exp_air_bludgeon = SQL_FetchInt(row, data.result, "exp_air_bludgeon");
    this->exp_earth_pike = SQL_FetchInt(row, data.result, "exp_earth_pike");
    this->exp_astral_shooting = SQL_FetchInt(row, data.result, "exp_astral_shooting");
    this->dress = SQL_FetchString(row, data.result, "dress");
    this->bag = SQL_FetchString(row, data.result, "bag");
    if (this->bag.empty()) {
        this->bag = "[0,0,0,0]";
    }
}

bool Checkpoint::SaveToDB(int character_id) const {
    SimpleSQL exists(Format("SELECT 1 FROM `checkpoint` WHERE `id` = %d;", character_id));
    bool insert = !exists || SQL_NumRows(exists.result) == 0;

    Printf(LOG_Info, "[checkpoint] saving for character %d (%s): %s\n", character_id, insert ? "insert" : "update", this->Describe().c_str());

    bool saved;
    if (insert) {
        saved = SimpleSQL(Format(
            R"(
                INSERT INTO `checkpoint` (
                    id,
                    body, reaction, mind, spirit,
                    monsters_kills, players_kills, frags, deaths,
                    exp_fire_blade, exp_water_axe, exp_air_bludgeon, exp_earth_pike, exp_astral_shooting,
                    dress, bag
                ) VALUES (
                    %d, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, "%s", "%s"
                );
            )",
            character_id,
            this->body, this->reaction, this->mind, this->spirit,
            this->monsters_kills, this->players_kills, this->frags, this->deaths,
            this->exp_fire_blade, this->exp_water_axe, this->exp_air_bludgeon, this->exp_earth_pike, this->exp_astral_shooting,
            this->dress.c_str(), this->bag.c_str()
        ));
    } else {
        saved = SimpleSQL(Format(
            R"(
                UPDATE `checkpoint`
                SET
                    body = %u, reaction = %u, mind = %u, spirit = %u,
                    monsters_kills = %u, players_kills = %u, frags = %u, deaths = %u,
                    exp_fire_blade = %u, exp_water_axe = %u, exp_air_bludgeon = %u, exp_earth_pike = %u, exp_astral_shooting = %u,
                    dress = "%s", bag = "%s"
                WHERE id = %d;
            )",
            this->body, this->reaction, this->mind, this->spirit,
            this->monsters_kills, this->players_kills, this->frags, this->deaths,
            this->exp_fire_blade, this->exp_water_axe, this->exp_air_bludgeon, this->exp_earth_pike, this->exp_astral_shooting,
            this->dress.c_str(), this->bag.c_str(), character_id
        ));
    }

    if (saved) {
        Printf(LOG_Info, "[checkpoint] saved for character %d\n", character_id);
    } else {
        Printf(LOG_Error, "[checkpoint] FAILED to save for character %d\n", character_id);
    }
    return saved;
}

std::string Checkpoint::Describe() const {
    return Format(
        "stats={%u, %u, %u, %u}, "
        "kills={monsters=%u, players=%u, frags=%u, deaths=%u}, "
        "exp={%u, %u, %u, %u, %u}, "
        "dress=%u items, bag=%u items",
        this->body, this->reaction, this->mind, this->spirit,
        this->monsters_kills, this->players_kills, this->frags, this->deaths,
        this->exp_fire_blade, this->exp_water_axe, this->exp_air_bludgeon, this->exp_earth_pike, this->exp_astral_shooting,
        CountItems(Login_UnserializeItems(this->dress)), CountItems(Login_UnserializeItems(this->bag))
    );
}

std::string DescribeCharacter(const CCharacter& chr) {
    return Format(
        "stats={%u, %u, %u, %u}, "
        "kills={monsters=%u, players=%u, frags=%u, deaths=%u}, "
        "exp={%u, %u, %u, %u, %u}, "
        "money=%u, spells=0x%x, dress=%u items, bag=%u items",
        chr.Body, chr.Reaction, chr.Mind, chr.Spirit,
        chr.MonstersKills, chr.PlayersKills, chr.Frags, chr.Deaths,
        chr.ExpFireBlade, chr.ExpWaterAxe, chr.ExpAirBludgeon, chr.ExpEarthPike, chr.ExpAstralShooting,
        chr.Money, chr.Spells, CountItems(chr.Dress), CountItems(chr.Bag)
    );
}

bool UpdateDeaths(int character_id, uint32_t deaths) {
    bool updated = SimpleSQL(Format("UPDATE `checkpoint` SET deaths = %d WHERE id = %d", deaths, character_id));
    if (updated) {
        Printf(LOG_Info, "[checkpoint] updated deaths for character %d to %u\n", character_id, deaths);
    } else {
        Printf(LOG_Error, "[checkpoint] FAILED to update deaths for character %d to %u\n", character_id, deaths);
    }
    return updated;
}

}
