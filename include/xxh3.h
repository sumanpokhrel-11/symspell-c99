/*
xxh3.h - Dr James Freeman

This code is a C port of the C++ code at https://github.com/chys87/constexpr-xxh3

This file uses code from Yann Collet's xxHash implementation.

Original xxHash copyright notice:

xxHash - Extremely Fast Hash algorithm
Header File
Copyright (C) 2012-2020 Yann Collet
*/

#ifndef XXH3_H
#define XXH3_H

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#define XXH3_STRIPE_LEN           64
#define XXH3_SECRET_CONSUME_RATE  8
#define XXH3_ACC_NB               8
#define XXH3_SECRET_DEFAULT_SIZE  192
#define XXH3_SECRET_SIZE_MIN      136
#define xxh3(x, len) xxh3_XXH3_64bits_const(x, len)
#define XXH3_64bits(x, len) xxh3_XXH3_64bits_const(x, len)

/* Structure to replace std::pair<uint64_t, uint64_t> */
typedef struct {
    uint64_t first;
    uint64_t second;
} uint64_pair_t;

/* Utility function to simulate std::data() for arrays */
static inline const void* xxh3_data(const void* ptr) {
    return ptr;
}

/* Utility function to handle byte size calculation */
static inline size_t xxh3_bytes_size_array(size_t array_size) {
    return (array_size ? array_size - 1 : 0);
}

static inline uint32_t xxh3_swap32(uint32_t x) {
  return ((x << 24) & 0xff000000) | ((x << 8) & 0x00ff0000) |
         ((x >> 8) & 0x0000ff00) | ((x >> 24) & 0x000000ff);
}

static inline uint32_t xxh3_readLE32(const void* ptr) {
  const uint8_t* p = (const uint8_t*)ptr;
  return (uint32_t)p[0] | ((uint32_t)p[1]) << 8 |
         ((uint32_t)p[2]) << 16 | ((uint32_t)p[3]) << 24;
}

static inline uint64_t xxh3_swap64(uint64_t x) {
  return ((x << 56) & 0xff00000000000000ULL) |
         ((x << 40) & 0x00ff000000000000ULL) |
         ((x << 24) & 0x0000ff0000000000ULL) |
         ((x << 8) & 0x000000ff00000000ULL) |
         ((x >> 8) & 0x00000000ff000000ULL) |
         ((x >> 24) & 0x0000000000ff0000ULL) |
         ((x >> 40) & 0x000000000000ff00ULL) |
         ((x >> 56) & 0x00000000000000ffULL);
}

static inline uint64_t xxh3_readLE64(const void* ptr) {
  const uint8_t* p = (const uint8_t*)ptr;
  return xxh3_readLE32(p) | ((uint64_t)xxh3_readLE32(p + 4)) << 32;
}

static inline void xxh3_writeLE64(uint8_t* dst, uint64_t v) {
  for (int i = 0; i < 8; ++i) dst[i] = (uint8_t)(v >> (i * 8));
}

static const uint32_t XXH3_PRIME32_1 = 0x9E3779B1U;
static const uint32_t XXH3_PRIME32_2 = 0x85EBCA77U;
static const uint32_t XXH3_PRIME32_3 = 0xC2B2AE3DU;

static const uint64_t XXH3_PRIME64_1 = 0x9E3779B185EBCA87ULL;
static const uint64_t XXH3_PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
static const uint64_t XXH3_PRIME64_3 = 0x165667B19E3779F9ULL;
static const uint64_t XXH3_PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
static const uint64_t XXH3_PRIME64_5 = 0x27D4EB2F165667C5ULL;

static const uint8_t xxh3_kSecret[XXH3_SECRET_DEFAULT_SIZE] = {
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c,
    0xf7, 0x21, 0xad, 0x1c, 0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb,
    0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f, 0xcb, 0x79, 0xe6, 0x4e,
    0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6,
    0x81, 0x3a, 0x26, 0x4c, 0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb,
    0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3, 0x71, 0x64, 0x48, 0x97,
    0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7,
    0xc7, 0x0b, 0x4f, 0x1d, 0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31,
    0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64, 0xea, 0xc5, 0xac, 0x83,
    0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26,
    0x29, 0xd4, 0x68, 0x9e, 0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc,
    0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce, 0x45, 0xcb, 0x3a, 0x8f,
    0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
};

