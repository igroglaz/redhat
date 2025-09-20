#include <fstream>

#include "config.hpp"
#include "socket.hpp"
#include "sql.hpp"
#include "utils.hpp"
#include "listener.hpp"
#include "status.hpp"
#include "login.hpp"
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
        } else if (arg == "-create-table-checkpoint") {
            SQL_CreateTableCheckpoint();
            exit_ = true;
        } else if (arg == "-fix-shelves") {
            SQL_FixShelves();
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
