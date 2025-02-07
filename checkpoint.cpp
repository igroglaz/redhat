#include "checkpoint.h"
#include "sql.hpp"
#include "utils.hpp"

namespace checkpoint {

Checkpoint::Checkpoint(CCharacter& chr, bool only_stats) {
    this->loaded_from_db = false;

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
}
    
Checkpoint::Checkpoint(int character_id) {
    this->loaded_from_db = false;

    SimpleSQL data(Format("SELECT * FROM `checkpoint` WHERE `id` = %d;", character_id));
    if (!data) {
        Printf(LOG_Warning, "[checkpoint] failed to select checkpoint data for %d\n", character_id);
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
}

bool Checkpoint::SaveToDB(int character_id) const {
    SimpleSQL exists(Format("SELECT 1 FROM `checkpoint` WHERE `id` = %d;", character_id));

    Printf(LOG_Info, "[checkpoint] saving for character %d (exists: %d)\n", character_id, SQL_NumRows(exists.result));

    if (!exists || SQL_NumRows(exists.result) == 0) {
        return SimpleSQL(Format(
            R"(
                INSERT INTO `checkpoint` (
                    id,
                    body, reaction, mind, spirit,
                    monsters_kills, players_kills, frags, deaths,
                    exp_fire_blade, exp_water_axe, exp_air_bludgeon, exp_earth_pike, exp_astral_shooting,
                    dress
                ) VALUES (
                    %d, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, "%s"
                );
            )",
            character_id,
            this->body, this->reaction, this->mind, this->spirit,
            this->monsters_kills, this->players_kills, this->frags, this->deaths,
            this->exp_fire_blade, this->exp_water_axe, this->exp_air_bludgeon, this->exp_earth_pike, this->exp_astral_shooting,
            this->dress.c_str()
        ));
    } else {
        return SimpleSQL(Format(
            R"(
                UPDATE `checkpoint`
                SET
                    body = %u, reaction = %u, mind = %u, spirit = %u,
                    monsters_kills = %u, players_kills = %u, frags = %u, deaths = %u,
                    exp_fire_blade = %u, exp_water_axe = %u, exp_air_bludgeon = %u, exp_earth_pike = %u, exp_astral_shooting = %u,
                    dress = "%s"
                WHERE id = %d;
            )",
            this->body, this->reaction, this->mind, this->spirit,
            this->monsters_kills, this->players_kills, this->frags, this->deaths,
            this->exp_fire_blade, this->exp_water_axe, this->exp_air_bludgeon, this->exp_earth_pike, this->exp_astral_shooting,
            this->dress.c_str(), character_id
        ));
    }
}

bool UpdateDeaths(int character_id, uint32_t deaths) {
    return SimpleSQL(Format("UPDATE `checkpoint` SET deaths = %d WHERE id = %d", deaths, character_id));
}

}
