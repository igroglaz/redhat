#include "config.hpp"
#include "sql.hpp"
#include "utils.hpp"
#include "shelf.hpp"
#include "login.hpp"

#include <inttypes.h>
#include <iostream>
#include <mysql.h>

#include <windows.h>

// This should be defined in inttypes.h, but some old compilers don't have that.
#ifndef PRId64
#define PRId64 "lld"
#endif // PRId64

namespace SQL
{
    bool Open = false;
    MYSQL Connection;
    HANDLE Mutex;
}

std::string SQL_Error()
{
    if(!SQL::Open)
        return "";
    return std::string(mysql_error(&SQL::Connection));
}

bool SQL_Init()
{
    MYSQL* s = mysql_init(&SQL::Connection);

    if(!s)
    {
        Printf(LOG_FatalError, "[DB] Unable to establish connection to MySQL server (at %s:%u).\n", Config::SqlAddress.c_str(), Config::SqlPort);
        Printf(LOG_FatalError, "[DB] The error reported was: %s\n", mysql_error(&SQL::Connection));
        return false;
    }

    my_bool reconnect = true;
    mysql_options(&SQL::Connection, MYSQL_OPT_RECONNECT, &reconnect);

    s = mysql_real_connect(&SQL::Connection, Config::SqlAddress.c_str(),
                            Config::SqlLogin.c_str(), Config::SqlPassword.c_str(),
                            Config::SqlDatabase.c_str(), Config::SqlPort, NULL, 0);

    if(!s)
    {
        Printf(LOG_FatalError, "[DB] Unable to establish connection to MySQL server (at %s:%u).\n", Config::SqlAddress.c_str(), Config::SqlPort);
        Printf(LOG_FatalError, "[DB] The error reported was: %s\n", mysql_error(&SQL::Connection));
        return false;
    }

    SQL::Open = true;
    SQL::Mutex = CreateMutex(NULL, FALSE, NULL);

    return true;
}

void SQL_Close()
{
    if(!SQL::Open) return;
    mysql_close(&SQL::Connection);
    CloseHandle(SQL::Mutex);
}

bool SQL_CheckConnected()
{
    return SQL::Open;
}

long int SQL_FetchInt(MYSQL_ROW row, MYSQL_RES* result, std::string fieldname)
{
    std::string str = SQL_FetchString(row, result, fieldname);
    if(!str.length() || !CheckInt(str)) return -1;
    return StrToInt(str);
}

int64_t SQL_FetchInt64(MYSQL_ROW row, MYSQL_RES* result, std::string fieldname) {
    std::string str = SQL_FetchString(row, result, fieldname);
    if (!str.length() || !CheckInt(str)) {
        return -1;
    }

    // StrToInt uses an int for conversion, so we cannot use it.
	int64_t retval;
	sscanf(str.c_str(), "%" PRId64, &retval);
	return retval;
}

std::string SQL_FetchString(MYSQL_ROW row, MYSQL_RES* result, std::string fieldname)
{
    unsigned long* lengths = SQL_FetchLengths(result);
    unsigned long numfields = SQL_NumFields(result);
    MYSQL_FIELD* fields = SQL_FetchFields(result);
    for(unsigned long i = 0; i < numfields; i++)
    {
        if(fields[i].name == fieldname)
        {
            char* chstr = new char[lengths[i] + 1];
            memset(chstr, 0, lengths[i] + 1);
            memcpy(chstr, row[i], lengths[i]);
            std::string strfrom(chstr, lengths[i]);
            delete[] chstr;
            return strfrom;
        }
    }
    return "";
}

void SQL_Lock()
{
    //WaitForSingleObject(SQL::Mutex, INFINITE);
}

void SQL_Unlock()
{
    //ReleaseMutex(SQL::Mutex);
}

