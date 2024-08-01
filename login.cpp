#include "login.hpp"
#include "sql.hpp"
#include "utils.hpp"
#include "config.hpp"

#include "sha1.h"

#include <windows.h>

std::string Login_MakePassword(std::string password)
{
    //Printf("Login_MakePassword()\n");
    if(!Config::UseSHA1) return password;
    static unsigned char hash[20];
    static char hexstring[41];
    hexstring[40] = 0;
    sha1::calc(password.c_str(), password.length(), hash);
    sha1::toHexString(hash, hexstring);
    return std::string(hexstring);
}

bool Login_UnlockOne(std::string login)
{
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login);

        std::string query_unlockone = Format("UPDATE `logins` SET `locked_hat`='0' WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_unlockone.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        SQL_Unlock();
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }

    return true;
}

bool Login_UnlockAll()
{
    if(!SQL_CheckConnected()) return false;

    try
    {
        std::string query_unlockall = "UPDATE `logins` SET `locked_hat`='0'";
        if(SQL_Query(query_unlockall.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        SQL_Unlock();
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }

    return true;
}

bool Login_Exists(std::string login)
{
    //Printf("Login_Exists()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_checklgn = Format("SELECT `name` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        int rows = SQL_NumRows(result);
        SQL_FreeResult(result);

        SQL_Unlock();

        return (rows);
    }
    catch(...)
    {
        SQL_Unlock();
        return true;
    }
}

bool Login_Create(std::string login, std::string password)
{
    //Printf("Login_Create()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        if(Login_Exists(login)) return false;
        login = SQL_Escape(login);

        std::string pass_new = SQL_Escape(Login_MakePassword(password));

        std::string query_createlgn = Format("INSERT INTO `logins` (`ip_filter`, `name`, `banned`, `banned_date`, `banned_unbandate`, `banned_reason`, `locked`, `locked_id1`, `locked_id2`, `locked_srvid`, `password`) VALUES \
                                            ('', '%s', '0', '0', '0', '', '0', '0', '0', '0', '%s')", login.c_str(), pass_new.c_str());

        if(SQL_Query(query_createlgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        int rows = SQL_AffectedRows();
        SQL_Unlock();

        if(rows != 1) return false; // sql error

        return true; // login created
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
}

bool Login_Delete(std::string login)
{
    //Printf("Login_Delete()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_checklgn = Format("SELECT `id` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_Unlock();
            SQL_FreeResult(result);
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result);
        int login_id = SQL_FetchInt(row, result, "id");
        SQL_FreeResult(result);

        std::string query_deletelgn = Format("DELETE FROM `logins` WHERE LOWER(`name`)=LOWER('%s') AND `id`='%u'", login.c_str(), login_id);
        if(SQL_Query(query_deletelgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        if(SQL_AffectedRows() != 1)
        {
            SQL_Unlock();
            return false;
        }

        if(login_id == -1)
        {
            SQL_Unlock();
            return true; // тут спорно, возвращать false или true. с одной стороны, логин таки удалЄн,
                         // с другой - запись в базе €вно неправильна€.
        }

        std::string query_deletecharacters = Format("DELETE FROM `characters` WHERE `login_id`='%d'", login_id);
        if(SQL_Query(query_deletecharacters.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        SQL_Unlock();
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
    return true;
}

bool Login_GetPassword(std::string login, std::string& password)
{
    //Printf("Login_GetPassword()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        if(!Login_Exists(login)) return false;
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_checklgn = Format("SELECT `password` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_Unlock();
            SQL_FreeResult(result);
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result);
        password = SQL_FetchString(row, result, "password");
        SQL_FreeResult(result);

        SQL_Unlock();
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
    return true;
}

bool Login_SetPassword(std::string login, std::string password)
{
    //Printf("Login_SetPassword()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        if(!Login_Exists(login)) return false;
        login = SQL_Escape(login);

        SQL_Lock();
        std::string pass_new = Login_MakePassword(password);
        std::string query_setpasswd = Format("UPDATE `logins` SET `password`='%s' WHERE LOWER(`name`)=LOWER('%s')", pass_new.c_str(), login.c_str());
        if(SQL_Query(query_setpasswd.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        if(SQL_AffectedRows() != 1)
        {
            SQL_Unlock();
            return false;
        }

        SQL_Unlock();
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }

    return true;
}

bool Login_SetLocked(std::string login, bool locked_hat, bool locked, unsigned long id1, unsigned long id2, unsigned long srvid)
{
    //Printf("Login_SetLocked()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        if(!Login_Exists(login)) return false;
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_setlockd = Format("UPDATE `logins` SET `locked_hat`='%u', `locked`='%u', `locked_id1`='%u', `locked_id2`='%u', `locked_srvid`='%u' WHERE LOWER(`name`)=LOWER('%s')",
                                                (unsigned int)locked_hat, (unsigned int)locked, id1, id2, srvid, login.c_str());
        if(SQL_Query(query_setlockd.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        /*
        if(SQL_AffectedRows() != 1)
        {
            SQL_Unlock();
            return false;
        }
        */

        SQL_Unlock();
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
    return true;
}

bool Login_GetLocked(std::string login, bool& locked_hat, bool& locked, unsigned long& id1, unsigned long& id2, unsigned long& srvid)
{
    //Printf("Login_GetLocked()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_checklgn = Format("SELECT `locked_hat`, `locked`, `locked_id1`, `locked_id2`, `locked_srvid` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_Unlock();
            SQL_FreeResult(result);
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result);
        locked_hat = (SQL_FetchInt(row, result, "locked_hat"));
        locked = (SQL_FetchInt(row, result, "locked"));
        id1 = SQL_FetchInt(row, result, "locked_id1");
        id2 = SQL_FetchInt(row, result, "locked_id2");
        srvid = SQL_FetchInt(row, result, "locked_srvid");
        SQL_FreeResult(result);
        SQL_Unlock();

        return true;
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
}

bool Login_SetBanned(std::string login, bool banned, unsigned long date_ban, unsigned long date_unban, std::string reason)
{
    //Printf("Login_SetBanned()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        if(!Login_Exists(login)) return false;
        login = SQL_Escape(login);
        reason = SQL_Escape(reason);

        SQL_Lock();

        std::string query_setlockd = Format("UPDATE `logins` SET `banned`='%u', `banned_date`='%u', `banned_unbandate`='%u', `banned_reason`='%s' WHERE LOWER(`name`)=LOWER('%s')",
                                                (unsigned int)banned, date_ban, date_unban, reason.c_str(), login.c_str());
        if(SQL_Query(query_setlockd.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        if(SQL_AffectedRows() != 1)
        {
            SQL_Unlock();
            return false;
        }

        SQL_Unlock();
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
    return true;
}

bool Login_GetBanned(std::string login, bool& banned, unsigned long& date_ban, unsigned long& date_unban, std::string& reason)
{
    //Printf("Login_GetBanned()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_checklgn = Format("SELECT `id`, `banned`, `banned_date`, `banned_unbandate`, `banned_reason` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_Unlock();
            SQL_FreeResult(result);
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result);
        banned = (SQL_FetchInt(row, result, "banned"));
        date_ban = SQL_FetchInt(row, result, "banned_date");
        date_unban = SQL_FetchInt(row, result, "banned_unbandate");
        reason = SQL_FetchString(row, result, "banned_reason");
        SQL_FreeResult(result);
        SQL_Unlock();

        return true;
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
}

bool Login_SetMuted(std::string login, bool muted, unsigned long date_mute, unsigned long date_unmute, std::string reason)
{
    //Printf("Login_SetMuted()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        if(!Login_Exists(login)) return false;
        login = SQL_Escape(login);
        reason = SQL_Escape(reason);

        SQL_Lock();

        std::string query_setlockd = Format("UPDATE `logins` SET `muted`='%u', `muted_date`='%u', `muted_unmutedate`='%u', `muted_reason`='%s' WHERE LOWER(`name`)=LOWER('%s')",
                                                (unsigned int)muted, date_mute, date_unmute, reason.c_str(), login.c_str());
        if(SQL_Query(query_setlockd.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        if(SQL_AffectedRows() != 1)
        {
            SQL_Unlock();
            return false;
        }

        SQL_Unlock();
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
    return true;
}

bool Login_GetMuted(std::string login, bool& muted, unsigned long& date_mute, unsigned long& date_unmute, std::string& reason)
{
    //Printf("Login_GetMuted()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_checklgn = Format("SELECT `id`, `muted`, `muted_date`, `muted_unmutedate`, `muted_reason` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_Unlock();
            SQL_FreeResult(result);
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result);
        muted = (SQL_FetchInt(row, result, "muted"));
        date_mute = SQL_FetchInt(row, result, "muted_date");
        date_unmute = SQL_FetchInt(row, result, "muted_unmutedate");
        reason = SQL_FetchString(row, result, "muted_reason");
        SQL_FreeResult(result);
        SQL_Unlock();

        return true;
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
}

#include "CCharacter.hpp"

std::string Login_SerializeItems(CItemList& list)
{
    std::string out;
    out = Format("[%u,%u,%u,%u]", list.UnknownValue0, list.UnknownValue1, list.UnknownValue2, list.Items.size());
    for(std::vector<CItem>::iterator it = list.Items.begin(); it != list.Items.end(); ++it)
    {
        CItem& item = (*it);
        out += Format(";[%u,%u,%u,%u", item.Id, (uint32_t)item.IsMagic, item.Price, item.Count);
        if(item.IsMagic)
        {
            for(std::vector<CEffect>::iterator jt = item.Effects.begin(); jt != item.Effects.end(); ++jt)
            {
                CEffect& effect = (*jt);
                out += Format(",{%u:%u:%u:%u}", effect.Id1, effect.Value1, effect.Id2, effect.Value2);
            }
        }
        out += "]";
    }
    return SQL_Escape(out);
}

CItemList Login_UnserializeItems(std::string list)
{
    CItemList items;
    items.UnknownValue0 = 0;
    items.UnknownValue1 = 0;
    items.UnknownValue2 = 0;

    std::vector<std::string> f_str = Explode(list, ";");
    std::string of_listdata = Trim(f_str[0]);
    if(of_listdata[0] != '[' || of_listdata[of_listdata.length()-1] != ']') return items;
    of_listdata.erase(0, 1);
    of_listdata.erase(of_listdata.length()-1, 1);
    std::vector<std::string> f_listdata = Explode(of_listdata, ",");
    if(f_listdata.size() != 4) return items;

    items.UnknownValue0 = StrToInt(Trim(f_listdata[0]));
    items.UnknownValue1 = StrToInt(Trim(f_listdata[1]));
    items.UnknownValue2 = StrToInt(Trim(f_listdata[2]));
    items.Items.resize(StrToInt(Trim(f_listdata[3])));

    f_str.erase(f_str.begin());

    for(std::vector<CItem>::iterator it = items.Items.begin(); it != items.Items.end(); ++it)
    {
        CItem& item = (*it);
        item.Id = 0;
        item.Count = 1;
        item.IsMagic = false;
        item.Price = 0;
        item.Effects.clear();

        if(!f_str.size()) return items;
        std::string of_itemdata = Trim(f_str[0]);
        if(of_itemdata[0] != '[' || of_itemdata[of_itemdata.length()-1] != ']') return items;
        of_itemdata.erase(0, 1);
        of_itemdata.erase(of_itemdata.length()-1, 1);
        std::vector<std::string> f_itemdata = Explode(of_itemdata, ",");
        if(f_itemdata.size() < 4) return items;
        item.Id = StrToInt(Trim(f_itemdata[0]));
        item.IsMagic = (StrToInt(Trim(f_itemdata[1])));
        item.Price = StrToInt(Trim(f_itemdata[2]));
        item.Count = StrToInt(Trim(f_itemdata[3]));
        if(item.IsMagic)
        {
            for(size_t i = 4; i < f_itemdata.size(); i++)
            {
                std::string of_efdata = f_itemdata[i];
                if(of_efdata[0] != '{' || of_efdata[of_efdata.length()-1] != '}') return items;
                of_efdata.erase(0, 1);
                of_efdata.erase(of_efdata.length()-1, 1);
                std::vector<std::string> f_efdata = Explode(of_efdata, ":");
                if(f_efdata.size() != 4) return items;
                CEffect effect;
                effect.Id1 = StrToInt(Trim(f_efdata[0]));
                effect.Value1 = StrToInt(Trim(f_efdata[1]));
                effect.Id2 = StrToInt(Trim(f_efdata[2]));
                effect.Value2 = StrToInt(Trim(f_efdata[3]));
                item.Effects.push_back(effect);
            }
        }
        f_str.erase(f_str.begin());
    }
    return items;
}

bool Login_SetCharacter(std::string login, unsigned long id1, unsigned long id2, unsigned long size, char* data, std::string nickname, unsigned long srvid)
{
    //Printf("Login_SetCharacter()\n");
    if(!SQL_CheckConnected()) return false; // Check if SQL connection is active

    try
    {
        login = SQL_Escape(login); // Escape SQL characters in login string
        nickname = SQL_Escape(nickname); // Escape SQL characters in nickname string

        SQL_Lock(); // Lock SQL to prevent concurrent access

        // Query to check if login exists
        std::string query_checklgn = Format("SELECT `id` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0) // Execute query
        {
            SQL_Unlock(); // Unlock SQL if query fails
            return false;
        }

        // Store result of query
        MYSQL_RES* result = SQL_StoreResult();

        // Check if result is valid
        if(!result)
        {
            SQL_Unlock(); // Unlock SQL if no result
            return false;
        }

        if(!SQL_NumRows(result)) // Check if login exists in result
        {
            SQL_Unlock();
            SQL_FreeResult(result); // Free result memory
            return false; // login does not exist
        }

        // Fetch row from result
        MYSQL_ROW row = SQL_FetchRow(result);
        // Get login id from row
        int login_id = SQL_FetchInt(row, result, "id");
        SQL_FreeResult(result); // Free result memory

        // Check if login id is valid
        if(login_id == -1)
        {
            SQL_Unlock();
            return false;
        }

        // Query to check if character exists
        std::string query_checkchr = Format("SELECT `login_id` FROM `characters` WHERE `login_id`='%d' AND `id1`='%u' AND `id2`='%u'", login_id, id1, id2);
        if(SQL_Query(query_checkchr.c_str()) != 0) // Execute query
        {
            SQL_Unlock();
            return false;
        }

        // FLAG to determine if character needs to be created
        bool create = true;

        result = SQL_StoreResult(); // Store result of query
        if(result)
        {
            // If character exists, set create FLAG to false
            if(SQL_NumRows(result) == 1) create = false;
            SQL_FreeResult(result); // Free result memory
        }

        // RETARDED CHARACTER
        if(size == 0x30 && *(unsigned long*)(data) == 0xFFDDAA11)
        {
            // Extract character attributes from data
            uint8_t p_nickname_length = *(uint8_t*)(data + 4);
            uint8_t p_body = *(uint8_t*)(data + 5);
            uint8_t p_reaction = *(uint8_t*)(data + 6);
            uint8_t p_mind = *(uint8_t*)(data + 7);
            uint8_t p_spirit = *(uint8_t*)(data + 8);
            uint8_t p_base = *(uint8_t*)(data + 9);
            uint8_t p_picture = *(uint8_t*)(data + 10);
            uint8_t p_sex = *(uint8_t*)(data + 11);
            uint32_t p_id1 = *(uint32_t*)(data + 12);
            uint32_t p_id2 = *(uint32_t*)(data + 16);
            std::string p_nickname(data+20, p_nickname_length);
            std::string p_nick, p_clan;

            // Check for clan separator in nickname
            size_t splw = p_nickname.find('|');
            if(splw != std::string::npos)
            {
                p_nick = p_nickname;
                p_clan = p_nickname;
                p_nick.erase(splw); // Separate nickname
                p_clan.erase(0, splw+1); // Separate clan
            }
            else
            {
                p_nick = p_nickname;
                p_clan = "";
            }

            // SQL query for creating or updating a character
            std::string chr_query_create1;

            // if create FLAG is true - create a new character
            if(create)
            {
                chr_query_create1 = Format("INSERT INTO `characters` (`login_id`, `retarded`, `body`, `reaction`, `mind`, `spirit`, \
                                           `mainskill`, `picture`, `class`, `id1`, `id2`, `nick`, `clan`, `clantag`, `deleted`) VALUES \
                                           ('%u', '1', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%s', '%s', '', '0')", login_id,
                                                p_body, p_reaction, p_mind, p_spirit, p_base, p_picture, p_sex, p_id1, p_id2,
                                                p_nick.c_str(), p_clan.c_str());
            }
            // create FLAG is false - update an existing character
            else
            {
                chr_query_create1 = Format("UPDATE `characters` SET `login_id`='%u', `retarded`='1', `body`='%u', `reaction`='%u', `mind`='%u', `spirit`='%u', \
                                           `mainskill`='%u', `picture`='%u', `class`='%u', `id1`='%u', `id2`='%u', `nick`='%s', `clan`='%s', `clantag`='', `deleted`='0'", login_id,
                                                p_body, p_reaction, p_mind, p_spirit, p_base, p_picture, p_sex, p_id1, p_id2,
                                                p_nick.c_str(), p_clan.c_str());
            }

            if(SQL_Query(chr_query_create1) != 0) // Execute character creation/update query
            {
                SQL_Unlock();
                return false;
            }
        }
        // REGULAR CHARACTER
        else
        {
            BinaryStream strm; // Stream for handling binary data
            std::vector<uint8_t>& buf = strm.GetBuffer();
            for(size_t i = 0; i < size; i++)
                buf.push_back(data[i]); // Fill buffer with data

            strm.Seek(0);
            CCharacter chr;
            if(!chr.LoadFromStream(strm)) // Load character data from stream
            {
                SQL_Unlock();
                return false;
            }

            if(chr.Clan.size() > 0) // Process clan data
            {
                int cpos = chr.Clan.find("[");
                if(cpos >= 0)
                {
                    if(cpos > 0 && chr.Clan[cpos-1] == '|')
                        cpos--;
                    chr.Clan = std::string(chr.Clan);
                    chr.Clan.erase(cpos);
                }
            }

            // SQL query for updating a character
            std::string chr_query_update;

            // Extract sections from character data
            std::vector<uint8_t>& data_40A40A40 = chr.Section40A40A40.GetBuffer();
            std::vector<uint8_t>& data_55555555 = chr.Section55555555.GetBuffer();

            // if create FLAG is true - create a new character
            if(create)
            {
                // Query to insert character with all attributes
                // (((NOTE: ascended field is not at the server. It's DB-only field so we can update it only
                // when we receive character from the server (down below))). There we just init it as 0)
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
                                                `exp_astral_shooting`, `bag`, `dress`, `clantag`, \
                                                `sec_55555555`, `sec_40A40A40`, `retarded`, `deleted`, \
                                                `ascended`) VALUES ( \
                                                    '%u', '%u', '%u', '%u', \
                                                    '%u', '%u', '%u', \
                                                    '%s', '%s', \
                                                    '%u', '%u', '%u', '%u', '%u', \
                                                    '%u', '%u', '%u', '%u', \
                                                    '%u', '%u', '%u', '%u', \
                                                    '%u', '%u', '%u', \
                                                    '%u', '%u', '%u', '%u', '%u', \
                                                    '%s', '%s', '%s', '",
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
                                                        Login_SerializeItems(chr.Bag).c_str(),
                                                        Login_SerializeItems(chr.Dress).c_str(),
                                                        SQL_Escape(chr.ClanTag).c_str());


                // Append section data to query
                for(size_t i = 0; i < data_55555555.size(); i++)
                {
                    char ch = (char)data_55555555[i];
                    if(ch == '\\' || ch == '\'') chr_query_update += '\\';
                    chr_query_update += ch;
                }

                chr_query_update += "', '";

                for(size_t i = 0; i < data_40A40A40.size(); i++)
                {
                    char ch = (char)data_40A40A40[i];
                    if(ch == '\\' || ch == '\'') chr_query_update += '\\';
                    chr_query_update += ch;
                }

                chr_query_update += "', '0', '0')";
            }
            // create FLAG is false - update an existing character
            else
            {
                // Reset character attributes for #1 server (remove 1000 starting gold)
                if (srvid == 1)
                {
                    chr.Money = 0;
                    chr.MonstersKills = 0;
                    chr.Deaths = 0;
                    chr.ExpFireBlade = 1;
                    chr.ExpWaterAxe = 0;
                    chr.ExpAirBludgeon = 0;
                    chr.ExpEarthPike = 0;
                    chr.ExpAstralShooting = 0;
                    // wipe bag
                    std::string serializedBag = "[0,0,0,0]";
                    chr.Bag = Login_UnserializeItems(serializedBag);
                    // wipe equipped
                    std::string serializedDress = "[0,0,40,12];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";
                    chr.Dress = Login_UnserializeItems(serializedDress);

                    if (chr.Sex == 64 || chr.Sex == 192) // mage
                    {
                        // give attack spell
                        switch (chr.MainSkill) {
                            case 1: chr.Spells = 16777218; break; // fire
                            case 2: chr.Spells = 16777248; break; // water
                            case 3: chr.Spells = 16778240; break; // air
                            case 4: chr.Spells = 16842752; break; // earth
                        }
                    }

                    if (chr.Sex == 0) { // warr
                    /////// NOTE: if change avatar number there - don't forget to change
                    /////// at rom2.ru code
                        chr.Picture = 64; // zombie
                    }
                    else if (chr.Sex == 64) { // mage
                        chr.Picture = 64; // zombie
                    }
                    else if (chr.Sex == 128) { // ama become warr
                        chr.Sex = 0;
                        chr.Picture = 64; // zombie
                    }
                    else if (chr.Sex == 192) { // witch become mage
                        chr.Sex = 64;
                        chr.Picture = 64; // zombie
                    }
                }

                ////////////
                // REBORN //
                //////////// before entering next difficulty

                ////////////////////////////////////////////
                // 1) check for "Boss key" - potion
                bool foundBossKey = false;

                const unsigned long bossKeyItem = 3667;  // "Quest Treasure".

                for (const auto &item: chr.Bag.Items) {
                    if (item.Id == bossKeyItem) {
                        foundBossKey = true;
                        break;
                    }
                }

                if (foundBossKey) {
                    // Remove all quest treasures from the player's bag.
                    std::vector<CItem> newItems;
                    newItems.reserve(chr.Bag.Items.size());

                    for (const auto& item: chr.Bag.Items) {
                        if (item.Id != bossKeyItem) {
                            newItems.push_back(item);
                        }
                    }

                    chr.Bag.Items = newItems;
                }

                ////////////////////////////////////////////
                // 2) now check for reborn

                bool reborn = false;
                bool meets_reborn_criteria = foundBossKey;
                unsigned int total_exp = chr.ExpFireBlade + chr.ExpWaterAxe + chr.ExpAirBludgeon +
                         chr.ExpEarthPike + chr.ExpAstralShooting;

                // fix problem when 0-exp character (eg due nullifing) stuck in limbo
                if (total_exp == 0) {
                    chr.ExpFireBlade = 1;
                }

                // note: we check there _!from which server!_ we received character
                // eg if we received char from srvid 2 and char finished its stay there
                // (by drinking mind to 15) - he will be reborned.. and his next login
                // to srvid 3 (as he can't enter 2 anymore) will be ab ovo
                if ((srvid == 2 && chr.Mind > 14) ||
                    (srvid == 3 && chr.Reaction > 19) ||
                    (srvid == 4 && chr.Reaction > 29) ||
                    (srvid == 5 && chr.Reaction > 39) ||
                    (srvid == 6 && chr.Reaction > 49))
                {
                    reborn = true;

                    // As we receive character from server - we can control, should it
                    // go to next lvl or not; so we can revert its stats back if reqs
                    // not satisfied. so...

                    // pay for the ticket (for !normal! characters too)
                    if (srvid == 2 && chr.Money < 30000) {
                        meets_reborn_criteria = false;
                    }
                    else if (srvid == 3 && chr.Money < 300000) {
                        meets_reborn_criteria = false;
                    }
                    else if (srvid == 4 && chr.Money < 1500000) {
                        meets_reborn_criteria = false;
                    }
                    else if (srvid == 5 && chr.Money < 10000000) {
                        meets_reborn_criteria = false;
                    }
                    else if (srvid == 6 && chr.Money < 50000000) {
                        meets_reborn_criteria = false;
                    }

                    // ..Revert stats for AMA/WITCH if exp is lower than
                    if (chr.Sex == 128 || chr.Sex == 192) {
                        if (srvid == 2 && (total_exp < 100000 || chr.MonstersKills < 1000 ||
                                 chr.Money < 100000)) {
                            meets_reborn_criteria = false;
                        }
                        else if (srvid == 3 && (total_exp < 500000 || chr.MonstersKills < 1200 ||
                                 chr.Money < 1000000)) {
                            meets_reborn_criteria = false;
                        }
                        else if (srvid == 4 && (total_exp < 2000000 || chr.MonstersKills < 1500 ||
                                 chr.Money < 5000000)) {
                            meets_reborn_criteria = false;
                        }
                        else if (srvid == 5 && (total_exp < 30000000 || chr.MonstersKills < 2000 ||
                                 chr.Money < 30000000)) {
                            meets_reborn_criteria = false;
                        }
                        else if (srvid == 6 && (total_exp < 100000000 || chr.MonstersKills < 4000 ||
                                 chr.Money < 100000000)) {
                            meets_reborn_criteria = false;
                        }
                    // ..also have min.exp for Hardcore chars (0 or 1 death)
                    } else if (chr.Deaths <= 1) {
                        if (srvid == 2 && total_exp < 50000) {
                            meets_reborn_criteria = false;
                        }
                        else if (srvid == 3 && total_exp < 100000) {
                            meets_reborn_criteria = false;
                        }
                        else if (srvid == 4 && total_exp < 500000) {
                            meets_reborn_criteria = false;
                        }
                        else if (srvid == 5 && total_exp < 2000000) {
                            meets_reborn_criteria = false;
                        }
                        else if (srvid == 6 && total_exp < 25000000) {
                            meets_reborn_criteria = false;
                        }
                    }
                }

                // The player wanted to do a reborn, but doesn't meet criteria: revert the stats, so the player is left on the same server.
                if (reborn && !meets_reborn_criteria) {
                    reborn = false;

                    uint8_t stat_ceiling = 0;

                    switch (srvid) {
                    case 2:
                        stat_ceiling = 14;
                        break;
                    case 3:
                        stat_ceiling = 19;
                        break;
                    case 4:
                        stat_ceiling = 29;
                        break;
                    case 5:
                        stat_ceiling = 39;
                        break;
                    case 6:
                        stat_ceiling = 49;
                        break;
                    }

                    // Make stats at most "max-1".
                    chr.Reaction = std::min(chr.Reaction, stat_ceiling);

                    // We don't need to touch other stats on #6 as all stats are acquired independently there.
                    if (srvid != 6) {
                        chr.Body = std::min(chr.Body, stat_ceiling);
                        chr.Mind = std::min(chr.Mind, stat_ceiling);
                        chr.Spirit = std::min(chr.Spirit, stat_ceiling);
                    }
                }

                if (reborn) {
                    // WARRIOR/MAGE (no reclass OR ascend)
                    if (chr.Sex == 0 || chr.Sex == 64) {
                        chr.Money = 0; // wipe gold
                        std::string serializedBag = "[0,0,0,0]";
                        chr.Bag = Login_UnserializeItems(serializedBag); // wipe bag

                        // Wipe experience for the main skill
                        switch (chr.MainSkill) {
                            case 1: chr.ExpFireBlade = 1; break;
                            case 2: chr.ExpWaterAxe = 1; break;
                            case 3: chr.ExpAirBludgeon = 1; break;
                            case 4: chr.ExpEarthPike = 1; break;
                        }

                        // Reduce all other skills in 2 times...
                        if (chr.MainSkill != 1) chr.ExpFireBlade /= 2;
                        if (chr.MainSkill != 2) chr.ExpWaterAxe /= 2;
                        if (chr.MainSkill != 3) chr.ExpAirBludgeon /= 2;
                        if (chr.MainSkill != 4) chr.ExpEarthPike /= 2;
                        // ... WARRIOR: astral/shooting in srvID times
                        // (hardcore character only 2x times)
                        if (chr.Deaths == 0) {
                            chr.ExpAstralShooting /= 2;
                        } else {
                            chr.ExpAstralShooting /= srvid;
                        }

                        // Mage
                        if (chr.Sex == 64) {
                            chr.ExpAstralShooting = 1; // MAGE wipe astral skill

                            // mages lose equipped items
                            std::string serializedDress = "[0,0,40,12];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";
                            chr.Dress = Login_UnserializeItems(serializedDress);

                            // ...and all spellbooks (leave only basic arrow)
                            // (if not hardcore)
                            if (chr.Deaths > 0) {
                                switch (chr.MainSkill) {
                                    case 1: chr.Spells = 16777218; break; // fire
                                    case 2: chr.Spells = 16777248; break; // water
                                    case 3: chr.Spells = 16778240; break; // air
                                    case 4: chr.Spells = 16842752; break; // earth
                                }
                            }
                        }
                    }
                    // RECLASSED chars reborn (AMA/WITCH)
                    else if (chr.Sex == 128 || chr.Sex == 192) {
                        chr.MonstersKills = 0; // reset monster kills for reborn restriction
                        chr.Money = 0; // wipe gold
                        std::string serializedBag = "[0,0,0,0]";
                        chr.Bag = Login_UnserializeItems(serializedBag); // wipe bag

                        chr.ExpFireBlade = 1; // wipe ALL exp
                        chr.ExpWaterAxe = 1;
                        chr.ExpAirBludgeon = 1;
                        chr.ExpEarthPike = 1;
                        chr.ExpAstralShooting = 1;

                        // Witch
                        if (chr.Sex == 192) {
                            // witch lose equipped items
                            std::string serializedDress = "[0,0,40,12];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";
                            chr.Dress = Login_UnserializeItems(serializedDress);

                            // ...and all spellbooks (leave only basic arrow)
                            switch (chr.MainSkill) {
                                case 1: chr.Spells = 16777218; break; // fire
                                case 2: chr.Spells = 16777248; break; // water
                                case 3: chr.Spells = 16778240; break; // air
                                case 4: chr.Spells = 16842752; break; // earth
                            }
                        }
                    }
                }

                //////////////////////////////
                // RECLASS after maxing EXP //
                //////////////////////////////
                unsigned int ascended = 0; // DB-only flag to ladder score
                unsigned int stats_sum = chr.Body + chr.Reaction + chr.Mind + chr.Spirit;

                // RECLASS: warrior/mage become ama/witch
                // (note it can't happen simultaneously with reborn as
                // at reborn we "half" the exp)
                if ((chr.Sex == 0 || chr.Sex == 64) && chr.Clan == "reclass" && total_exp > 177777777) {

                    chr.MonstersKills = 0; // reset monster kills (we need it for reborn restrictions)
                    chr.Money = 0; // Reset Money
                    chr.Body = 1; // stats
                    chr.Reaction = 1;
                    chr.Mind = 1;
                    chr.Spirit = 1;
                    chr.ExpFireBlade = 1; // exp
                    chr.ExpWaterAxe = 1;
                    chr.ExpAirBludgeon = 1;
                    chr.ExpEarthPike = 1;
                    chr.ExpAstralShooting = 1;
                    // wipe bag
                    std::string serializedBag = "[0,0,0,0]";
                    chr.Bag = Login_UnserializeItems(serializedBag);
                    // wipe equipped
                    std::string serializedDress = "[0,0,40,12];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";
                    chr.Dress = Login_UnserializeItems(serializedDress);

                    // reclass: war/mage change class
                    if (chr.Sex == 0) { // warr become ama
                        chr.Sex = 128;
                        chr.Picture = 11; // and become human
                    }
                    else if (chr.Sex == 64) { // mage becomes witch
                        chr.Sex = 192;
                        chr.Picture = 6; // and become human
                    }

                ///////////////////////////////
                // ASCEND after maxing STATS //
                ///////////////////////////////

                // ASCEND: ama/witch become again war/mage and receive crown
                } else if ((chr.Sex == 128 || chr.Sex == 192) && chr.Clan == "ascend" &&
                            stats_sum == 284 && total_exp > 177777777) {

                    // increment ascended DB-only field to mark that character was ascended (for ladder score)
                    ascended = 1;

                    // Loose some stats as price for ascend,
                    // but still save some be able stay on #7;
                    // otherwise (eg if we reset stats to 1)...
                    // ...reborn will cause mage staff to dissapear
                    chr.Body = 50;
                    chr.Reaction = 50;
                    chr.Mind = 50;
                    chr.Spirit = 50;

                    chr.ExpFireBlade = 1; // pay with exp too
                    chr.ExpWaterAxe = 1;
                    chr.ExpAirBludgeon = 1;
                    chr.ExpEarthPike = 1;
                    chr.ExpAstralShooting = 1;

                    // (we do not wipe inventory/gold at this point...
                    // ..as players anyway will save items on mule, so why to make hastle.
                    // So we wipe only equipment to be able to award player with crown/staff
                    if (chr.Sex == 128) { // amazon become warrior and get CROWN (Good Gold Helm) +3 body +2 scanRange
                        chr.Sex = 0;
                        chr.Picture = 32;
                        std::string serializedDress = "[0,0,40,12];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[18118,1,2,1,{2:3:0:0},{19:2:0:0}];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";
                        chr.Dress = Login_UnserializeItems(serializedDress);
                    } else if (chr.Sex == 192) { // witch become mage and get STAFF +3 (Good Bone Staff) body
                        chr.Sex = 64;
                        chr.Picture = 15;
                        std::string serializedDress = "[0,0,40,12];[53709,1,2,1,{2:3:0:0}];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";
                        chr.Dress = Login_UnserializeItems(serializedDress);
                    }
                }


                // Query to update character with new attributes
                // (((NOTE: ascended field is not at the server. It's DB-only field so we can update it only
                // when we receive character from the server - in this query)))
                chr_query_update = Format("UPDATE `characters` SET \
                                                `id1`='%u', `id2`='%u', `hat_id`='%u', \
                                                `unknown_value_1`='%u', `unknown_value_2`='%u', `unknown_value_3`='%u', \
                                                `nick`='%s', `clan`='%s', \
                                                `picture`='%u', `body`='%u', `reaction`='%u', `mind`='%u', `spirit`='%u', \
                                                `class`='%u', `mainskill`='%u', `flags`='%u', `color`='%u', \
                                                `monsters_kills`='%u', `players_kills`='%u', `frags`='%u', `deaths`='%u', \
                                                `spells`='%u', `active_spell`='%u', `money`='%u', \
                                                `exp_fire_blade`='%u', `exp_water_axe`='%u', \
                                                `exp_air_bludgeon`='%u', `exp_earth_pike`='%u', \
                                                `exp_astral_shooting`='%u', `bag`='%s', `dress`='%s', `deleted`='0', \
                                                `ascended`='%u'",
                                                    chr.Id1, chr.Id2, chr.HatId,
                                                    chr.UnknownValue1, chr.UnknownValue2, chr.UnknownValue3,
                                                    SQL_Escape(chr.Nick).c_str(), SQL_Escape(chr.Clan).c_str(),
                                                    chr.Picture, chr.Body, chr.Reaction, chr.Mind, chr.Spirit,
                                                    chr.Sex, chr.MainSkill, chr.Flags, chr.Color,
                                                    chr.MonstersKills, chr.PlayersKills, chr.Frags, chr.Deaths,
                                                    chr.Spells, chr.ActiveSpell, chr.Money,
                                                    chr.ExpFireBlade, chr.ExpWaterAxe,
                                                    chr.ExpAirBludgeon, chr.ExpEarthPike,
                                                    chr.ExpAstralShooting,
                                                    Login_SerializeItems(chr.Bag).c_str(),
                                                    Login_SerializeItems(chr.Dress).c_str(),
                                                    ascended);

                chr_query_update += ", `sec_55555555`='"; // Append section data
                for(size_t i = 0; i < data_55555555.size(); i++)
                {
                    char ch = (char)data_55555555[i];
                    if(ch == '\\' || ch == '\'') chr_query_update += '\\';
                    chr_query_update += ch;
                }

                chr_query_update += "', `sec_40A40A40`='";
                for(size_t i = 0; i < data_40A40A40.size(); i++)
                {
                    char ch = (char)data_40A40A40[i];
                    if(ch == '\\' || ch == '\'') chr_query_update += '\\';
                    chr_query_update += ch;
                }

                chr_query_update += Format("', `retarded`='0' WHERE `login_id`='%u' AND `id1`='%u' AND `id2`='%u'", login_id, id1, id2); // Finalize query
            }

            if(SQL_Query(chr_query_update) != 0) // Execute update query
            {
                Printf(LOG_Error, "[SQL] %s\n", SQL_Error().c_str());

                SQL_Unlock();
                return false;
            }
        }

        SQL_Unlock(); // Unlock SQL after successful operation
        return true;
    }
    catch(...)
    {
        Printf(LOG_Error, "[SQL] Caught\n");

        SQL_Unlock(); // Unlock SQL in case of exception
        return false;
    }

    return true; // Default return true
}


/**
 * Retrieves character data in binary format.
 *
 * @param login The login name of the user.
 * @param id1 The first part of the character's ID.
 * @param id2 The second part of the character's ID.
 * @param size Reference to store the size of the binary data.
 * @param data Reference to store the pointer to the binary data.
 * @param nickname Reference to store the character's nickname.
 * @param genericId Flag to determine whether to use a generic HatID.
 * @return true if the character data was successfully retrieved, false otherwise.
 */
// Called for operations like sending data over the network or saving it in a compact form.
bool Login_GetCharacter(std::string login, unsigned long id1, unsigned long id2, unsigned long& size, char*& data, std::string& nickname, bool genericId)
{
    //Printf("Login_GetCharacter()\n");

    // Check if SQL connection is active
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login); // Escape SQL characters in login string

        SQL_Lock(); // Lock SQL to prevent concurrent access

        // Query to check if login exists
        std::string query_checklgn = Format("SELECT `id` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0) // Execute query
        {
            SQL_Unlock(); // Unlock SQL if query fails
            return false;
        }

        MYSQL_RES* result = SQL_StoreResult(); // Store query result
        if(!result) // Check if result is valid
        {
            SQL_Unlock(); // Unlock SQL if no result
            return false;
        }

        if(!SQL_NumRows(result)) // Check if login exists in result
        {
            SQL_Unlock();
            SQL_FreeResult(result); // Free result memory
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result); // Fetch row from result
        int login_id = SQL_FetchInt(row, result, "id"); // Get login id from row
        SQL_FreeResult(result); // Free result memory

        // Check if login id is valid
        if(login_id == -1)
        {
            SQL_Unlock();
            return false;
        }

        // Query to get character data
        std::string query_character = Format("SELECT * FROM `characters` WHERE `login_id`='%d' AND `id1`='%u' AND `id2`='%u' AND `deleted`='0'", login_id, id1, id2);
        if(SQL_Query(query_character.c_str()) != 0) // Execute query
        {
            SQL_Unlock();
            return false;
        }

        result = SQL_StoreResult(); // Store query result
        if(!result) // Check if result is valid
        {
            SQL_Unlock();
            return false;
        }

        if(SQL_NumRows(result) != 1) // Ensure exactly one character found
        {
            SQL_Unlock();
            return false;
        }

        row = SQL_FetchRow(result); // Fetch row from result

        // Check if character is flagged as "retarded"
        bool p_retarded = (bool)SQL_FetchInt(row, result, "retarded");
        if(p_retarded)
        {
            size = 0x30; // Set size for binary data
            data = new char[size]; // Allocate memory for binary data
            std::string p_nick = SQL_FetchString(row, result, "nick"); // Fetch nickname
            std::string p_clan = SQL_FetchString(row, result, "clan"); // Fetch clan
            std::string p_nickname = p_nick; // Combine nickname and clan if clan exists
            if(p_clan.length()) p_nickname += "|" + p_clan;
            *(uint32_t*)(data) = 0xFFDDAA11; // Set magic number for binary data
            *(uint8_t*)(data + 4) = (uint8_t)p_nickname.size();
            *(uint8_t*)(data + 5) = (uint8_t)SQL_FetchInt(row, result, "body");
            *(uint8_t*)(data + 6) = (uint8_t)SQL_FetchInt(row, result, "reaction");
            *(uint8_t*)(data + 7) = (uint8_t)SQL_FetchInt(row, result, "mind");
            *(uint8_t*)(data + 8) = (uint8_t)SQL_FetchInt(row, result, "spirit");
            *(uint8_t*)(data + 9) = (uint8_t)SQL_FetchInt(row, result, "mainskill");
            *(uint8_t*)(data + 10) = (uint8_t)SQL_FetchInt(row, result, "picture");
            *(uint8_t*)(data + 11) = (uint8_t)SQL_FetchInt(row, result, "class");
            *(uint32_t*)(data + 12) = (uint32_t)SQL_FetchInt(row, result, "id1");
            *(uint32_t*)(data + 16) = (uint32_t)SQL_FetchInt(row, result, "id2");
            memcpy(data + 20, p_nickname.data(), p_nickname.size()); // Copy nickname to binary data
            nickname = p_nick; // Set nickname
        }
        // Handle normal characters
        else
        {
            CCharacter chr;
            chr.Id1 = SQL_FetchInt(row, result, "id1");
            chr.Id2 = SQL_FetchInt(row, result, "id2");
            chr.HatId = (genericId ? Config::HatID : SQL_FetchInt(row, result, "hat_id"));
            chr.UnknownValue1 = SQL_FetchInt(row, result, "unknown_value_1");
            chr.UnknownValue2 = SQL_FetchInt(row, result, "unknown_value_2");
            chr.UnknownValue3 = SQL_FetchInt(row, result, "unknown_value_3");
            chr.Nick = SQL_FetchString(row, result, "nick");
            chr.Clan = SQL_FetchString(row, result, "clan");
            chr.ClanTag = SQL_FetchString(row, result, "clantag");
            chr.Picture = SQL_FetchInt(row, result, "picture");
            chr.Body = SQL_FetchInt(row, result, "body");
            chr.Reaction = SQL_FetchInt(row, result, "reaction");
            chr.Mind = SQL_FetchInt(row, result, "mind");
            chr.Spirit = SQL_FetchInt(row, result, "spirit");
            chr.Sex = SQL_FetchInt(row, result, "class");
            chr.MainSkill = SQL_FetchInt(row, result, "mainskill");
            chr.Flags = SQL_FetchInt(row, result, "flags");
            chr.Color = SQL_FetchInt(row, result, "color");
            chr.MonstersKills = SQL_FetchInt(row, result, "monsters_kills");
            chr.PlayersKills = SQL_FetchInt(row, result, "players_kills");
            chr.Frags = SQL_FetchInt(row, result, "frags");
            chr.Deaths = SQL_FetchInt(row, result, "deaths");
            chr.Money = SQL_FetchInt(row, result, "money");
            chr.Spells = SQL_FetchInt(row, result, "spells");
            chr.ActiveSpell = SQL_FetchInt(row, result, "active_spell");
            chr.ExpFireBlade = SQL_FetchInt(row, result, "exp_fire_blade");
            chr.ExpWaterAxe = SQL_FetchInt(row, result, "exp_water_axe");
            chr.ExpAirBludgeon = SQL_FetchInt(row, result, "exp_air_bludgeon");
            chr.ExpEarthPike = SQL_FetchInt(row, result, "exp_earth_pike");
            chr.ExpAstralShooting = SQL_FetchInt(row, result, "exp_astral_shooting");

            // Handle additional sections
            std::string data_55555555 = SQL_FetchString(row, result, "sec_55555555");
            std::string data_40A40A40 = SQL_FetchString(row, result, "sec_40A40A40");
            chr.Section55555555.Reset();
            chr.Section55555555.WriteFixedString(data_55555555, data_55555555.size());
            chr.Section40A40A40.Reset();
            chr.Section40A40A40.WriteFixedString(data_40A40A40, data_55555555.size());

            chr.Bag = Login_UnserializeItems(SQL_FetchString(row, result, "bag"));
            chr.Dress = Login_UnserializeItems(SQL_FetchString(row, result, "dress"));

            // Serialize character to binary stream
            BinaryStream strm;
            if(!chr.SaveToStream(strm))
            {
                SQL_FreeResult(result);
                SQL_Unlock();
                return false;
            }

            // Copy binary stream to data
            std::vector<uint8_t>& buf = strm.GetBuffer();
            size = buf.size();
            data = new char[size];
            for(uint32_t i = 0; i < size; i++)
                data[i] = buf[i];

            nickname = chr.Nick; // Set nickname
        }

        SQL_FreeResult(result); // Free query result
        SQL_Unlock(); // Unlock SQL after successful operation
        return true;
    }
    catch(...)
    {
        SQL_Unlock(); // Unlock SQL in case of exception
        return false;
    }

    return true; // Default return true
}

/**
 * Retrieves character data directly into a CCharacter object.
 *
 * @param login The login name of the user.
 * @param id1 The first part of the character's ID.
 * @param id2 The second part of the character's ID.
 * @param character Reference to store the retrieved character object.
 * @return true if the character data was successfully retrieved, false otherwise.
 */
// Called to get direct access to character attributes.
// It returns a populated CCharacter object that can be manipulated or accessed within the application.
bool Login_GetCharacter(std::string login, unsigned long id1, unsigned long id2, CCharacter& character)
{
    // Check if SQL connection is active
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login); // Escape SQL characters in login string

        SQL_Lock(); // Lock SQL to prevent concurrent access

        // Query to check if login exists
        std::string query_checklgn = Format("SELECT `id` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0) // Execute query
        {
            SQL_Unlock(); // Unlock SQL if query fails
            return false;
        }

        MYSQL_RES* result = SQL_StoreResult(); // Store query result
        if(!result) // Check if result is valid
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result)) // Check if login exists in result
        {
            SQL_Unlock();
            SQL_FreeResult(result); // Free result memory
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result); // Fetch row from result
        int login_id = SQL_FetchInt(row, result, "id"); // Get login id from row
        SQL_FreeResult(result); // Free result memory

        // Check if login id is valid
        if(login_id == -1)
        {
            SQL_Unlock();
            return false;
        }

        // Query to get character data
        std::string query_character = Format("SELECT * FROM `characters` WHERE `login_id`='%d' AND `id1`='%u' AND `id2`='%u' AND `deleted`='0'", login_id, id1, id2);
        if(SQL_Query(query_character.c_str()) != 0) // Execute query
        {
            SQL_Unlock();
            return false;
        }

        result = SQL_StoreResult(); // Store query result
        if(!result) // Check if result is valid
        {
            SQL_Unlock();
            return false;
        }

        if(SQL_NumRows(result) != 1) // Ensure exactly one character found
        {
            SQL_Unlock();
            return false;
        }

        row = SQL_FetchRow(result); // Fetch row from result

        // Populate CCharacter object with character data
        bool p_retarded = (bool)SQL_FetchInt(row, result, "retarded");
        CCharacter chr;
        chr.Retarded = p_retarded;
        chr.Id1 = SQL_FetchInt(row, result, "id1");
        chr.Id2 = SQL_FetchInt(row, result, "id2");
        chr.HatId = SQL_FetchInt(row, result, "hat_id");
        chr.UnknownValue1 = SQL_FetchInt(row, result, "unknown_value_1");
        chr.UnknownValue2 = SQL_FetchInt(row, result, "unknown_value_2");
        chr.UnknownValue3 = SQL_FetchInt(row, result, "unknown_value_3");
        chr.Nick = SQL_FetchString(row, result, "nick");
        chr.Clan = SQL_FetchString(row, result, "clan");
        chr.ClanTag = SQL_FetchString(row, result, "clantag");
        chr.Picture = SQL_FetchInt(row, result, "picture");
        chr.Body = SQL_FetchInt(row, result, "body");
        chr.Reaction = SQL_FetchInt(row, result, "reaction");
        chr.Mind = SQL_FetchInt(row, result, "mind");
        chr.Spirit = SQL_FetchInt(row, result, "spirit");
        chr.Sex = SQL_FetchInt(row, result, "class");
        chr.MainSkill = SQL_FetchInt(row, result, "mainskill");
        chr.Flags = SQL_FetchInt(row, result, "flags");
        chr.Color = SQL_FetchInt(row, result, "color");
        chr.MonstersKills = SQL_FetchInt(row, result, "monsters_kills");
        chr.PlayersKills = SQL_FetchInt(row, result, "players_kills");
        chr.Frags = SQL_FetchInt(row, result, "frags");
        chr.Deaths = SQL_FetchInt(row, result, "deaths");
        chr.Money = SQL_FetchInt(row, result, "money");
        chr.Spells = SQL_FetchInt(row, result, "spells");
        chr.ActiveSpell = SQL_FetchInt(row, result, "active_spell");
        chr.ExpFireBlade = SQL_FetchInt(row, result, "exp_fire_blade");
        chr.ExpWaterAxe = SQL_FetchInt(row, result, "exp_water_axe");
        chr.ExpAirBludgeon = SQL_FetchInt(row, result, "exp_air_bludgeon");
        chr.ExpEarthPike = SQL_FetchInt(row, result, "exp_earth_pike");
        chr.ExpAstralShooting = SQL_FetchInt(row, result, "exp_astral_shooting");

        // Handle additional sections
        std::string data_55555555 = SQL_FetchString(row, result, "sec_55555555");
        std::string data_40A40A40 = SQL_FetchString(row, result, "sec_40A40A40");
        chr.Section55555555.Reset();
        chr.Section55555555.WriteFixedString(data_55555555, data_55555555.size());
        chr.Section40A40A40.Reset();
        chr.Section40A40A40.WriteFixedString(data_40A40A40, data_55555555.size());

        chr.Bag = Login_UnserializeItems(SQL_FetchString(row, result, "bag"));
        chr.Dress = Login_UnserializeItems(SQL_FetchString(row, result, "dress"));

        character = chr; // Set the character object

        SQL_FreeResult(result); // Free query result
        SQL_Unlock(); // Unlock SQL after successful operation
        return true;
    }
    catch(...)
    {
        SQL_Unlock(); // Unlock SQL in case of exception
        return false;
    }

    return true; // Default return true
}


bool Login_GetCharacterList(std::string login, std::vector<CharacterInfo>& info, int hatId)
{
    //Printf("Login_GetCharacterList()\n");
    info.clear();

    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_checklgn = Format("SELECT `id` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_Unlock();
            SQL_FreeResult(result);
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result);
        int login_id = SQL_FetchInt(row, result, "id");
        SQL_FreeResult(result);
        if(login_id == -1)
        {
            SQL_Unlock();
            return false;
        }

        std::string query_character = Format("SELECT `id1`, `id2` FROM `characters` WHERE `login_id`='%d' AND `deleted`='0' AND `retarded`='0'", login_id);
        if (hatId > 0) query_character += Format(" AND (`hat_id`='%d' or (`id2`&0x3F000000)=0x3F000000)", hatId);
        if(SQL_Query(query_character.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        int rows = SQL_NumRows(result);
        for(int i = 0; i < rows; i++)
        {
            row = SQL_FetchRow(result);
            CharacterInfo inf;
            inf.ID1 = SQL_FetchInt(row, result, "id1");
            inf.ID2 = SQL_FetchInt(row, result, "id2");
            info.push_back(inf);
        }

        SQL_FreeResult(result);

        SQL_Unlock();
        return true;
    }
    catch(...)
    {
        SQL_Unlock();
        return true;
    }
}

const char enru_charmap_en[] =
    {
        'a',
        'A',
        'c',
        'C',
        'e',
        'E',
        'K',
        'o',
        'О',
        'p',
        'P',
        'u',
        'x',
        'X',
        'y',
        'Y',
        'm',
    };

const char enru_charmap_ru[] =
    {
        '†',
        'А',
        'б',
        'С',
        '•',
        'Е',
        'К',
        'Ѓ',
        'О',
        'а',
        'Р',
        '®',
        'е',
        'Х',
        'г',
        'У',
        'в',
    };

std::string RegexEscape(char what)
{
    std::string retval = "";
    switch(what)
    {
        case '*':
        case '.':
        case '?':
        case '\\':
        case '|':
        case '(':
        case ')':
        case '[':
        case ']':
        case '+':
        case '-':
        case '{':
        case '}':
        case '"':
        case '\'':
        case '^':
        case '$':
            retval += "\\";
        default:
            retval += what;
            break;
    }

    return retval;
}

bool Login_NickExists(std::string nickname, int hatId)
{
    //Printf("Login_NickExists()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        std::string nickname2 = nickname;
        nickname = "";
        for(size_t i = 0; i < nickname2.length(); i++)
        {
            bool char_dest = false;
            for(size_t j = 0; j < sizeof(enru_charmap_en); j++)
            {
                if(nickname2[i] == enru_charmap_en[j] ||
                   nickname2[i] == enru_charmap_ru[j])
                {
                    if(!char_dest) nickname += "[";
                    char_dest = true;
                    nickname += RegexEscape((enru_charmap_en[j] == nickname2[i]) ? enru_charmap_ru[j] : enru_charmap_en[j]);
                }
            }

            nickname += RegexEscape(nickname2[i]);
            if(char_dest)
                nickname += "]";
        }

        nickname = SQL_Escape(nickname);
        //Printf(LOG_Info, "Resulting nickname: %s\n", nickname.c_str());

        SQL_Lock();
        std::string query_nickheck = Format("SELECT `nick` FROM `characters` WHERE `nick` REGEXP '^%s$' AND `deleted`='0'", nickname.c_str());
        if (hatId > 0) query_nickheck += Format(" AND `hat_id`='%d'", hatId);
        if(SQL_Query(query_nickheck.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_FreeResult(result);
            SQL_Unlock();
            return false; // nick does not exist
        }

        SQL_FreeResult(result);
        SQL_Unlock();
        return true; // nick exists
    }
    catch(...)
    {
        SQL_Unlock();
        return true;
    }
}

bool Login_DelCharacter(std::string login, unsigned long id1, unsigned long id2)
{
    //Printf("Login_DelCharacter()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_checklgn = Format("SELECT `id` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_Unlock();
            SQL_FreeResult(result);
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result);
        int login_id = SQL_FetchInt(row, result, "id");
        SQL_FreeResult(result);
        if(login_id == -1)
        {
            SQL_Unlock();
            return false;
        }

        //std::string query_delchar = Format("DELETE FROM `characters` WHERE `login_id`='%d' AND `id1`='%u' AND `id2`='%u'", login_id, id1, id2);
        std::string query_delchar = Format("UPDATE `characters` SET `deleted`='1' WHERE `login_id`='%d' AND `id1`='%u' AND `id2`='%u'", login_id, id1, id2);
        if(SQL_Query(query_delchar.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        if(SQL_AffectedRows() != 1)
        {
            SQL_Unlock();
            return false;
        }

        SQL_Unlock();
        return true;
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
}

bool Login_CharExists(std::string login, unsigned long id1, unsigned long id2, bool onlyNormal)
{
    //Printf("Login_CharExists()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_checklgn = Format("SELECT `id` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_checklgn.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_Unlock();
            SQL_FreeResult(result);
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result);
        int login_id = SQL_FetchInt(row, result, "id");
        SQL_FreeResult(result);
        if(login_id == -1)
        {
            SQL_Unlock();
            return false;
        }

        std::string query_char = Format("SELECT `login_id` FROM `characters` WHERE `login_id`='%d' AND `id1`='%u' AND `id2`='%u' AND `deleted`='0'", login_id, id1, id2);
        if(onlyNormal) query_char += " AND `retarded`='0'";
        if(SQL_Query(query_char.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_Unlock();
            SQL_FreeResult(result);
            return false;
        }

        SQL_FreeResult(result);

        SQL_Unlock();
        return true;
    }
    catch(...)
    {
        SQL_Unlock();
        return true;
    }
}

bool Login_GetIPF(std::string login, std::string& ipf)
{
    //Printf("Login_GetIPF()\n");
    if(!SQL_CheckConnected()) return false;

    try
    {
        login = SQL_Escape(login);

        SQL_Lock();
        std::string query_getipf = Format("SELECT `ip_filter` FROM `logins` WHERE LOWER(`name`)=LOWER('%s')", login.c_str());
        if(SQL_Query(query_getipf.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }
        MYSQL_RES* result = SQL_StoreResult();
        if(!result)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_NumRows(result))
        {
            SQL_Unlock();
            SQL_FreeResult(result);
            return false; // login does not exist
        }

        MYSQL_ROW row = SQL_FetchRow(result);
        ipf = SQL_FetchString(row, result, "ip_filter");
        SQL_FreeResult(result);
        SQL_Unlock();
        return true;
    }
    catch(...)
    {
        SQL_Unlock();
        return true;
    }
}

bool Login_SetIPF(std::string login, std::string ipf)
{
    //Printf("Login_SetIPF()\n");
    try
    {
        login = SQL_Escape(login);
        ipf = SQL_Escape(ipf);

        if(!Login_Exists(login)) return false;

        SQL_Lock();
        std::string query_setipf = Format("UPDATE `logins` SET `ip_filter`='%s' WHERE LOWER(`name`)=LOWER('%s')", ipf.c_str(), login.c_str());
        if(SQL_Query(query_setipf.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        if(!SQL_AffectedRows())
        {
            SQL_Unlock();
            return false;
        }

        SQL_Unlock();
        return true;
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
}

bool Login_LogAuthentication(std::string login, std::string ip, std::string uuid)
{
    try
    {
        login = SQL_Escape(login);
        ip = SQL_Escape(ip);
        uuid = SQL_Escape(uuid);

        SQL_Lock();
        std::string query_logauth = Format("INSERT INTO `authlog` (`login_name`, `ip`, `uuid`, `date`) VALUES ('%s', '%s', '%s', NOW())", login.c_str(), ip.c_str(), uuid.c_str());
        if (SQL_Query(query_logauth.c_str()) != 0)
        {
            SQL_Unlock();
            return false;
        }

        SQL_Unlock();
        return true;
    }
    catch(...)
    {
        SQL_Unlock();
        return false;
    }
}
