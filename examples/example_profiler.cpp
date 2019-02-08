//#define DISABLE_PROFILING
#include "y_profiler.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

std::string RandStr () {
    YP_SCOPE("RandStr");
    int len = rand() % 20 + 1;
    std::string ret (len, '-');
    for (int i = 0; i < len; ++i)
        ret[i] = 'A' + rand() % 26;
    return ret;
}

bool CompareStr (std::string const & a, std::string const & b) {
    YP_SCOPE("String Comparator");
    if (a.size() == b.size())
        return a < b;
    else
        return a.size() < b.size();
}

int main () {
    YP_SCOPE("main");

    auto t0 = std::chrono::steady_clock::now();
    std::vector<std::string> a;
    a.reserve(1'000'000);
    for (int i = 0; i < 1'000'000; ++i)
        a.emplace_back(RandStr());

    std::sort(a.begin(), a.end(), CompareStr);
    std::cout << a.front() << '\n';

    auto t1 = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count() << '\n';
    return 0;
}
