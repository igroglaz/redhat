#define _GLIBCXX_HAVE_STDINT_H 1

#include <stdint.h>

#include "UnitTest++.h"

#include "../config.hpp"
#include "../thresholds.h"
#include "../utils.hpp"

int main() {
    Config::LogLevel = LOG_Silent; // Silence all logs for all tests.

    try {
#include "thresholds.generated.h"
        thresholds::thresholds.LoadFromContent(default_thresholds);
    } catch (const thresholds::ParseException& e) {
        printf("Thresholds: %s\n", e.what());
        return 1;
    }

    return UnitTest::RunAllTests();
}
