
#include "y_array.hpp"
#include "catch.hpp"

#if 0
static_assert(sizeof(y::FixedVector<char, 1>    ) == 1 +     1, "");
static_assert(sizeof(y::FixedVector<char, 254>  ) == 1 +   254, "");
static_assert(sizeof(y::FixedVector<char, 255>  ) == 1 +   255, "");
static_assert(sizeof(y::FixedVector<char, 256>  ) == 2 +   256 + 0, "");
static_assert(sizeof(y::FixedVector<char, 257>  ) == 2 +   257 + 1, "");
static_assert(sizeof(y::FixedVector<char, 65534>) == 2 + 65534 + 0, "");
static_assert(sizeof(y::FixedVector<char, 65535>) == 2 + 65535 + 1, "");
static_assert(sizeof(y::FixedVector<char, 65536>) == 4 + 65536 + 0, "");
static_assert(sizeof(y::FixedVector<char, 65537>) == 4 + 65537 + 3, "");
#endif

template <typename T>
y::RangeSize Find (y::RangeRd<T> range, T const & key, y::RangeSize start_from = 0) {
    for (RangeSize i = start_from; i < range.size; ++i)
        if (range[i] == key)
            return i;
    return range.size;
}

template <typename T>
void Sort (y::RangeWr<T> range) {
    
}

TEST_CASE("Construction", "[Array]") {
    y::Array<int, 100> a;

    REQUIRE(a.size() == decltype(a)::Size);
    REQUIRE(a.size() == 100);
}