void SQL_DropTables()
{
    std::string query_table_logins = "DROP TABLE `logins`";
    std::string query_table_characters = "DROP TABLE `characters`";
    std::string query_table_shelf = "DROP TABLE `shelf`";

    Printf(LOG_Silent, "[DB] Warning: this action COULD NOT BE UNDONE!\n");
    PrintfT(LOG_Silent, "[DB] Query: \"%s\". Type \"yes\" to continue: ", query_table_logins.c_str());
    std::string in;
    std::cin >> in;
    in = ToLower(in);
    if(in == "y" || in == "ye" || (in.find("yes") == 0))
    {
        if(mysql_query(&SQL::Connection, query_table_logins.c_str()) != 0)
            Printf(LOG_Silent, "[DB] Warning: table `logins` NOT deleted.\n");
    }
    else Printf(LOG_Silent, "[DB] Table `logins` NOT deleted.\n");

    PrintfT(LOG_Silent, "[DB] Query: \"%s\". Type \"yes\" to continue: ", query_table_characters.c_str());
    std::cin >> in;
    in = ToLower(in);
    if(in == "y" || in == "ye" || (in.find("yes") == 0))
    {
        if(mysql_query(&SQL::Connection, query_table_characters.c_str()) != 0)
            Printf(LOG_Silent, "[DB] Warning: table `characters` NOT deleted.\n");
    }
    else Printf(LOG_Silent, "[DB] Table `characters` NOT deleted.\n");

    PrintfT(LOG_Silent, "[DB] Query: \"%s\". Type \"yes\" to continue: ", query_table_shelf.c_str());
    std::cin >> in;
    in = ToLower(in);
    if (in == "y" || in == "yes") {
        if(mysql_query(&SQL::Connection, query_table_shelf.c_str()) != 0) {
            Printf(LOG_Silent, "[DB] Warning: failed to delete table `shelf`: %s\n", SQL_Error().c_str());
        }
    } else {
        Printf(LOG_Silent, "[DB] Table `shelf` NOT deleted.\n");
    }
}

void SQL_CreateTableCheckpoint() {
    std::string create_table_checkpoint = R"(
        CREATE TABLE IF NOT EXISTS `checkpoint` (
            `id` BIGINT(1) NOT NULL AUTO_INCREMENT,
            `body` TINYINT(1) UNSIGNED NOT NULL,
            `reaction` TINYINT(1) UNSIGNED NOT NULL,
            `mind` TINYINT(1) UNSIGNED NOT NULL,
            `spirit` TINYINT(1) UNSIGNED NOT NULL,
            `monsters_kills` INT(1) UNSIGNED NOT NULL,
            `players_kills` INT(1) UNSIGNED NOT NULL,
            `frags` INT(1) UNSIGNED NOT NULL,
            `deaths` INT(1) UNSIGNED NOT NULL,
            `exp_fire_blade` INT(1) UNSIGNED NOT NULL,
            `exp_water_axe` INT(1) UNSIGNED NOT NULL,
            `exp_air_bludgeon` INT(1) UNSIGNED NOT NULL,
            `exp_earth_pike` INT(1) UNSIGNED NOT NULL,
            `exp_astral_shooting` INT(1) UNSIGNED NOT NULL,
            `dress` LONGTEXT CHARACTER SET cp866 COLLATE cp866_bin NOT NULL,
            UNIQUE(`id`)
        );
    )";
    if (mysql_query(&SQL::Connection, create_table_checkpoint.c_str()) != 0) {
        Printf(LOG_Silent, "[DB] Warning: table `checkpoint` not created: %s\n", SQL_Error().c_str());
    }
}

