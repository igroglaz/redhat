#include "UnitTest++.h"

#include "../merge_items.hpp"
#include "../kill_stats.h"

namespace
{

// A couple helper functions to represent binary data as hex strings.
namespace helpers
{

uint8_t HexChar(char c) {
    CHECK(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));
    return c <= '9' ? c - '0' : c - 'A' + 10;
}

char HalfCharHex(char c) {
    CHECK(c <= 15);
    return c <= 9 ? c + '0' : c - 10 + 'A';
} 

BinaryStream FromHex(const std::string& data) {    
    BinaryStream stream;
    for (size_t i = 0; i < data.length() / 2; ++i) {
        stream.WriteUInt8(HexChar(data[2*i]) * 16 + HexChar(data[2*i+1]));
    }
    return stream;
}

std::string ToHex(BinaryStream& stream) {
    std::string data;
    data.reserve(stream.GetLength() * 2);

    for (uint32_t i = 0; i < stream.GetLength(); ++i) {
        uint8_t b = stream.ReadUInt8();
        data.push_back(HalfCharHex(b >> 4));
        data.push_back(HalfCharHex(b & 0xF));
    }

    return data;
}

} // namespace helpers

TEST(Hex) {
    std::string s = "00010A0FABFF";
    BinaryStream b = helpers::FromHex(s);
    std::string got = helpers::ToHex(b);
    CHECK_EQUAL(s, got);
}

TEST(Empty) {
    std::string data = "000A0000FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF009400";
    BinaryStream stream = helpers::FromHex(data);

    KillStats kill_stats;
    bool success = kill_stats.Unmarshal(stream);
    CHECK_EQUAL(true, success);

    for (size_t i = 0; i < kill_stats.by_server_id.size(); ++i) {
        CHECK_EQUAL(0, kill_stats.by_server_id[i]);
    }

    BinaryStream output;
    success = kill_stats.Marshal(output);
    CHECK_EQUAL(true, success);

    std::string converted = helpers::ToHex(output);
    CHECK_EQUAL(data, converted);
}

TEST(Specific) {
    std::string data = "000A0000FF00FF00FF00FF00EC0001038F000101FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF009600";
    BinaryStream stream = helpers::FromHex(data);

    KillStats kill_stats;
    bool success = kill_stats.Unmarshal(stream);
    CHECK_EQUAL(true, success);

    CHECK_EQUAL(3, kill_stats.by_server_id[616]);
    CHECK_EQUAL(1, kill_stats.by_server_id[632]);

    BinaryStream output;
    success = kill_stats.Marshal(output);
    CHECK_EQUAL(true, success);

    std::string converted = helpers::ToHex(output);
    CHECK_EQUAL(data, converted);
}

TEST(Lots) {
    std::string data = "000A0000020001FF00FF00FF00FF00DA00930E0103830E01048C0E8200820E8200820E8200820E030C0E048200830E01008F0E040A0E000E830003020006830002050284000C050E08070806030401070001870E05010D090B04850ED100820E8500020200830E8D00010BFF00C100010EFF00FF00FF00FF00FF00FF00FF00FF00C0000802070C07030E020E87000309000A8300010E8200040E0402068500040B0907088A000101FF00E300820E0103FF00B800";
    BinaryStream stream = helpers::FromHex(data);

    KillStats kill_stats;
    bool success = kill_stats.Unmarshal(stream);
    CHECK_EQUAL(true, success);

    BinaryStream output;
    success = kill_stats.Marshal(output);
    CHECK_EQUAL(true, success);

    std::string converted = helpers::ToHex(output);
    CHECK_EQUAL(data, converted);
}

}
