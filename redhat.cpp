#include <fstream>

#include "config.hpp"
#include "socket.hpp"
#include "sql.hpp"
#include "utils.hpp"
#include "listener.hpp"
#include "status.hpp"
#include "login.hpp"
#include "circle.h"
#include "thresholds.h"

void H_Quit()
{
    Printf(LOG_Info, "[HC] Hat is shutting down.\n");
    Net_Quit();
    SQL_Close();
}

void LGN_DBConvert(std::string directory);

DWORD ExceptionHandler(struct _EXCEPTION_POINTERS *info) {
    try {
        // dump info
        Printf(LOG_Error, "EXCEPTION DUMP:\neax=%08Xh,ebx=%08Xh,ecx=%08Xh,edx=%08Xh,\nesp=%08Xh,ebp=%08Xh,esi=%08Xh,edi=%08Xh;\neip=%08Xh;\naddr=%08Xh,code=%08Xh,flags=%08Xh\n",
                info->ContextRecord->Eax,
                info->ContextRecord->Ebx,
                info->ContextRecord->Ecx,
                info->ContextRecord->Edx,
                info->ContextRecord->Esp,
                info->ContextRecord->Ebp,
                info->ContextRecord->Esi,
                info->ContextRecord->Edi,
                info->ContextRecord->Eip,
                info->ExceptionRecord->ExceptionAddress,
                info->ExceptionRecord->ExceptionCode,
                info->ExceptionRecord->ExceptionFlags);

        Printf(LOG_Error, "BEGIN STACK TRACE: 0x%08Xh <= ", info->ExceptionRecord->ExceptionAddress);
        unsigned long stebp = *(unsigned long*)(info->ContextRecord->Ebp);
        while (true) {
            bool bad_ebp = false;
            if(stebp & 3) bad_ebp = true;
            if(!bad_ebp && IsBadReadPtr((void*)stebp, 8)) bad_ebp = true;

            if(bad_ebp) break;

            Printf(LOG_Error, "%08Xh <= ", *(unsigned long*)(stebp+4));
            stebp = *(unsigned long*)(stebp);
        }
        Printf(LOG_Error, "END STACK TRACE\n");
        
        ExitProcess(1);
    } catch (...) {
        Printf(LOG_Error, "Failed to capture stack trace\n");
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void SetExceptionFilter() {
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&ExceptionHandler);
}

void GiveCircleReward(std::string value) {
    size_t colon_pos = value.find(':');

    if (colon_pos == std::string::npos) {
        Printf(LOG_Error, "Wrong format: should be like `-give-circle-reward=coolnickname:1`\n");
        return;
    }

    std::string nickname = value.substr(0, colon_pos);
    std::string reward_str = value.substr(colon_pos + 1);
    int reward_index = std::stoi(reward_str);
    
    SimpleSQL query{Format("SELECT class, bag FROM characters WHERE nick = '%s' AND deleted = 0;", SQL_Escape(nickname).c_str())};
    if (!query) {
        Printf(LOG_Error, "Failed to query character with nick %s\n", nickname.c_str());
        return;
    }

    if (SQL_NumRows(query.result) == 0) {
        Printf(LOG_Error, "Character with nick %s not found\n", nickname.c_str());
        return;
    }

    auto row = SQL_FetchRow(query.result);
    auto sex = static_cast<uint8_t>(SQL_FetchInt(row, query.result, "class"));
    auto bag = SQL_FetchString(row, query.result, "bag");

    auto item_list = Login_UnserializeItems(bag);

    auto item = circle::Reward(sex, reward_index);
    Printf(LOG_Info, "Giving reward id=%d to character '%s'\n", item.Id, nickname.c_str());

    if (!item.Id) {
        Printf(LOG_Error, "No reward\n");
        return;
    }

    item_list.Items.push_back(std::move(item));
    std::string new_bag = Login_SerializeItems(item_list);

    SimpleSQL insert{Format("UPDATE characters SET bag = '%s' WHERE nick = '%s' AND deleted = 0;", SQL_Escape(new_bag).c_str(), SQL_Escape(nickname).c_str())};
    if (!query) {
        Printf(LOG_Error, "Failed to update\n");
        return;
    }

    Printf(LOG_Info, "Done!\n");
}

bool H_Init(int argc, char* argv[])
{
    atexit(H_Quit);
    SetExceptionFilter();

    SetConsoleTitle("Red Hat");

    HANDLE wHnd = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT windowSize = {0, 0, 119, 49};
    COORD bufferSize = {120, 300};

    // Change the console window size:
    SetConsoleScreenBufferSize(wHnd, bufferSize);
    SetConsoleWindowInfo(wHnd, TRUE, &windowSize);

    if(!ReadConfig("redhat.cfg")) return false;
    if(!SQL_Init()) return false;

    if(!Login_UnlockAll())
        Printf(LOG_Warning, "[HC] Unable to hat-unlock all logins.\n");

    bool exit_ = false;
    for(int i = 0; i < argc; i++)
    {
        std::string arg = argv[i];
        if(arg == "-droptables")
        {
            SQL_DropTables();
            exit_ = true;
        }
        else if(arg == "-createtables")
        {
            SQL_CreateTables();
            exit_ = true;
        }
        else if(arg == "-convertlgn" && i != argc-1)
        {
            std::string arg2 = argv[i+1];
            LGN_DBConvert(arg2);
            exit_ = true;
        }
        else if(arg == "-updatetables")
        {
            SQL_UpdateTables();
            exit_ = true;
        } else if (arg.find("-give-circle-reward=") == 0) {
            // Format: `-give-circle-reward=coolnickname:1` will give the first circle reward to the character with nickname `coolnickname`.
            GiveCircleReward(arg.substr(strlen("-give-circle-reward=")));
            exit_ = true;
        } else if (arg == "-create-table-checkpoint") {
            SQL_CreateTableCheckpoint();
            exit_ = true;
        }
    }
    if(exit_) return false;

    Printf(LOG_Info, "[HC] Red Hat (v1.3) started.\n");

    Net_Init();
    
    try {
        Printf(LOG_Info, "[thresholds] Loading thresholds\n");
#include "thresholds.generated.h"
        thresholds::thresholds.LoadFromContent(default_thresholds);

        Printf(LOG_Info, "[thresholds] Saving thresholds for servers\n");
        std::ofstream f_out("thresholds.cfg");
        f_out.write(default_thresholds, strlen(default_thresholds));
        if (!f_out) {
            throw new std::exception("failed to write threshold settings to thresholds.cfg");
        }
        f_out.close();

        Printf(LOG_Info, "[thresholds] Thresholds OK\n");
    } catch(const thresholds::ParseException& e) {
        Printf(LOG_Error, "Thresholds: %s\n", e.what());
        return false;
    }

    return true;
}

void H_Process()
{
    while(true)
    {
        Net_Listen();
        ST_Generate();
        Sleep(1);
    }
}

int main(int argc, char* argv[])
{
    if(!H_Init(argc, argv)) return 1;
    H_Process();
    return 0;
}
