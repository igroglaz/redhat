// Packet-level network logging for the HAT - see netlog.hpp.
#include "netlog.hpp"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <windows.h>

namespace netlog
{
    static FILE* open_file()
    {
        static FILE* f = NULL;
        static bool tried = false;
        if(tried) return f;
        tried = true;

        // Opt-in: only log when the command line carries --net-log=DIRECTORY
        // (the value ends at whitespace or a quote).
        const char* cli = GetCommandLineA();
        const char* flag = cli ? strstr(cli, "--net-log=") : NULL;
        if(!flag) return NULL;
        flag += strlen("--net-log=");

        char dir[MAX_PATH] = {0};
        size_t n = 0;
        while(*flag && *flag != ' ' && *flag != '\t' && *flag != '"')
        {
            if(n < sizeof(dir)-1) dir[n++] = *flag;
            flag++;
        }
        if(!n) return NULL;

        char path[MAX_PATH];
        _snprintf(path, sizeof(path)-1, "%s\\hat.log", dir);

        char dirbuf[MAX_PATH];
        strncpy(dirbuf, path, sizeof(dirbuf)-1);
        char* slash = strrchr(dirbuf, '\\');
        if(slash) { *slash = 0; CreateDirectoryA(dirbuf, NULL); }

        f = fopen(path, "a");
        if(f)
        {
            SYSTEMTIME tm;
            GetLocalTime(&tm);
            fprintf(f, "=== hat session start %04u-%02u-%02u %02u:%02u:%02u pid=%lu ===\n",
                    tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond,
                    GetCurrentProcessId());
            fflush(f);
        }
        return f;
    }

    static void write_head(FILE* f, const char* dir)
    {
        SYSTEMTIME tm;
        GetLocalTime(&tm);
        fprintf(f, "[%02u:%02u:%02u.%03u] %-8s ", tm.wHour, tm.wMinute, tm.wSecond,
                tm.wMilliseconds, dir);
    }

    void logf(const char* dir, const char* fmt, ...)
    {
        FILE* f = open_file();
        if(!f) return;
        write_head(f, dir);
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fputc('\n', f);
        fflush(f);
    }

    void hex(const char* dir, const void* data, unsigned long len, const char* fmt, ...)
    {
        FILE* f = open_file();
        if(!f) return;
        write_head(f, dir);
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fprintf(f, " (%lu bytes)\n", len);

        const unsigned char* d = (const unsigned char*)data;
        unsigned long cap = len < 400 ? len : 400;
        for(unsigned long i = 0; i < cap; i += 16)
        {
            fprintf(f, "             %04lx  ", i);
            for(unsigned long j = 0; j < 16; j++)
            {
                if(i + j < cap) fprintf(f, "%02x ", d[i+j]);
                else fputs("   ", f);
            }
            fputs(" |", f);
            for(unsigned long j = 0; j < 16 && i + j < cap; j++)
            {
                unsigned char c = d[i+j];
                fputc(c >= 0x20 && c < 0x7f ? c : '.', f);
            }
            fputs("|\n", f);
        }
        if(len > cap) fprintf(f, "             ... (%lu more bytes)\n", len - cap);
        fflush(f);
    }
}
