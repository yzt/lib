#include "y_json.h"

#include <chrono>
#include <cstdio>

static double Now () {
    using namespace std::chrono;
    return duration_cast<duration<double>>(high_resolution_clock::now().time_since_epoch()).count();
}

char simple_json [] = R"(
{
    "name": null,
    "age"          :42,
    "nope":[false, 1.8, -12, .09, "xnay", {}]
}
)";

json_user_handle_t ElemPrinter (
    json_elem_type_e elem_type,
    json_user_handle_t self,
    json_value_t value,
    json_user_handle_t parent,
    json_buffer_t const * input,
    json_location_t const * location,
    void * user_data
) {
    return nullptr;
}

void ErrorPrinter (
    json_error_severity_e severity,
    json_error_e error,
    json_buffer_t const * input,
    json_location_t const * location,
    void * user_data,
    char const * msg,
    int param1,
    int param2
) {
}

int main () {
    auto t0 = Now();
    JSON_Parse({simple_json, sizeof(simple_json) - 1}, ElemPrinter, ErrorPrinter, nullptr);
    auto t1 = Now();
    ::printf("%d nanosecs\n", int(0.5 + (t1 - t0) * 1'000'000'000));

    return 0;
}

