/*
 * stm32_uidhash.c
 *
 *  Created on: Mar 15, 2024
 *      Author: https://pcbartists.com/firmware/stm32-firmware/generating-32-bit-stm32-unique-id/
 */


#include <stdio.h>
#include <string.h>

#include "stm32_uidhash.h"

// Magic numbers for 32-bit hashing.  Copied from Murmur3.
static const uint32_t c1 = 0xcc9e2d51;
static const uint32_t c2 = 0x1b873593;

static uint32_t UNALIGNED_LOAD32(const uint8_t *p) {
  uint32_t result;
  memcpy(&result, p, sizeof(result));
  return result;
}

static uint32_t Fetch32(const uint8_t *p) {
  return UNALIGNED_LOAD32(p);
}

static uint32_t Rotate32(uint32_t val, int shift) {
  // Avoid shifting by 32: doing so yields an undefined result.
  return shift == 0 ? val : ((val >> shift) | (val << (32 - shift)));
}

// A 32-bit to 32-bit integer hash copied from Murmur3.
static uint32_t fmix(uint32_t h)
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

static uint32_t Mur(uint32_t a, uint32_t h) {
  // Helper from Murmur3 for combining two 32-bit values.
  a *= c1;
  a = Rotate32(a, 17);
  a *= c2;
  h ^= a;
  h = Rotate32(h, 19);
  return h * 5 + 0xe6546b64;
}

uint32_t Hash32Len5to12(const uint8_t *s, size_t len) {
  uint32_t a = (uint32_t)len, b = a * 5, c = 9, d = b;
  a += Fetch32(s);
  b += Fetch32(s + len - 4);
  c += Fetch32(s + ((len >> 1) & 4));
  return fmix(Mur(c, Mur(b, Mur(a, d))));
}
