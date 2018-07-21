
#include "y_array.hpp"
#include "catch.hpp"

#include <cstdio>
#include <random>
#include <utility>

using namespace y;

#if 0
static_assert(sizeof(FixedVector<char, 1>    ) == 1 +     1, "");
static_assert(sizeof(FixedVector<char, 254>  ) == 1 +   254, "");
static_assert(sizeof(FixedVector<char, 255>  ) == 1 +   255, "");
static_assert(sizeof(FixedVector<char, 256>  ) == 2 +   256 + 0, "");
static_assert(sizeof(FixedVector<char, 257>  ) == 2 +   257 + 1, "");
static_assert(sizeof(FixedVector<char, 65534>) == 2 + 65534 + 0, "");
static_assert(sizeof(FixedVector<char, 65535>) == 2 + 65535 + 1, "");
static_assert(sizeof(FixedVector<char, 65536>) == 4 + 65536 + 0, "");
static_assert(sizeof(FixedVector<char, 65537>) == 4 + 65537 + 3, "");
#endif

template <typename T>
RangeSize Find (RangeRd<T> range, T const & key, RangeSize start_from = 0) {
    for (RangeSize i = start_from; i < range.size; ++i)
        if (range[i] == key)
            return i;
    return range.size;
}

//template <typename T>
//struct Unref {using Type = T;};
//
//template <typename T>
//struct Unref<T &> {using Type = T;};
//
//template <typename T>
//struct Unref<T &&> {using Type = T;};

template <typename T>
void Swap (T & a, T & b) {
    auto t = std::move(a);
    a = std::move(b);
    b = std::move(t);
}

//template <typename T>
//void Swap (T const & a, T const & b) = delete;

template <typename T>
RangeSize Partition (RangeWr<T> range, RangeSize index_of_pivot) {
    RangeSize ret = InvalidRangeSize;
    if (!range.empty() && index_of_pivot >= 0 && index_of_pivot < range.size()) {
        RangeSize pivot = range.size() - 1;
        if (index_of_pivot != pivot)
            Swap(range[index_of_pivot], range[pivot]);
        RangeSize p = 0;
        for (RangeSize i = 0; i < pivot; ++i) {
            if (range[i] < range[pivot]) {  // Should it be "<=" ?
                Swap(range[p], range[i]);
                p += 1;
            }
        }
        Swap(range[p], range[pivot]);
        ret = p;
    }
    return ret;
}

template <typename T>
void Sort (RangeWr<T> range) {
    
}

template <typename T>
bool IsSorted (RangeRd<T> const range) {
    for (RangeSize i = 1; i < range.size(); ++i)
        if (range[i] < range[i - 1])
            return false;
    return true;
}

void RandomFill (RangeWr<int> range, int low_inc, int high_exc) {
    std::random_device rd;
    for (auto & e : range)
        e = low_inc + (rd() % (high_exc - low_inc));
}

void Print (RangeRd<int> range) {
    for ( ; !range.empty(); range.pop_front())
        ::printf("%d ", range.front());
    ::printf("\n");
}

TEST_CASE("Array Construction", "[array]") {
    Array<int, 100> a;

    REQUIRE(a.size() == decltype(a)::Size);
    REQUIRE(a.size() == 100);
	REQUIRE(a.all().size() == a.size());
    REQUIRE(a.all().data() == a.data());
    REQUIRE(a.all().size() == 100);

    RandomFill(a.all(), -10'000, 10'000);
    //Print(a.all());
    //::printf("\n");
    CHECK_FALSE(IsSorted(a.all().rd()));
}

TEST_CASE("Partition 1", "[basics]") {
    for (int _ = 0; _ < 10; ++_) {
        Array<int, 20> a;

        RandomFill(a.all(), -1000, 1000);
        CHECK_FALSE(IsSorted(a.all().rd()));

        int c1 = 0, c2 = 0;
        //Print(a.all());
        auto p = Partition(a.all(), 0);
        //::printf("%zu\n", p);
        //Print(a.all());
        for (y::RangeSize i = 0; i < p; ++i)
            if (a[i] > a[p])
                c1 += 1;
        CHECK(c1 == 0);
        for (y::RangeSize i = p + 1; i < a.size(); ++i)
            if (a[i] < a[p])
                c2 += 1;
        CHECK(c2 == 0);
    }
}

TEST_CASE("Partition 2", "[basics]") {
    for (int _ = 0; _ < 10; ++_) {
        Array<int, 20> a;

        RandomFill(a.all(), -5, 5);
        CHECK_FALSE(IsSorted(a.all().rd()));

        int c1 = 0, c2 = 0;
        //Print(a.all());
        auto p = Partition(a.all(), 0);
        //::printf("%zu\n", p);
        //Print(a.all());
        for (y::RangeSize i = 0; i < p; ++i)
            if (a[i] > a[p])
                c1 += 1;
        CHECK(c1 == 0);
        for (y::RangeSize i = p + 1; i < a.size(); ++i)
            if (a[i] < a[p])
                c2 += 1;
        CHECK(c2 == 0);
    }
}