void SQL_FixShelves() {
    SimpleSQL cabinet1{"SELECT login_id, server_id FROM shelf WHERE cabinet = 1;"};
    for (int i = 0; i < SQL_NumRows(cabinet1.result); i++) {
        MYSQL_ROW row = SQL_FetchRow(cabinet1.result);

        uint32_t login_id = SQL_FetchInt(row, cabinet1.result, "login_id");
        uint32_t shelf_number = SQL_FetchInt(row, cabinet1.result, "server_id");

        CCharacter chr;
        chr.Nick = "@nick";
        chr.LoginID = login_id;

        int32_t mutex = 0;
        std::string items;
        int64_t money;
        bool shelf_exists;

        if (!shelf::PickFromShelf(chr, shelf_number, &mutex, &items, &money, shelf_exists)) {
            Printf(LOG_Error, "SQL_FixShelves: failed to load shelf for login_id %d and shelf_number %d\n", login_id, shelf_number);
        } else {
            chr.Nick = "nick";
            auto items_list = Login_UnserializeItems(items);
            if (!shelf::StoreOnShelf(chr, ServerIDType(shelf_number), std::move(items_list.Items), money)) {
                Printf(LOG_Error, "SQL_FixShelves: failed to save shelf for login_id %d and shelf_number %d\n", login_id, shelf_number);
            }
        }
    }
    
    SimpleSQL{"DELETE FROM shelf WHERE cabinet = 1;"};
}

void SQL_CreateTableTreasure() {
    std::string create_table_treasure = R"(
        CREATE TABLE IF NOT EXISTS treasure (
            server_id INT(1) NOT NULL COMMENT 'Server ID, 1--10',
            character_id BIGINT(1) NOT NULL COMMENT 'Character ID, same as characters.id',
            treasure_points INT(1) NOT NULL COMMENT 'Amount of treasure points',
            INDEX treasure_id_index (server_id, character_id)
        );
    )";
    if (mysql_query(&SQL::Connection, create_table_treasure.c_str()) != 0) {
        Printf(LOG_Silent, "[DB] Warning: table `treasure` not created: %s\n", SQL_Error().c_str());
    }
}

