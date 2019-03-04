
#include <y_format.hpp>
#include "catch.hpp"
#include <string>
using namespace std::string_literals;

TEST_CASE("C-str to C-str 01", "[fmt]") {
    char buffer [15];
    auto r = y::fmt::ToCStr(buffer, sizeof(buffer), "Hello, world!");
    CHECK(r == 14);
    CHECK(buffer[r - 2] != '\0');
    CHECK(buffer[r - 1] == '\0');
    CHECK("Hello, world!"s == buffer);
}

TEST_CASE("C-str to C-str 02", "[fmt]") {
    char buffer [20];
    auto r = y::fmt::ToCStr(buffer, 10, "Hello, world!");
    CHECK(r == 10);
    CHECK(buffer[r - 2] != '\0');
    CHECK(buffer[r - 1] == '\0');
    CHECK("Hello, wo"s == buffer);
}

TEST_CASE("C-str to C-str 03", "[fmt]") {
    char buffer [100];
    auto r = y::fmt::ToCStr(buffer, sizeof(buffer), "Hello, {{world}!");
    CHECK(r == 16);
    CHECK(buffer[r - 2] != '\0');
    CHECK(buffer[r - 1] == '\0');
    CHECK("Hello, {world}!"s == buffer);

    r = y::fmt::ToCStr(buffer, sizeof(buffer), "Hello, {}!", "Kevin");
    CHECK(r == 14);
    CHECK(buffer[r - 2] != '\0');
    CHECK(buffer[r - 1] == '\0');
    CHECK("Hello, Kevin!"s == buffer);

    r = y::fmt::ToCStr(buffer, sizeof(buffer), "Tes{}, {}... {}... {}... {{{}}", "ting", true, 2, -3, "*does the test*"s);
    CHECK(r == 43);
    CHECK(buffer[r - 2] != '\0');
    CHECK(buffer[r - 1] == '\0');
    CHECK("Testing, 1... 2... -3... {*does the test*}"s == buffer);

    r = y::fmt::ToConsole("{},{},{}\n", 3.14159265358979323846264338328f, 3.14159265358979323846264338328, 3.14159265358979323846264338328L);

    char cs1 [10] = "Hellop";
    int * p = nullptr;
    y::fmt::ToConsole("{},0x{}\n", cs1, p);

    ::printf("%10d,%-10d\n", 7, 7);
}

TEST_CASE("Formatting with explicit argument numbers", "[fmt]") {
    auto s = y::fmt::ToStr("{1}{0}{3}", 'a', 'b', 'c', 'd');
    CHECK(s == "bad");
}

TEST_CASE("Format Specs Galore!", "[fmt]") {
    auto s = y::fmt::ToStr("Only {0w13f-b10+rU}, and that's it.", 3.14);
    CHECK(s == "Only ---------3.14, and that's it.");
    s = y::fmt::ToStr("Only {0w13f-r}, and that's it.", 3.14);
    CHECK(s == "Only ---------3.14, and that's it.");
    s = y::fmt::ToStr("Only {0w13r}, and that's it.", 3.14);
    CHECK(s == "Only          3.14, and that's it.");
    s = y::fmt::ToStr("Only {0w13C}, and that's it.", 3.14);
    CHECK(s == "Only      3.14    , and that's it.");
    s = y::fmt::ToStr("Only {0w13c}, and that's it.", 3.14);
    CHECK(s == "Only     3.14     , and that's it.");
}
