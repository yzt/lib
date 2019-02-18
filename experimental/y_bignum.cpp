
#include "../experimental/y_bignum.hpp"
#include <assert.h>
#include <stdlib.h> // for malloc(), et al
#include <string.h> // for memset()

static const y_bignum_dig_t WordMax = ~(y_bignum_dig_t)0;

static inline int
min (int a, int b) {
    return b < a ? b : a;
}

static inline int
max (int a, int b) {
    return b < a ? a : b;
}

static inline y_bignum_dig_t
dig (y_bignum_num_t const * num, int idx) {
    assert(num && num->digs);
    assert(idx >= 0 && idx < num->size);
    return num->digs[idx];
}

static inline y_bignum_dig_t
dig_safe (y_bignum_num_t const * num, int idx) {
    if (num && num->digs && idx >= 0 && idx < num->size)
        return num->digs[idx];
    else
        return 0;
}

static inline void
set_dig (y_bignum_num_t * num, int idx, y_bignum_dig_t v) {
    assert(num && num->digs);
    assert(idx >= 0 && idx < num->size);
    num->digs[idx] = v;
}

static inline int
dig_count (y_bignum_num_t const * num) {
    int ret = 0;
    if (num && num->digs)
        ret = num->size;
    return ret;
}

static inline bool
is_negative (y_bignum_num_t const * num) {
    bool ret = false;
    if (num)
        ret = num->negative;
    return ret;
}

static inline void
set_sign (y_bignum_num_t * num, bool negative) {
    if (num)
        num->negative = negative;
}

struct ResultAndCarry {
    y_bignum_dig_t result, carry;
};

static inline ResultAndCarry
add_with_carry (y_bignum_dig_t a, y_bignum_dig_t b, y_bignum_dig_t c) {
    assert(c < WordMax);    // input carry is small!
    ResultAndCarry ret;
    ret.result = a + b + c;
    ret.carry = ((ret.result < a || ret.result < b /*|| ret.result < c*/) ? 1 : 0);
    return ret;
}

bool y_bignum_alloc (y_bignum_num_t * num, int capacity_in_words, bool clear_to_zero) {
    bool ret = false;
    return ret;
}

bool y_bignum_realloc (y_bignum_num_t * num, int new_capacity_in_words, bool clear_to_zero) {
    bool ret = false;
    if (num && new_capacity_in_words > 0) {
        if (num->digs && num->size != new_capacity_in_words) {
            ::free(num->digs);
            num->digs = nullptr;
            num->size = 0;
        }
        if (!num->digs) {
            num->digs = (y_bignum_dig_t *)::malloc(new_capacity_in_words * sizeof(y_bignum_dig_t));
            num->size = new_capacity_in_words;
        }
        if (clear_to_zero)
            ::memset(num->digs, 0, new_capacity_in_words * sizeof(y_bignum_dig_t));
        ret = true;
    }
    return ret;
}

void y_bignum_free (y_bignum_num_t * num) {
    if (num) {
        if (num->digs) {
            ::free(num->digs);
            num->digs = nullptr;
        }
        num->size = 0;
    }
}

bool y_bignum_copy (y_bignum_num_t * dst, y_bignum_num_t const * src);
void y_bignum_trim (y_bignum_num_t * num, bool free_excess_memory);

bool y_bignum_init (y_bignum_num_t * num, y_bignum_dig_t v) {
    bool ret = false;
    if (num) {
        y_bignum_realloc(num, 1, false);
        num->negative = false;
        set_dig(num, 0, v);
        ret = true;
    }
    return ret;
}

//bool y_bignum_init (y_bignum_num_t * num, y_bignum_num_t const * src);
//bool y_bignum_init (y_bignum_num_t * num, char const * decimal_number_str);

bool y_bignum_negate (y_bignum_num_t * num) {
    bool ret = false;
    if (num) {
        num->negative = !num->negative;
        ret = true;
    }
    return ret;
}

bool y_bignum_set_sign (y_bignum_num_t * num, bool negative) {
    bool ret = false;
    if (num) {
        num->negative = negative;
        ret = true;
    }
    return ret;
}

bool y_bignum_add_unsigned (y_bignum_num_t * res, y_bignum_num_t const * a, y_bignum_num_t const * b) {
    bool ret = false;
    int acnt = dig_count(a);
    int bcnt = dig_count(b);
    int rcnt = dig_count(res);
    int xcnt = max(acnt, bcnt);
    if (rcnt >= xcnt) {
        int ncnt = min(acnt, bcnt);
        y_bignum_dig_t carry = 0;
        int i = 0;
        for (; i < ncnt; ++i) {
            ResultAndCarry v = add_with_carry(dig(a, i), dig(b, i), carry);
            carry = v.carry;
            set_dig(res, i, v.result);
        }
        for (; i < acnt; ++i) {
            ResultAndCarry v = add_with_carry(dig(a, i), 0, carry);
            carry = v.carry;
            set_dig(res, i, v.result);
        }
        for (; i < bcnt; ++i) {
            ResultAndCarry v = add_with_carry(0, dig(b, i), carry);
            carry = v.carry;
            set_dig(res, i, v.result);
        }
        if (carry <= 0 || (carry > 0 && rcnt >= xcnt + 1)) {
            if (carry > 0) {
                set_dig(res, i, carry);
                i += 1;
            }
            for (; i < rcnt; ++i)
                set_dig(res, i, 0);
            y_bignum_set_sign(res, false);
            ret = true;
        }
    }
    return ret;
}

bool y_bignum_sub_unsigned (y_bignum_num_t * res, y_bignum_num_t const * a, y_bignum_num_t const * b) {
    bool ret = false;
    
    return ret;
}

bool y_bignum_add (y_bignum_num_t * res, y_bignum_num_t const * a, y_bignum_num_t const * b) {
    if (res && a && b) {
        bool aneg = is_negative(a);
        bool bneg = is_negative(b);
        if (!aneg && !bneg)
            return y_bignum_add_unsigned(res, a, b);
        else if (!aneg && bneg)
            return y_bignum_sub_unsigned(res, a, b);
        else if (aneg && !bneg)
            return y_bignum_sub_unsigned(res, b, a);
        else //if (aneg && bneg)
            if (y_bignum_add_unsigned(res, a, b)) {
                y_bignum_negate(res);
                return true;
            } else
                return false;
    } else {
        return false;
    }
}

bool y_bignum_sub (y_bignum_num_t * res, y_bignum_num_t const * a, y_bignum_num_t const * b) {
    if (res && a && b) {
        bool aneg = is_negative(a);
        bool bneg = is_negative(b);
        if (!aneg && !bneg)
            return y_bignum_sub_unsigned(res, a, b);
        else if (!aneg && bneg)
            return y_bignum_add_unsigned(res, a, b);
        else if (aneg && !bneg)
            if (y_bignum_add_unsigned(res, a, b)) {
                y_bignum_negate(res);
                return true;
            } else
                return false;
        else //if (aneg && bneg)
            return y_bignum_sub_unsigned(res, b, a);
    } else {
        return false;
    }
}

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
