#pragma once

#if !defined(Y_JSON_H_INCLUDE_GUARD_)
    #define  Y_JSON_H_INCLUDE_GUARD_

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned json_size_t;
typedef void * json_user_handle_t;

typedef struct {
    char const * ptr;
    json_size_t size;
} json_buffer_t;

typedef struct {
    json_size_t start;
    json_size_t size;
} json_range_t;

typedef struct {
    int depth;
    json_size_t byte;
    json_size_t line;
    json_size_t column;
    bool eoi;
    bool has_error;
} json_location_t;

typedef union {
    bool b;
    long long i;
    double r;
    json_range_t s;    // This is NOT a NUL-terminated string!
} json_value_u;

typedef enum {
    JSON_ETYPE_INVALID = 0,
    JSON_ETYPE_Begin,
    JSON_ETYPE_End,
    JSON_ETYPE_ObjectBegin,  // loc, name?
    JSON_ETYPE_ObjectEnd,    // loc, name?, member count, handle
    JSON_ETYPE_ArrayBegin,   // loc, name?
    JSON_ETYPE_ArrayEnd,     // loc, name?, member count, handle
    JSON_ETYPE_Name,         // loc, name
    JSON_ETYPE_Value,
} json_elem_type_e;

typedef enum {
    JSON_VTYPE_INVALID = 0,
    JSON_VTYPE_Null,
    JSON_VTYPE_Bool,
    JSON_VTYPE_Int,
    JSON_VTYPE_Real,
    JSON_VTYPE_String,
    JSON_VTYPE_Array,
    JSON_VTYPE_Object,
} json_value_type_e;

typedef enum {
    JSON_SEV_INVALID = 0,
    JSON_SEV_Pedantic,
    JSON_SEV_Fatal,
} json_error_severity_e;

typedef enum {
    JSON_ERR_INVALID = 0,
    JSON_ERR_BadParams,
    JSON_ERR_IncompleteInput,
    JSON_ERR_MissingEnclosingBrace, // pedantic
} json_error_e;

typedef json_user_handle_t (*json_element_f) (
    json_elem_type_e elem_type,
    json_value_type_e value_type,
    json_value_u value,
    json_user_handle_t parent,
    json_buffer_t const * input,
    json_location_t const * location,
    void * user_data
);

typedef bool (*json_error_f) (
    json_error_severity_e severity,
    json_error_e error,
    json_buffer_t const * input,
    json_location_t const * location,
    void * user_data,
    char const * msg
);

// This is a "SAX" style parser, for those old-enough to remember!
// Takes the JSON it's supposed to parse, and returns the number of bytes
//  that it consumed.
// For each element it encounters in the JSON, it calls the elem_cb.
// For each error, the error_cb is called.
json_size_t JSON_Parse (
    json_buffer_t input,
    json_element_f elem_cb,
    json_error_f error_cb,
    void * user_data
);

#if defined(__cplusplus)
}   // extern "C"
#endif

#endif  // Y_JSON_H_INCLUDE_GUARD_
