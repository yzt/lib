
#include "../experimental/y_map.hpp"
#include "catch.hpp"

#include <iostream>

TEST_CASE("Map Construction", "[map]") {
    constexpr unsigned Cap = 1000;
    double values [Cap];
    double keys [Cap];
    y::Map<double, double, uint32_t>::Metadata meta [Cap];

    y::Map map (Cap, keys, values, meta);
    REQUIRE(map.capacity() == Cap);
    REQUIRE(map.count() == 0);

//    y::Map_Clear(map);
}
