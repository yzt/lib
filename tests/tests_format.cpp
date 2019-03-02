
#include "catch.hpp"
#include <y_format.hpp>
#include <string>
using namespace std::string_literals;

TEST_CASE("C-str to C-str 01", "[fmt]") {
    char buffer [15];
    auto r = y::fmt::ToCStr(buffer, sizeof(buffer), "Hello, world!");
    REQUIRE(r == 14);
    REQUIRE(buffer[r - 2] != '\0');
    REQUIRE(buffer[r - 1] == '\0');
    REQUIRE("Hello, world!"s == buffer);
}

TEST_CASE("C-str to C-str 02", "[fmt]") {
    char buffer [20];
    auto r = y::fmt::ToCStr(buffer, 10, "Hello, world!");
    REQUIRE(r == 10);
    REQUIRE(buffer[r - 2] != '\0');
    REQUIRE(buffer[r - 1] == '\0');
    REQUIRE("Hello, wo"s == buffer);
}

TEST_CASE("C-str to C-str 03", "[fmt]") {
    char buffer [100];
    auto r = y::fmt::ToCStr(buffer, sizeof(buffer), "Hello, {{world}!");
    REQUIRE(r == 16);
    REQUIRE(buffer[r - 2] != '\0');
    REQUIRE(buffer[r - 1] == '\0');
    REQUIRE("Hello, {world}!"s == buffer);

    r = y::fmt::ToCStr(buffer, sizeof(buffer), "Hello, {}!", "Kevin");
    REQUIRE(r == 14);
    REQUIRE(buffer[r - 2] != '\0');
    REQUIRE(buffer[r - 1] == '\0');
    REQUIRE("Hello, Kevin!"s == buffer);

    r = y::fmt::ToCStr(buffer, sizeof(buffer), "Tes{}, {}... {}... {}... {{{}}", "ting", true, 2, -3, "*does the test*"s);
    REQUIRE(r == 43);
    REQUIRE(buffer[r - 2] != '\0');
    REQUIRE(buffer[r - 1] == '\0');
    REQUIRE("Testing, 1... 2... -3... {*does the test*}"s == buffer);

    r = y::fmt::ToConsole("{},{},{}\n", 3.14159265358979323846264338328f, 3.14159265358979323846264338328, 3.14159265358979323846264338328L);

    char cs1 [10] = "Hellop";
    int * p = nullptr;
    y::fmt::ToConsole("{},0x{}\n", cs1, p);

    ::printf("%10d,%-10d\n", 7, 7);
}