void SQL_CreateTables()
{
    std::string query_table_logins = "CREATE TABLE IF NOT EXISTS `logins` ( \
        `name` VARCHAR(256) NOT NULL, \
        `banned` TINYINT(1) NOT NULL, \
        `banned_date` BIGINT(1) UNSIGNED NOT NULL, \
        `banned_unbandate` BIGINT(1) UNSIGNED NOT NULL, \
        `banned_reason` TEXT CHARACTER SET cp866 COLLATE cp866_bin NOT NULL, \
        `muted` TINYINT(1) NOT NULL, \
        `muted_date` BIGINT(1) UNSIGNED NOT NULL, \
        `muted_unmutedate` BIGINT(1) UNSIGNED NOT NULL, \
        `muted_reason` TEXT CHARACTER SET cp866 COLLATE cp866_bin NOT NULL, \
        `locked` TINYINT(1) NOT NULL, \
        `locked_id1` INT(1) UNSIGNED NOT NULL, \
        `locked_id2` INT(1) UNSIGNED NOT NULL, \
        `locked_srvid` INT(1) NOT NULL, \
        `id` BIGINT(1) NOT NULL AUTO_INCREMENT, \
        `password` VARCHAR(256) NOT NULL, \
        `ip_filter` TEXT NOT NULL, \
        `locked_hat` INT(1) UNSIGNED NOT NULL, \
        `discordName` VARCHAR(255) DEFAULT NULL, \
        `discordUserId` VARCHAR(255) DEFAULT NULL, \
        `discordRole` VARCHAR(1020) DEFAULT NULL, \
        `changes` TINYINT(4) NOT NULL DEFAULT '0', \
        `allow_mage` TINYINT(1) NOT NULL DEFAULT '0', \
        `allow_female` TINYINT(1) NOT NULL DEFAULT '-1', \
        `alias_nickname` VARCHAR(50) DEFAULT NULL, \
        UNIQUE(`id`))";


    std::string query_table_characters = "CREATE TABLE IF NOT EXISTS `characters` ( \
        `id` BIGINT(1) NOT NULL AUTO_INCREMENT, \
        `login_id` BIGINT(1) NOT NULL, \
        `id1` INT(1) UNSIGNED NOT NULL, \
        `id2` INT(1) UNSIGNED NOT NULL, \
        `hat_id` INT(1) UNSIGNED NOT NULL, \
        `unknown_value_1` TINYINT(1) UNSIGNED NOT NULL, \
        `unknown_value_2` TINYINT(1) UNSIGNED NOT NULL, \
        `unknown_value_3` TINYINT(1) UNSIGNED NOT NULL, \
        `nick` VARCHAR(32) CHARACTER SET cp866 COLLATE cp866_bin NOT NULL, \
        `clan` VARCHAR(32) CHARACTER SET cp866 COLLATE cp866_bin NOT NULL, \
        `picture` TINYINT(1) UNSIGNED NOT NULL, \
        `body` TINYINT(1) UNSIGNED NOT NULL, \
        `reaction` TINYINT(1) UNSIGNED NOT NULL, \
        `mind` TINYINT(1) UNSIGNED NOT NULL, \
        `spirit` TINYINT(1) UNSIGNED NOT NULL, \
        `class` TINYINT(1) UNSIGNED NOT NULL, \
        `mainskill` TINYINT(1) UNSIGNED NOT NULL, \
        `flags` TINYINT(1) UNSIGNED NOT NULL, \
        `color` TINYINT(1) UNSIGNED NOT NULL, \
        `monsters_kills` INT(1) UNSIGNED NOT NULL, \
        `players_kills` INT(1) UNSIGNED NOT NULL, \
        `frags` INT(1) UNSIGNED NOT NULL, \
        `deaths` INT(1) UNSIGNED NOT NULL, \
        `money` INT(1) UNSIGNED NOT NULL, \
        `spells` INT(1) UNSIGNED NOT NULL, \
        `active_spell` INT(1) UNSIGNED NOT NULL, \
        `exp_fire_blade` INT(1) UNSIGNED NOT NULL, \
        `exp_water_axe` INT(1) UNSIGNED NOT NULL, \
        `exp_air_bludgeon` INT(1) UNSIGNED NOT NULL, \
        `exp_earth_pike` INT(1) UNSIGNED NOT NULL, \
        `exp_astral_shooting` INT(1) UNSIGNED NOT NULL, \
        `bag` LONGTEXT CHARACTER SET cp866 COLLATE cp866_bin NOT NULL, \
        `dress` LONGTEXT CHARACTER SET cp866 COLLATE cp866_bin NOT NULL, \
        `sec_55555555` MEDIUMBLOB NOT NULL, \
        `sec_40A40A40` MEDIUMBLOB NOT NULL, \
        `retarded` INT(1) UNSIGNED NOT NULL, \
        `deleted` INT(1) UNSIGNED NOT NULL, \
        `clantag` VARCHAR(16) CHARACTER SET cp866 COLLATE cp866_bin NOT NULL, \
        `ascended` INT(1) UNSIGNED NOT NULL, \
        `points` INT(1) UNSIGNED NOT NULL, \
        UNIQUE(`id`))"; // long query to create characters table

    std::string query_table_authlog = "CREATE TABLE IF NOT EXISTS `authlog` ( \
        `id` BIGINT(1) NOT NULL AUTO_INCREMENT, \
        `login_name` VARCHAR(256) NOT NULL, \
        `ip` VARCHAR(16) NOT NULL, \
        `uuid` VARCHAR(64) NOT NULL, \
        `date` TIMESTAMP NOT NULL, \
        UNIQUE(`id`))"; // long query to create authlog table

    if(mysql_query(&SQL::Connection, query_table_logins.c_str()) != 0)
        Printf(LOG_Silent, "[DB] Warning: table `logins` not created!\n");
    if(mysql_query(&SQL::Connection, query_table_characters.c_str()) != 0)
        Printf(LOG_Silent, "[DB] Warning: table `characters` not created!\n");
    if(mysql_query(&SQL::Connection, query_table_authlog.c_str()) != 0)
        Printf(LOG_Silent, "[DB] Warning: table `authlog` not created!\n");

    std::string create_table_shelf = R"(
        CREATE TABLE IF NOT EXISTS shelf (
            login_id BIGINT(1) NOT NULL COMMENT 'Player login ID, same as in logins',
            server_id INT(1) NOT NULL COMMENT 'Server ID, 1--7',
            cabinet INT(1) NOT NULL COMMENT 'Cabinet, 0 for regular characters, 1 for solo, 2 for solo-hardcore',
            mutex INT(1) COMMENT 'Used to detect concurrent modification. Always read this field for updates and increment by 1 when updating the row.',
            items LONGTEXT COMMENT 'Shelved items, in DB format for CItemList',
            money BIGINT(1) COMMENT 'Shelved money',
            INDEX shelf_id_index (login_id, server_id, cabinet)
        );
    )";
    if (mysql_query(&SQL::Connection, create_table_shelf.c_str()) != 0) {
        Printf(LOG_Silent, "[DB] Warning: table `shelf` not created: %s\n", SQL_Error().c_str());
    }

    SQL_CreateTableCheckpoint();
    SQL_CreateTableTreasure();
}

