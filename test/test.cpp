#define _GLIBCXX_HAVE_STDINT_H 1

#include <stdint.h>

#include "UnitTest++.h"

#include "../config.hpp"
#include "../utils.hpp"

int main() {
    Config::LogLevel = LOG_Silent; // Silence all logs for all tests.

    return UnitTest::RunAllTests();
}
