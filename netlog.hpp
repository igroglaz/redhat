// Packet-level network logging for the HAT: opt-in via the command line
// --net-log=DIRECTORY, appends to DIRECTORY\hat.log. Without the flag
// nothing is logged. Best-effort only.
#pragma once

#include <string>

namespace netlog
{
    void logf(const char* dir, const char* fmt, ...);
    void hex(const char* dir, const void* data, unsigned long len, const char* fmt, ...);
}
