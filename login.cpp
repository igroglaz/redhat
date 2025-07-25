#include "checkpoint.h"
#include "circle.h"
#include "constants.h"
#include "login.hpp"
#include "sql.hpp"
#include "utils.hpp"
#include "config.hpp"
#include "kill_stats.h"
#include "merge_items.hpp"
#include "server_id.hpp"
#include "shelf.hpp"
#include "update_character.h"

#include "sha1.h"

#include <stdint.h>
#include <windows.h>
#include <limits>

#include <cstdlib> // For std::rand()

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
            return true; // Unclear if this should be `true` or `false`. On the one hand, the login was deleted, on the other --- the DB entry is clearly wrong.
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

bool Login_SetLocked(std::string login, bool locked_hat, bool locked, unsigned long id1, unsigned long id2, ServerIDType srvid)
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

bool Login_GetLocked(std::string login, bool& locked_hat, bool& locked, unsigned long& id1, unsigned long& id2, ServerIDType& srvid)
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
        srvid = static_cast<ServerIDType>(SQL_FetchInt(row, result, "locked_srvid"));
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

    items.UnknownValue0 = static_cast<uint8_t>(StrToInt(Trim(f_listdata[0])));
    items.UnknownValue1 = static_cast<uint8_t>(StrToInt(Trim(f_listdata[1])));
    items.UnknownValue2 = static_cast<uint8_t>(StrToInt(Trim(f_listdata[2])));
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
        item.Count = static_cast<uint16_t>(StrToInt(Trim(f_itemdata[3])));
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
                effect.Id1 = static_cast<uint8_t>(StrToInt(Trim(f_efdata[0])));
                effect.Value1 = static_cast<uint8_t>(StrToInt(Trim(f_efdata[1])));
                effect.Id2 = static_cast<uint8_t>(StrToInt(Trim(f_efdata[2])));
                effect.Value2 = static_cast<uint8_t>(StrToInt(Trim(f_efdata[3])));
                item.Effects.push_back(effect);
            }
        }
        f_str.erase(f_str.begin());
    }
    return items;
}

int AllowMage(const char* login) {
    int allow_mage = 0;

    std::string query_get_allow = Format("SELECT `allow_mage` FROM `logins` WHERE LOWER(`name`) = LOWER('%s')", login);
    if (SQL_Query(query_get_allow.c_str()) != 0) {
        Printf(LOG_Warning, "[DB] Failed to select `allow_mage` for login `%s`: %s\n", login, SQL_Error().c_str());
        return 0;
    }

    MYSQL_RES* res_allow = SQL_StoreResult();
    if (SQL_NumRows(res_allow) > 0) {
        MYSQL_ROW row_allow = SQL_FetchRow(res_allow);
        allow_mage = SQL_FetchInt(row_allow, res_allow, "allow_mage");
    }

    SQL_FreeResult(res_allow);

    return allow_mage;
}