static inline uint64_pair_t xxh3_mult64to128(uint64_t lhs, uint64_t rhs) {
  uint64_t lo_lo = (uint64_t)((uint32_t)lhs) * (uint32_t)rhs;
  uint64_t hi_lo = (lhs >> 32) * (uint32_t)rhs;
  uint64_t lo_hi = (uint32_t)lhs * (rhs >> 32);
  uint64_t hi_hi = (lhs >> 32) * (rhs >> 32);
  uint64_t cross = (lo_lo >> 32) + (uint32_t)hi_lo + lo_hi;
  uint64_t upper = (hi_lo >> 32) + (cross >> 32) + hi_hi;
  uint64_t lower = (cross << 32) | (uint32_t)lo_lo;
  uint64_pair_t result = {lower, upper};
  return result;
}

static inline uint64_t xxh3_mul128_fold64(uint64_t lhs, uint64_t rhs) {
#if defined __GNUC__ && __WORDSIZE >= 64
  /* It appears both GCC and Clang support evaluating __int128 as constexpr */
  unsigned __int128 product = (unsigned __int128)lhs * rhs;
  return (uint64_t)(product >> 64) ^ (uint64_t)product;
#else
  uint64_pair_t product = xxh3_mult64to128(lhs, rhs);
  return product.first ^ product.second;
#endif
}

static inline uint64_t xxh3_XXH64_avalanche(uint64_t h) {
  h = (h ^ (h >> 33)) * XXH3_PRIME64_2;
  h = (h ^ (h >> 29)) * XXH3_PRIME64_3;
  return h ^ (h >> 32);
}

static inline uint64_t xxh3_XXH3_avalanche(uint64_t h) {
  h = (h ^ (h >> 37)) * 0x165667919E3779F9ULL;
  return h ^ (h >> 32);
}

static inline uint64_t xxh3_rrmxmx(uint64_t h, uint64_t len) {
  h ^= ((h << 49) | (h >> 15)) ^ ((h << 24) | (h >> 40));
  h *= 0x9FB21C651E98DF25ULL;
  h ^= (h >> 35) + len;
  h *= 0x9FB21C651E98DF25ULL;
  return h ^ (h >> 28);
}

static inline uint64_t xxh3_mix16B(const void* input, const void* secret, uint64_t seed) {
  const uint8_t* inp = (const uint8_t*)input;
  const uint8_t* sec = (const uint8_t*)secret;
  return xxh3_mul128_fold64(xxh3_readLE64(inp) ^ (xxh3_readLE64(sec) + seed),
                       xxh3_readLE64(inp + 8) ^ (xxh3_readLE64(sec + 8) - seed));
}

static inline void xxh3_accumulate_512(uint64_t* acc, const void* input, const void* secret) {
  const uint8_t* inp = (const uint8_t*)input;
  const uint8_t* sec = (const uint8_t*)secret;
  for (size_t i = 0; i < XXH3_ACC_NB; i++) {
    uint64_t data_val = xxh3_readLE64(inp + 8 * i);
    uint64_t data_key = data_val ^ xxh3_readLE64(sec + i * 8);
    acc[i ^ 1] += data_val;
    acc[i] += (uint32_t)data_key * (data_key >> 32);
  }
}

