
#include <y_basics.hpp>
#include "catch.hpp"

TEST_CASE("Basics Basics 1", "[basics]") {
    y::Size x = 0;
    REQUIRE(x == 0);
    x--;
    REQUIRE(x > 0);

    y::Optional<y::Blob> y;
    REQUIRE_FALSE(y.has_value());
    REQUIRE_THROWS(y.value());
    y.emplace(nullptr, nullptr);
    REQUIRE(y.has_value());

    y::Pair p (3, 3.2f);
    y::Triplet t (p, -1, false);
}

TEST_CASE("Basics Defer 1", "[basics]") {
    int test = 0;
    {
        Y_DEFER {REQUIRE(test == 3); test += 4;};
        Y_DEFER {REQUIRE(test == 1); test += 2;};
        REQUIRE(test == 0);
        test += 1;
        REQUIRE(test == 1);
    }
    REQUIRE(test == 7);
}