bool Login_SetCharacter(std::string login, unsigned long id1, unsigned long id2, unsigned long size, char* data, std::string nickname, ServerIDType srvid)
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
        std::string query_checkchr = Format("SELECT `id` FROM `characters` WHERE `login_id`='%d' AND `id1`='%u' AND `id2`='%u'", login_id, id1, id2);
        int character_id = -1;
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
            if (SQL_NumRows(result) == 1) {
                MYSQL_ROW row = SQL_FetchRow(result);
                character_id = SQL_FetchInt(row, result, "id");
                create = false;
            }

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
            chr.LoginID = login_id;
            chr.ID = character_id;

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

            // Giga-characters are restored from a checkpoint if they've died.
            if (chr.Nick[0] == '_') {
                checkpoint::Checkpoint saved(character_id);

                if (!saved.loaded_from_db) {
                    // Character died before saving for the first time. Create a fake checkpoint from scratch.
                    if (srvid != EASY) {
                        Printf(LOG_Error, "[checkpoint] no checkpoint for %d at server %d!\n", character_id, srvid);
                    }
                    saved.body = chr.Body;
                    saved.reaction = chr.Reaction;
                    saved.mind = chr.Mind;
                    saved.spirit = chr.Spirit;
                    saved.monsters_kills = 0;
                    saved.players_kills = 0;
                    saved.frags = 0;
                    saved.deaths = 0; // Set deaths to 0, so `chr.Deaths > saved.deaths` below will trigger.
                    saved.exp_fire_blade = 0;
                    saved.exp_water_axe = 0;
                    saved.exp_air_bludgeon = 0;
                    saved.exp_earth_pike = 0;
                    saved.exp_astral_shooting = 0;
                    saved.dress = "[0,0,40,12];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1];[0,0,0,1]";
                }

                if (chr.Deaths > saved.deaths) {
                    Printf(LOG_Info, "[checkpoint] restoring character %d from checkpoint\n", character_id);
                    if (srvid != EASY) { // On EASY server the base stats are preserved.
                        chr.Body = saved.body;
                        chr.Reaction = saved.reaction;
                        chr.Mind = saved.mind;
                        chr.Spirit = saved.spirit;
                    }
                    chr.ExpFireBlade = saved.exp_fire_blade;
                    chr.ExpWaterAxe = saved.exp_water_axe;
                    chr.ExpAirBludgeon = saved.exp_air_bludgeon;
                    chr.ExpEarthPike = saved.exp_earth_pike;
                    chr.ExpAstralShooting = saved.exp_astral_shooting;
                    chr.Dress = Login_UnserializeItems(saved.dress);
                    chr.Bag.Items.clear();
                    update_character::ClearMonsterKills(chr); // reset info about killed mobs

                    if (chr.IsWizard()) {
                        update_character::WipeSpells(chr);
                    }

                    if (saved.loaded_from_db) { // If the character is new, we'll create a new checkpoint in `UpdateCharacter` anyway.
                        checkpoint::UpdateDeaths(character_id, chr.Deaths);
                    }
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
                                                `ascended`, \
                                                `points`) VALUES ( \
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
                unsigned int ascended = 0; // DB-only flag to ladder score
                unsigned int points = 0;   // DB-only flag to ladder score
                UpdateCharacter(chr, srvid, shelf::StoreOnShelf, &ascended, &points); // pass pointers
                update_character::SaveTreasurePoints(chr.ID, srvid, points);

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
                                                `ascended` = `ascended` + %u",
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

            Printf(LOG_Info, "[update] Character '%s' saved to the database\n", chr.GetFullName().c_str());
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
            chr.LoginID = SQL_FetchInt(row, result, "login_id");
            chr.Id1 = SQL_FetchInt(row, result, "id1");
            chr.Id2 = SQL_FetchInt(row, result, "id2");
            chr.HatId = (genericId ? Config::HatID : SQL_FetchInt(row, result, "hat_id"));
            chr.UnknownValue1 = static_cast<uint8_t>(SQL_FetchInt(row, result, "unknown_value_1"));
            chr.UnknownValue2 = static_cast<uint8_t>(SQL_FetchInt(row, result, "unknown_value_2"));
            chr.UnknownValue3 = static_cast<uint8_t>(SQL_FetchInt(row, result, "unknown_value_3"));
            chr.Nick = SQL_FetchString(row, result, "nick");
            chr.Clan = SQL_FetchString(row, result, "clan");
            chr.ClanTag = SQL_FetchString(row, result, "clantag");
            chr.Picture = static_cast<uint8_t>(SQL_FetchInt(row, result, "picture"));
            chr.Body = static_cast<uint8_t>(SQL_FetchInt(row, result, "body"));
            chr.Reaction = static_cast<uint8_t>(SQL_FetchInt(row, result, "reaction"));
            chr.Mind = static_cast<uint8_t>(SQL_FetchInt(row, result, "mind"));
            chr.Spirit = static_cast<uint8_t>(SQL_FetchInt(row, result, "spirit"));
            chr.Sex = static_cast<uint8_t>(SQL_FetchInt(row, result, "class"));
            chr.MainSkill = static_cast<uint8_t>(SQL_FetchInt(row, result, "mainskill"));
            chr.Flags = static_cast<uint8_t>(SQL_FetchInt(row, result, "flags"));
            chr.Color = static_cast<uint8_t>(SQL_FetchInt(row, result, "color"));
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
        chr.UnknownValue1 = static_cast<uint8_t>(SQL_FetchInt(row, result, "unknown_value_1"));
        chr.UnknownValue2 = static_cast<uint8_t>(SQL_FetchInt(row, result, "unknown_value_2"));
        chr.UnknownValue3 = static_cast<uint8_t>(SQL_FetchInt(row, result, "unknown_value_3"));
        chr.Nick = SQL_FetchString(row, result, "nick");
        chr.Clan = SQL_FetchString(row, result, "clan");
        chr.ClanTag = SQL_FetchString(row, result, "clantag");
        chr.Picture = static_cast<uint8_t>(SQL_FetchInt(row, result, "picture"));
        chr.Body = static_cast<uint8_t>(SQL_FetchInt(row, result, "body"));
        chr.Reaction = static_cast<uint8_t>(SQL_FetchInt(row, result, "reaction"));
        chr.Mind = static_cast<uint8_t>(SQL_FetchInt(row, result, "mind"));
        chr.Spirit = static_cast<uint8_t>(SQL_FetchInt(row, result, "spirit"));
        chr.Sex = static_cast<uint8_t>(SQL_FetchInt(row, result, "class"));
        chr.MainSkill = static_cast<uint8_t>(SQL_FetchInt(row, result, "mainskill"));
        chr.Flags = static_cast<uint8_t>(SQL_FetchInt(row, result, "flags"));
        chr.Color = static_cast<uint8_t>(SQL_FetchInt(row, result, "color"));
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

bool Login_NickExists(std::string nickname, int hatId) {
    if(!SQL_CheckConnected()) return false;

    std::string query_nickheck = Format("SELECT `nick` FROM `characters` WHERE LOWER(`nick`) = LOWER('%s') AND `deleted`='0'", SQL_Escape(nickname).c_str());
    if (hatId > 0) {
        query_nickheck += Format(" AND `hat_id`='%d'", hatId);
    }

    SimpleSQL result(query_nickheck);
    if (!result) {
        return false;
    }

    if (!SQL_NumRows(result.result)) {
        return false; // nick does not exist
    }

    return true; // nick exists
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
        //std::string query_delchar = Format("DELETE FROM `characters` WHERE `login_id`=%d AND `id1`=%u AND `id2`=%u", login_id, id1, id2);
        // Mark character as deleted
        std::string query_delchar = Format("UPDATE `characters` SET `deleted`=1 WHERE `login_id`=%d AND `id1`=%u AND `id2`=%u", login_id, id1, id2);
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
        
        // Delete from treasure table by character_id
        std::string query_deltreasure = Format(
            "DELETE FROM `treasure` WHERE `character_id` = "
            "(SELECT `id` FROM `characters` WHERE `login_id`=%d AND `id1`=%u AND `id2`=%u)", 
            login_id, id1, id2);

        SQL_Query(query_deltreasure.c_str());
        
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

/** Update the character in the database, after creation or after leaving a map.
 *
 *  Parameters:
 *      chr: the character. Will be updated inplace.
 *      srvid: the ID of a server the character was on. For example: 2 means the character was at #2 (so, mind <= 15).
 *      ascended: output parameter: has the character ascended?
 *      points: output parameter: points for finding the treasure
 */
void UpdateCharacter(CCharacter& chr, ServerIDType srvid, shelf::StoreOnShelfFunction store_on_shelf, unsigned int* ascended, unsigned int* points) {
    const std::string chr_full_name = chr.GetFullName();
    const char* full_name = chr_full_name.c_str();
    Printf(LOG_Info, "[update] character '%s' on s%d\n", full_name, srvid);

    MergeItems(chr.Bag, srvid);

    int haveTreasures = update_character::ConsumeTreasures(chr, srvid);
    *points = haveTreasures;

    Printf(LOG_Info, "[update] character '%s' has %d treasures\n", full_name, haveTreasures);

    update_character::VisitShelf(chr, srvid);

    Printf(LOG_Info, "[update] character '%s' visited shelf\n", full_name);

    bool reborn = update_character::IsAttemptingReborn(chr, srvid);
    if (reborn) {
        Printf(LOG_Info, "[update] character '%s' is attempting reborn\n", full_name);
        // The player wanted to do a reborn, but doesn't meet criteria: revert the stats, so the player is left on the same server.
        if (!update_character::MeetsRebornCriteria(chr, srvid, haveTreasures)) {
            Printf(LOG_Info, "[update] character '%s' does not meet reborn criteria\n", full_name);
            reborn = false;
            update_character::FailReborn(chr, srvid);
        }
    }

    if (reborn) {
        if (srvid == EASY) {
            Printf(LOG_Info, "[update] character '%s' does reborn from EASY, gets new face\n", full_name);
            // Male characters become zombies on first reborn (except Ironman and Legend)
            if (!chr.IsFemale() && chr.Nick[0] != '@' && chr.Nick[0] != '_') {
                chr.Picture = 64;
            }

        // allow to create mages only if managed to finish HARD
        // (fill allow_mage DB field on rebirth)
        } else if (srvid == HARD) {
            Printf(LOG_Info, "[update] character '%s' does reborn from HARD, unlocks mages\n", full_name);
            if (chr.Nick[0] == '@') { // for @ chars: make it 1 if it's 0
                std::string query_update_allow = Format("UPDATE `logins` SET `allow_mage` = 1 WHERE `id` = '%u' AND `allow_mage` = 0", chr.LoginID);

                SQL_Lock();
                if (SQL_Query(query_update_allow.c_str()) != 0)
                {
                     Printf(LOG_Error, "[DB] Failed to update allow_mage: %s\n", SQL_Error().c_str());
                }
                SQL_Unlock();
            } else if (chr.Nick[0] == '_') { // for _ chars: make it 2
                std::string query_update_allow = Format("UPDATE `logins` SET `allow_mage` = 2 WHERE `id` = '%u'", chr.LoginID);

                SQL_Lock();
                if (SQL_Query(query_update_allow.c_str()) != 0)
                {
                     Printf(LOG_Error, "[DB] Failed to update allow_mage: %s\n", SQL_Error().c_str());
                }
                SQL_Unlock();
            }
        }

        Printf(LOG_Info, "[update] character '%s' performs reborn\n", full_name);
        update_character::PerformReborn(chr, srvid, store_on_shelf);

        // Create a checkpoint for giga-characters on reborn.
        if (chr.Nick[0] == '_') {
            Printf(LOG_Info, "[checkpoint] saving %d on reborn from server %d\n", chr.ID, srvid);
            checkpoint::Checkpoint(chr, false).SaveToDB(chr.ID);
        }
    }

    // Create a checkpoint for giga-characters on EASY server always but only with stats.
    if (chr.Nick[0] == '_' && !reborn && srvid == EASY) {
        Printf(LOG_Info, "[checkpoint] saving stats for %d\n", chr.ID);
        checkpoint::Checkpoint(chr, true).SaveToDB(chr.ID);
    }

    // RECLASS: warrior/mage become ama/witch
    // (note it can't happen simultaneously with reborn as at reborn we "half" the exp)
    if (update_character::ShouldReclass(chr, srvid)) {
        Printf(LOG_Info, "[update] character '%s' performs reclass\n", full_name);
        update_character::PerformReclass(chr, srvid, store_on_shelf);

        // Create a checkpoint for giga-characters on reclass.
        if (chr.Nick[0] == '_') {
            Printf(LOG_Info, "[checkpoint] saving %d on reclass\n", chr.ID);
            checkpoint::Checkpoint(chr, false).SaveToDB(chr.ID);
        }
    // ASCEND: ama/witch become again war/mage and receive crown
    } else if (update_character::ShouldAscend(chr, srvid)) {
        Printf(LOG_Info, "[update] character '%s' performs ascend\n", full_name);
        update_character::PerformAscend(chr, srvid, store_on_shelf);

        // increment ascended DB-only field to mark that character was ascended (for ladder score)
        *ascended = 1; // We use it as a flag. DB increments if it's 1.

        // Create a checkpoint for giga-characters on ascend.
        if (chr.Nick[0] == '_') {
            Printf(LOG_Info, "[checkpoint] saving %d on ascend\n", chr.ID);
            checkpoint::Checkpoint(chr, false).SaveToDB(chr.ID);
        }
    } else if (circle::Allowed(chr)) {
        Printf(LOG_Info, "[update] character '%s' goes to hell\n", full_name);
        if (chr.Clan == "miss_hell" && circle::Circle(chr) == 0) {
            // Womanize!
            if (chr.IsWarrior()) {
                chr.Sex = sex::amazon;
                chr.Picture = 11;
            } else if (chr.IsMage()) {
                chr.Sex = sex::witch;
                chr.Picture = 6;
            }
        }

        circle::Advance(chr);
        chr.Money -= circle::price;

        update_character::ClearMonsterKills(chr);

        update_character::StoreOnShelf(srvid, chr, true, store_on_shelf);

        if (chr.Deaths == 0) {
            chr.Money = 133; // HC: leave 133 gold to buy a bow
        }

        chr.Body = chr.Reaction = chr.Mind = chr.Spirit = 1;
        chr.ExpFireBlade = chr.ExpWaterAxe = chr.ExpAirBludgeon = chr.ExpEarthPike = chr.ExpAstralShooting = 0;

        if (chr.IsWizard()) {
            update_character::WipeSpells(chr);
        }

        if (chr.Nick[0] == '_') {
            Printf(LOG_Info, "[checkpoint] saving %d on circle\n", chr.ID);
            checkpoint::Checkpoint(chr, false).SaveToDB(chr.ID);
        }
    } else {
        Printf(LOG_Info, "[update] character '%s' eats the treasure\n", full_name);
        // If the player didn't ascend or reclass, the boss key on NIGHTMARE+ increases stats.
        update_character::DrinkTreasure(chr, srvid, haveTreasures);
    }

    update_character::ExperienceLimit(chr, srvid);

    Printf(LOG_Info, "[update] character '%s' updated successfully\n", full_name);
}
