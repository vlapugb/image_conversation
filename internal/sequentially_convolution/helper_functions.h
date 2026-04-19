#pragma once

#include <stddef.h>

#include <filters/filter.h>

static inline unsigned char clamp_to_u8(double value) {
  if (value < 0.0) {
    return 0U;
  }
  if (value > 255.0) {
    return 255U;
  }
  return (unsigned char)(value + 0.5);
}

static inline size_t wrap_index(long value, size_t limit) {
  long wrapped = value % (long)limit;
  if (wrapped < 0) {
    wrapped += (long)limit;
  }
  return (size_t)wrapped;
}

static inline size_t clamp_index(long value, size_t limit) {
  if (value < 0) {
    return 0U;
  }
  if ((size_t)value >= limit) {
    return limit - 1U;
  }
  return (size_t)value;
}

static inline size_t reflect_index(long value, size_t limit) {
  if (limit <= 1U) {
    return 0U;
  }

  const long period = 2L * ((long)limit - 1L);
  long reflected = value % period;

  if (reflected < 0) {
    reflected += period;
  }
  if (reflected >= (long)limit) {
    reflected = period - reflected;
  }

  return (size_t)reflected;
}

static inline size_t
resolve_index(long value, size_t limit, filter_border_mode_t border_mode) {
  switch (border_mode) {
  case FILTER_BORDER_CLAMP:
    return clamp_index(value, limit);
  case FILTER_BORDER_REFLECT:
    return reflect_index(value, limit);
  case FILTER_BORDER_WRAP:
  default:
    return wrap_index(value, limit);
  }
}