void SQL_UpdateVersion1() {
    std::string query_check = R"(
        SELECT COLUMN_NAME
        FROM INFORMATION_SCHEMA.COLUMNS
        WHERE TABLE_SCHEMA = ')" + Config::SqlDatabase + R"('
            AND TABLE_NAME = 'shelf'
            AND COLUMN_NAME = 'cabinet';
    )";

    if (SQL_Query(query_check) != 0) {
        Printf(LOG_Warning, "[DB] Warning: failed to check if shelf table needs to be updated to v1: %s\n", SQL_Error().c_str());
        return;
    }

    MYSQL_RES* result = SQL_StoreResult();
    if (!result) {
        Printf(LOG_Warning, "[DB] Warning: failed to check if shelf table needs to be updated to v1 (no result): %s\n", SQL_Error().c_str());
        return;
    }

    bool need_update = SQL_NumRows(result) == 0;

    SQL_FreeResult(result);

    if (!need_update) {
        return;
    }

    Printf(LOG_Info, "[DB]: updating shelf table to v1\n");

    const char* query;

    query = "ALTER TABLE shelf ADD COLUMN cabinet INT(1) NOT NULL COMMENT 'Cabinet, 0 for regular characters, 1 for solo, 2 for solo-hardcore'";
    if (mysql_query(&SQL::Connection, query) != 0) {
        Printf(LOG_Warning, "[DB] Warning: failed to update shelf table to v1: add column: %s\n", SQL_Error().c_str());
    }
    
    query = "UPDATE shelf SET cabinet = IF(server_id < 0, 1, 0), server_id = ABS(server_id)";
    if (mysql_query(&SQL::Connection, query) != 0) {
        Printf(LOG_Warning, "[DB] Warning: failed to update shelf table to v1: update: %s\n", SQL_Error().c_str());
    }
    
    query = "ALTER TABLE shelf DROP INDEX shelf_id_index";
    if (mysql_query(&SQL::Connection, query) != 0) {
        Printf(LOG_Warning, "[DB] Warning: failed to update shelf table to v1: drop index: %s\n", SQL_Error().c_str());
    }

    query = "ALTER TABLE shelf ADD INDEX shelf_id_index (login_id, server_id, cabinet)";
    if (mysql_query(&SQL::Connection, query) != 0) {
        Printf(LOG_Warning, "[DB] Warning: failed to update shelf table to v1: add index: %s\n", SQL_Error().c_str());
    }
}

void SQL_UpdateAllowFemale() {
    std::string query = "ALTER TABLE `logins` ADD COLUMN `allow_female` TINYINT(1) NOT NULL DEFAULT -1;";

    if (SQL_Query(query) != 0) {
        Printf(LOG_Warning, "[DB] Warning: failed to add `allow_female` column: %s\n", SQL_Error().c_str());
    }
}