static inline uint64_t xxh3_hashLong_64b_internal(const void* input, size_t len,
                                         const void* secret,
                                         size_t secretSize) {
  const uint8_t* inp = (const uint8_t*)input;
  const uint8_t* sec = (const uint8_t*)secret;
  uint64_t acc[XXH3_ACC_NB] = {XXH3_PRIME32_3, XXH3_PRIME64_1, XXH3_PRIME64_2, XXH3_PRIME64_3,
                       XXH3_PRIME64_4, XXH3_PRIME32_2, XXH3_PRIME64_5, XXH3_PRIME32_1};
  size_t nbStripesPerBlock = (secretSize - XXH3_STRIPE_LEN) / XXH3_SECRET_CONSUME_RATE;
  size_t block_len = XXH3_STRIPE_LEN * nbStripesPerBlock;
  size_t nb_blocks = (len - 1) / block_len;

  for (size_t n = 0; n < nb_blocks; n++) {
    for (size_t i = 0; i < nbStripesPerBlock; i++)
      xxh3_accumulate_512(acc, inp + n * block_len + i * XXH3_STRIPE_LEN,
                     sec + i * XXH3_SECRET_CONSUME_RATE);
    for (size_t i = 0; i < XXH3_ACC_NB; i++)
      acc[i] = (acc[i] ^ (acc[i] >> 47) ^
                xxh3_readLE64(sec + secretSize - XXH3_STRIPE_LEN + 8 * i)) *
               XXH3_PRIME32_1;
  }

  size_t nbStripes = ((len - 1) - (block_len * nb_blocks)) / XXH3_STRIPE_LEN;
  for (size_t i = 0; i < nbStripes; i++)
    xxh3_accumulate_512(acc, inp + nb_blocks * block_len + i * XXH3_STRIPE_LEN,
                   sec + i * XXH3_SECRET_CONSUME_RATE);
  xxh3_accumulate_512(acc, inp + len - XXH3_STRIPE_LEN,
                 sec + secretSize - XXH3_STRIPE_LEN - 7);
  uint64_t result = len * XXH3_PRIME64_1;
  for (size_t i = 0; i < 4; i++)
    result +=
        xxh3_mul128_fold64(acc[2 * i] ^ xxh3_readLE64(sec + 11 + 16 * i),
                      acc[2 * i + 1] ^ xxh3_readLE64(sec + 11 + 16 * i + 8));
  return xxh3_XXH3_avalanche(result);
}

/* Function pointer type for hash long function */
typedef uint64_t (*xxh3_hash_long_func_t)(const void* input, size_t len, uint64_t seed, const void* secret, size_t secretLen);

static inline uint64_t xxh3_XXH3_64bits_internal(const void* input, size_t len,
                                        uint64_t seed, const void* secret,
                                        size_t secretLen,
                                        xxh3_hash_long_func_t f_hashLong) {
  const uint8_t* inp = (const uint8_t*)input;
  const uint8_t* sec = (const uint8_t*)secret;
  if (len == 0) {
    return xxh3_XXH64_avalanche(seed ^
                           (xxh3_readLE64(sec + 56) ^ xxh3_readLE64(sec + 64)));
  } else if (len < 4) {
    uint64_t keyed = (((uint32_t)inp[0] << 16) |
                      ((uint32_t)inp[len >> 1] << 24) |
                      inp[len - 1] | ((uint32_t)len << 8)) ^
                     ((xxh3_readLE32(sec) ^ xxh3_readLE32(sec + 4)) + seed);
    return xxh3_XXH64_avalanche(keyed);
  } else if (len <= 8) {
    uint64_t keyed =
        (xxh3_readLE32(inp + len - 4) + ((uint64_t)xxh3_readLE32(inp) << 32)) ^
        ((xxh3_readLE64(sec + 8) ^ xxh3_readLE64(sec + 16)) -
         (seed ^ ((uint64_t)xxh3_swap32((uint32_t)seed) << 32)));
    return xxh3_rrmxmx(keyed, len);
  } else if (len <= 16) {
    uint64_t input_lo =
        xxh3_readLE64(inp) ^
        ((xxh3_readLE64(sec + 24) ^ xxh3_readLE64(sec + 32)) + seed);
    uint64_t input_hi =
        xxh3_readLE64(inp + len - 8) ^
        ((xxh3_readLE64(sec + 40) ^ xxh3_readLE64(sec + 48)) - seed);
    uint64_t acc =
        len + xxh3_swap64(input_lo) + input_hi + xxh3_mul128_fold64(input_lo, input_hi);
    return xxh3_XXH3_avalanche(acc);
  } else if (len <= 128) {
    uint64_t acc = len * XXH3_PRIME64_1;
    size_t secret_off = 0;
    for (size_t i = 0, j = len; j > i; i += 16, j -= 16) {
      acc += xxh3_mix16B(inp + i, sec + secret_off, seed);
      acc += xxh3_mix16B(inp + j - 16, sec + secret_off + 16, seed);
      secret_off += 32;
    }
    return xxh3_XXH3_avalanche(acc);
  } else if (len <= 240) {
    uint64_t acc = len * XXH3_PRIME64_1;
    for (size_t i = 0; i < 128; i += 16)
      acc += xxh3_mix16B(inp + i, sec + i, seed);
    acc = xxh3_XXH3_avalanche(acc);
    for (size_t i = 128; i < len / 16 * 16; i += 16)
      acc += xxh3_mix16B(inp + i, sec + (i - 128) + 3, seed);
    acc += xxh3_mix16B(inp + len - 16, sec + XXH3_SECRET_SIZE_MIN - 17, seed);
    return xxh3_XXH3_avalanche(acc);
  } else {
    return f_hashLong(input, len, seed, secret, secretLen);
  }
}

