
#include "../experimental/y_format.hpp"
#include "catch.hpp"
#include <string>
using namespace std::string_literals;

TEST_CASE("C-str to C-str 01", "[fmt]") {
    char buffer [15];
    auto r = y::fmt::c2c(buffer, sizeof(buffer), "Hello, world!");
    REQUIRE(r == 14);
    REQUIRE(buffer[r - 2] != '\0');
    REQUIRE(buffer[r - 1] == '\0');
    REQUIRE("Hello, world!"s == buffer);
}

TEST_CASE("C-str to C-str 02", "[fmt]") {
    char buffer [20];
    auto r = y::fmt::c2c(buffer, 10, "Hello, world!");
    REQUIRE(r == 10);
    REQUIRE(buffer[r - 2] != '\0');
    REQUIRE(buffer[r - 1] == '\0');
    REQUIRE("Hello, wo"s == buffer);
}

TEST_CASE("C-str to C-str 03", "[fmt]") {
    char buffer [100];
    auto r = y::fmt::c2c(buffer, sizeof(buffer), "Hello, {{world}!");
    REQUIRE(r == 16);
    REQUIRE(buffer[r - 2] != '\0');
    REQUIRE(buffer[r - 1] == '\0');
    REQUIRE("Hello, {world}!"s == buffer);

    r = y::fmt::c2c(buffer, sizeof(buffer), "Hello, {}!", "Kevin");
    REQUIRE(r == 14);
    REQUIRE(buffer[r - 2] != '\0');
    REQUIRE(buffer[r - 1] == '\0');
    REQUIRE("Hello, Kevin!"s == buffer);

    r = y::fmt::c2c(buffer, sizeof(buffer), "Tes{}, {}... {}... {}... {{{}}", "ting", true, 2, -3, "*does the test*"s);
    REQUIRE(r == 43);
    REQUIRE(buffer[r - 2] != '\0');
    REQUIRE(buffer[r - 1] == '\0');
    REQUIRE("Testing, 1... 2... -3... {*does the test*}"s == buffer);

    r = y::fmt::c2c(buffer, sizeof(buffer), "{},{},{}\n", 3.14159265358979323846264338328f, 3.14159265358979323846264338328, 3.14159265358979323846264338328L);
    ::printf(buffer);
}