#include "CCharacter.hpp"
#include "login.hpp"

bool SQL_RecoverCharacter(uint32_t login_id, CCharacter& chr)
{
    std::string chr_query_update;

    std::vector<uint8_t>& data_40A40A40 = chr.Section40A40A40.GetBuffer();
    std::vector<uint8_t>& data_55555555 = chr.Section55555555.GetBuffer();

    chr_query_update = Format("INSERT INTO `characters` ( \
                                    `login_id`, `id1`, `id2`, `hat_id`, \
                                    `unknown_value_1`, `unknown_value_2`, `unknown_value_3`, \
                                    `nick`, `clan`, \
                                    `picture`, `body`, `reaction`, `mind`, `spirit`, \
                                    `class`, `mainskill`, `flags`, `color`, \
                                    `monsters_kills`, `players_kills`, `frags`, `deaths`, \
                                    `spells`, `active_spell`, `money`, \
                                    `exp_fire_blade`, `exp_water_axe`, \
                                    `exp_air_bludgeon`, `exp_earth_pike`, \
                                    `exp_astral_shooting`, `bag`, `dress`, \
                                    `sec_55555555`, `sec_40A40A40`, `retarded`) VALUES ( \
                                        '%u', '%u', '%u', '%u', \
                                        '%u', '%u', '%u', \
                                        '%s', '%s', \
                                        '%u', '%u', '%u', '%u', '%u', \
                                        '%u', '%u', '%u', '%u', \
                                        '%u', '%u', '%u', '%u', \
                                        '%u', '%u', '%u', \
                                        '%u', '%u', '%u', '%u', '%u', \
                                        '%s', '%s', '",
                                            login_id, chr.Id1, chr.Id2, chr.HatId,
                                            chr.UnknownValue1, chr.UnknownValue2, chr.UnknownValue3,
                                            SQL_Escape(chr.Nick).c_str(), SQL_Escape(chr.Clan).c_str(),
                                            chr.Picture, chr.Body, chr.Reaction, chr.Mind, chr.Spirit,
                                            chr.Sex, chr.MainSkill, chr.Flags, chr.Color,
                                            chr.MonstersKills, chr.PlayersKills, chr.Frags, chr.Deaths,
                                            chr.Spells, chr.ActiveSpell, chr.Money,
                                            chr.ExpFireBlade, chr.ExpWaterAxe,
                                            chr.ExpAirBludgeon, chr.ExpEarthPike,
                                            chr.ExpAstralShooting,
                                            Login_SerializeItems(chr.Bag).c_str(), Login_SerializeItems(chr.Dress).c_str());

    for(size_t i = 0; i < data_55555555.size(); i++)
        chr_query_update += (char)data_55555555[i];

    chr_query_update += "', '";

    for(size_t i = 0; i < data_40A40A40.size(); i++)
        chr_query_update += (char)data_40A40A40[i];

    chr_query_update += "', '0')";

    return (SQL_Query(chr_query_update) == 0);
}

