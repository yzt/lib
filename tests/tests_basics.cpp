
#include "y_basics.hpp"
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
}
