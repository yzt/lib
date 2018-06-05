#include "y_json.h"

#include <chrono>
#include <cstdio>

static double Now () {
    using namespace std::chrono;
    return duration_cast<duration<double>>(high_resolution_clock::now().time_since_epoch()).count();
}

int main () {
    return 0;
}