/* Helper function for default secret hash long */
static uint64_t xxh3_hashLong_default_secret(const void* input, size_t len, uint64_t seed, const void* secret, size_t secretLen) {
  (void)seed;  /* unused for default secret */
  (void)secret;  /* unused, we use kSecret */
  (void)secretLen;  /* unused, we use sizeof(kSecret) */
  return xxh3_hashLong_64b_internal(input, len, xxh3_kSecret, sizeof(xxh3_kSecret));
}

/* Helper function for custom secret hash long */
static uint64_t xxh3_hashLong_custom_secret(const void* input, size_t len, uint64_t seed, const void* secret, size_t secretLen) {
  (void)seed;  /* unused for custom secret */
  return xxh3_hashLong_64b_internal(input, len, secret, secretLen);
}

/* Helper function for seeded hash long */
static uint64_t xxh3_hashLong_with_seed(const void* input, size_t len, uint64_t seed, const void* secret, size_t secretLen) {
  (void)secret;  /* unused, we generate our own */
  (void)secretLen;  /* unused */
  uint8_t secret_local[XXH3_SECRET_DEFAULT_SIZE];
  for (size_t i = 0; i < XXH3_SECRET_DEFAULT_SIZE; i += 16) {
    xxh3_writeLE64(secret_local + i, xxh3_readLE64(xxh3_kSecret + i) + seed);
    xxh3_writeLE64(secret_local + i + 8, xxh3_readLE64(xxh3_kSecret + i + 8) - seed);
  }
  return xxh3_hashLong_64b_internal(input, len, secret_local, sizeof(secret_local));
}

/* Basic interfaces */

static inline uint64_t xxh3_XXH3_64bits_const(const void* input, size_t len) {
  return xxh3_XXH3_64bits_internal(
      input, len, 0, xxh3_kSecret, sizeof(xxh3_kSecret),
      xxh3_hashLong_default_secret);
}

static inline uint64_t xxh3_XXH3_64bits_withSecret_const(const void* input, size_t len,
                                                const void* secret,
                                                size_t secretSize) {
  return xxh3_XXH3_64bits_internal(
      input, len, 0, secret, secretSize,
      xxh3_hashLong_custom_secret);
}

static inline uint64_t xxh3_XXH3_64bits_withSeed_const(const void* input, size_t len,
                                              uint64_t seed) {
  if (seed == 0) return xxh3_XXH3_64bits_const(input, len);
  return xxh3_XXH3_64bits_internal(
      input, len, seed, xxh3_kSecret, sizeof(xxh3_kSecret),
      xxh3_hashLong_with_seed);
}

/* Convenient interfaces for arrays */

static inline uint64_t xxh3_XXH3_64bits_const_array(const void* input, size_t array_size) {
  return xxh3_XXH3_64bits_const(input, xxh3_bytes_size_array(array_size));
}

static inline uint64_t xxh3_XXH3_64bits_withSecret_const_array(const void* input, size_t input_array_size,
                                                const void* secret, size_t secret_array_size) {
  return xxh3_XXH3_64bits_withSecret_const(input, xxh3_bytes_size_array(input_array_size),
                                      secret, xxh3_bytes_size_array(secret_array_size));
}

static inline uint64_t xxh3_XXH3_64bits_withSeed_const_array(const void* input, size_t array_size,
                                              uint64_t seed) {
  return xxh3_XXH3_64bits_withSeed_const(input, xxh3_bytes_size_array(array_size), seed);
}

#endif /* XXH3_H */


