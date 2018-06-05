#include "y_json.h"

#include <cassert>

#define Y_ASSERT(cond, ...)         assert(cond)
#define Y_ASSERT_STRONG(cond, ...)  assert(cond)

static inline bool
LocAdvance (
    json_buffer_t in,
    json_location_t & loc
) {
    bool ret = false;
    if (!loc.eoi && loc.byte < in.size) {
        if ('\n' == in.ptr[loc.byte]) {
            loc.line += 1;
            loc.column = 1;
        } else {
            loc.column += 1;
        }
        loc.byte += 1;
        if (loc.byte < in.size)
            ret = true;
        else
            loc.eoi = true;
    }
    return ret;
}

// Note: this is too simplistic. There are UNICODE whitespace characters not considered here.
static inline bool
IsWhitespace (
    char c
) {
    return ' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c || '\v' == c;
}

static inline bool
SkipWhitespace (
    json_buffer_t const & in,
    json_location_t & loc
) {
    bool ret = false;
    ret = (!loc.eoi && !loc.has_error);
    while (!loc.eoi && IsWhitespace(in.ptr[loc.byte]))
        LocAdvance(in, loc);
    return ret;
}

static json_size_t
ParseObject (
    json_buffer_t const & in,
    json_location_t & loc,
    json_user_handle_t parent,
    json_element_f elem_cb,
    json_error_f error_cb,
    void * user_data
) {
    Y_ASSERT(in.ptr[loc.byte] == '{');
    auto obj_handle = elem_cb(
        JSON_ETYPE_ObjectBegin, JSON_VTYPE_INVALID, {},
        parent, &in, &loc, user_data
    );
    loc.depth += 1;
    LocAdvance(in, loc);
    
    bool done = false;
    int child_count = 0;

    if (!SkipWhitespace(in, loc)) {
        error_cb(JSON_SEV_Fatal, JSON_ERR_IncompleteInput, &in, &loc, user_data, "Found a '{' but nothing after");
        return false;
    }

    char c = in.ptr[loc.byte];
    if ('}' != c) {

        while (!done && !cur.eoi && !cur.has_error) {
        
            char c = in.ptr[cur.byte];

            if (IsWhitespace(c)) {
                // Do nothing!
            } else if (child_count <= 0) {  // first
                if ('"' == c) {
                    ParseNVP();    
                } else if (',' == c) {
                done = true;
                } else if ('}' == c) {
                done = true;
                }
            } else {    // !first
            }
            LocAdvance(in, cur);
        }
        
    }
    
    Y_ASSERT('}' == c);
    elem_cb(...);

    return done;
}

json_size_t
JSON_Parse (
    json_buffer_t in,
    json_element_f elem_cb,
    json_error_f error_cb,
    void * user_data
) {
    json_location_t cur = {0, 0, 1, 0, false, false};

    if (!error_cb) {
        return 0;
    }

    if (!elem_cb) {
        error_cb(JSON_SEV_Fatal, JSON_ERR_BadParams, cur, in, user_data, "The element handling callback must be valid");
        return 0;
    

    if (!elem_cb || (!in.ptr && in.size > 0)) {
        error_cb(JSON_SEV_Fatal, JSON_ERR_BadParams, cur, in, user_data, "Invalid input (no pointer with size > 0)"); 
        return 0;
    }

    while (cur.byte < in.size) {
        
    }

    return cur.byte;
}
