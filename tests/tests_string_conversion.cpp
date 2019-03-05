
#include "../experimental/y_string_conversion.hpp"
#include "catch.hpp"

#include <string>
using namespace std::string_literals;

TEST_CASE("Basics 1", "[cvt]") {
    char buffer [100];

    auto res = y::cvt::i2s(buffer, sizeof(buffer), 42U, 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 2);
    CHECK(buffer == "42"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), 42, 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 2);
    CHECK(buffer == "42"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), -42, 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 3);
    CHECK(buffer == "-42"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), 42U, 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 2);
    CHECK(buffer == "42"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), (signed char)(0), 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), (unsigned char)(0), 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), (signed short)(0), 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), (unsigned short)(0), 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), (signed int)(0), 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), (unsigned int)(0), 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), (signed long)(0), 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), (unsigned long)(0), 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), (signed long long)(0), 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), (unsigned long long)(0), 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), 0, 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 1);
    CHECK(buffer == "0"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), 100000000, 10, false, 3, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 11);
    CHECK(buffer == "100'000'000"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), 1048576 + 256 + 12, 16, false, 4, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 7);
    CHECK(buffer == "10'010c"s);

    res = y::cvt::i2s(buffer, sizeof(buffer), 1048576 + 256 + 12, 16, true, 4, '\'', true);
    CHECK(res.succeeded);
    CHECK(res.size == 7);
    CHECK(buffer == "10'010C"s);
}