void SQL_UpdateTables()
{
    if(SQL_Query("RENAME TABLE `characters` TO `characters_old`") != 0)
    {
        Printf(LOG_Silent, "[DB] Error: %s\n", mysql_error(&SQL::Connection));
        Printf(LOG_Silent, "[DB] Error: table `characters` not updated!\n");
        return;
    }

    SQL_CreateTables();

    if(SQL_Query("SELECT * FROM `characters_old`") != 0)
    {
        Printf(LOG_Silent, "[DB] Error: table `characters` not updated!\n");
        return;
    }

    MYSQL_RES* result = SQL_StoreResult();
    if(!result)
    {
        Printf(LOG_Silent, "[DB] Error: table `characters` not updated!\n");
        return;
    }

    for(int i = 0; i < SQL_NumRows(result); i++)
    {
        MYSQL_ROW row = SQL_FetchRow(result);

        uint32_t login_id = SQL_FetchInt(row, result, "login_id");
        uint32_t id1 = SQL_FetchInt(row, result, "id1");
        uint32_t id2 = SQL_FetchInt(row, result, "id2");
        std::string data = SQL_FetchString(row, result, "data");

        if(data.size() == 0x30)
        {
            Printf(LOG_Silent, "[DB] Warning: retarded character %u:%u not recovered!\n", id1, id2);
            continue;
        }

        BinaryStream strm;
        strm.WriteFixedString(data, data.length());

        CCharacter chr;
        if(!chr.LoadFromStream(strm) || !SQL_RecoverCharacter(login_id, chr))
            Printf(LOG_Silent, "[DB] Warning: character %u:%u not recovered!\n", id1, id2);
    }
}

std::string SQL_Escape(std::string string)
{
    string = Trim(string);

    std::string output = "";
    for(std::string::iterator it = string.begin(); it != string.end(); ++it)
    {
        char ch = (*it);
        if(ch == '\\') output += "\\\\";
        else if(ch == '\'') output += "\\'";
        else output += ch;
    }

    return output;
}

int SQL_Query(std::string query)
{
    //Printf("SQL_Query: %s\n", query.c_str());
    if(!SQL::Open) return -1;

    mysql_query(&SQL::Connection, "SET NAMES 'cp866'");
    int result = mysql_real_query(&SQL::Connection, query.data(), query.length());
    if(result != 0 && Config::ReportDatabaseErrors) // error
        Printf(LOG_Error, "[DB] Error: %s\n", mysql_error(&SQL::Connection));

    return result;
}

int SQL_NumRows(MYSQL_RES* result)
{
    //Printf("SQL_NumRows()\n");
    return static_cast<int>(mysql_num_rows(result));
}

MYSQL_RES* SQL_StoreResult()
{
    //Printf("SQL_StoreResult()\n");
    return mysql_store_result(&SQL::Connection);
}

void SQL_FreeResult(MYSQL_RES* result)
{
    //Printf("SQL_FreeResult()\n");
    mysql_free_result(result);
}

MYSQL_ROW SQL_FetchRow(MYSQL_RES* result)
{
    //Printf("SQL_FetchRow()\n");
    return mysql_fetch_row(result);
}

int SQL_AffectedRows()
{
    //Printf("SQL_AffectedRows()\n");
    return static_cast<int>(mysql_affected_rows(&SQL::Connection));
}

unsigned long* SQL_FetchLengths(MYSQL_RES* result)
{
    //Printf("SQL_FetchLengths()\n");
    return mysql_fetch_lengths(result);
}

unsigned long SQL_NumFields(MYSQL_RES* result)
{
    //Printf("SQL_NumFields()\n");
    return mysql_num_fields(result);
}

MYSQL_FIELD* SQL_FetchFields(MYSQL_RES* result)
{
    //Printf("SQL_FetchFields()\n");
    return mysql_fetch_fields(result);
}

SimpleSQL::SimpleSQL(std::string query) {
    this->query = query;
    this->result = nullptr;
    this->success = true;

    if (SQL_Query(query) != 0) {
        Printf(LOG_Error, "[DB] Query failed: %s --- %s\n", query.c_str(), SQL_Error().c_str());
        this->success = false;
        return;
    }

    // Auto-detect SELECT statements. Non-select statements fail the `SQL_StoreResult` call.
    if (query.find("SELECT") != query.find_first_not_of(' ')) {
        return;
    }

    this->result = SQL_StoreResult();
    if (!this->result) {
        Printf(LOG_Error, "[DB] Query failed to store result: %s --- %s\n", query.c_str(), SQL_Error().c_str());
        this->success = false;
        return;
    }
}

SimpleSQL::~SimpleSQL() {
    if (this->result) {
        SQL_FreeResult(this->result);
    }
}
