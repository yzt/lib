
#include "y_stats.hpp"
#include "catch.hpp"

#include <cmath>
//#include <iostream>

TEST_CASE("Bunch Basics 1", "[stats]") {
    y::Stats::Bunch<> b;

    REQUIRE(b.empty());
    REQUIRE(b.count() == 0);
    REQUIRE(b.min() == INFINITY);
    REQUIRE(isinf(b.min()));
    REQUIRE(b.min() > 0);
    REQUIRE(b.max() == -INFINITY);
    REQUIRE(isinf(b.max()));
    REQUIRE(b.max() < 0);
    REQUIRE(b.sum() == 0);
    REQUIRE(isnan(b.mean()));
}

TEST_CASE("Bunch Basics 2", "[stats]") {
    y::Stats::Bunch<> b;

    b.addSample(2.5);
    b.addSample(2.5);
    b.addSample(2.5);
    b.addSample(-1.5);
    b.addSample(0);
    b.addSample(3);

    REQUIRE_FALSE(b.empty());
    REQUIRE(b.count() == 6);
    REQUIRE(b.min() == -1.5);
    REQUIRE(b.max() == 3.0);
    REQUIRE(b.sum() == 9.0);
    REQUIRE(b.mean() == 1.5);
}

TEST_CASE("Track Basics 1", "[stats]") {
    y::Stats::Track<30> t;
}