#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned int y_bignum_dig_t;

typedef struct {
    y_bignum_dig_t * digs;
    unsigned len;
    unsigned cap;
} y_bignum_raw_t;

typedef struct {
    //int shift;
    //int sign;
} y_bignum_meta_t;

typedef struct {
    //y_bignum_raw_t digs;
    //y_bignum_meta_t meta;
    y_bignum_dig_t * digs;
    int size;
    //int shift;
    //int sign;
} y_bignum_num_t;


bool y_bignum_alloc (y_bignum_num_t * num, int capacity_in_words, bool clear_to_zero);
bool y_bignum_realloc (y_bignum_num_t * num, int new_capacity_in_words, bool clear_to_zero);
void y_bignum_free (y_bignum_dig_t digs);

bool y_bignum_copy (y_bignum_num_t * dst, y_bignum_num_t const * src);
void y_bignum_trim (y_bignum_num_t * num, bool free_excess_memory);

// These all do allocate and initialize a bignum
bool y_bignum_init (y_bignum_num_t * num, y_bignum_dig_t v);
//bool y_bignum_init (y_bignum_num_t * num, y_bignum_num_t const * src);
//bool y_bignum_init (y_bignum_num_t * num, char const * decimal_number_str);

bool y_bignum_add (y_bignum_num_t * res, y_bignum_num_t const * a, y_bignum_num_t const * b);
bool y_bignum_sub (y_bignum_num_t * res, y_bignum_num_t const * a, y_bignum_num_t const * b);
bool y_bignum_mul (y_bignum_num_t * res, y_bignum_num_t const * a, y_bignum_num_t const * b);
bool y_bignum_div (y_bignum_num_t * res, y_bignum_num_t const * a, y_bignum_num_t const * b);
bool y_bignum_mod (y_bignum_num_t * res, y_bignum_num_t const * a, y_bignum_num_t const * b);
bool y_bignum_pow (y_bignum_num_t * res, y_bignum_num_t const * a, y_bignum_num_t const * b);   // res = a ** b (or a ^ b)

//bool y_bignum_add (y_bignum_num_t * res, y_bignum_num_t const * a, unsigned long long b);
//bool y_bignum_sub (y_bignum_num_t * res, y_bignum_num_t const * a, unsigned long long b);
//bool y_bignum_mul (y_bignum_num_t * res, y_bignum_num_t const * a, unsigned long long b);
//bool y_bignum_div (y_bignum_num_t * res, y_bignum_num_t const * a, unsigned long long b);
//bool y_bignum_mod (y_bignum_num_t * res, y_bignum_num_t const * a, unsigned long long b);
//bool y_bignum_pow (y_bignum_num_t * res, y_bignum_num_t const * a, unsigned long long b);

int y_bignum_cmp (y_bignum_num_t const * a, y_bignum_num_t const * b);  // like strcmp()
//int y_bignum_cmp (y_bignum_num_t const * a, unsigned long long b);  // like strcmp()
//int y_bignum_cmp (unsigned long long a, y_bignum_num_t const * b);  // like strcmp()

#if defined(__cplusplus)
}   // extern "C"
#endif
