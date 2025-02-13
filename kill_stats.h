#pragma once

#include <array>
#include <cstdint>
#include "BinaryStream.hpp"

struct KillStats {
    std::array<uint8_t, 2560> by_server_id;

    bool Unmarshal(BinaryStream& section55555555);
    bool Marshal(BinaryStream& section55555555);
};
