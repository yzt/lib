#pragma once

typedef unsigned int y_bignum_dig_t;

typedef struct {
    y_bignum_dig_t * digs;
    unsigned len;
    unsigned cap;
} y_bignum_raw_t;

typedef struct {
    int shift;
    int sign;
} y_bignum_meta_t;


y_bignum_dig_t y_bignum_alloc (unsigned capacity_in_words);
y_bignum_dig_t y_bignum_realloc (y_bignum_dig_t digs, unsigned new_capacity_in_words);
void y_bignum_free (y_bignum_dig_t digs);
