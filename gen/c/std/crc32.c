#ifndef WUFFS_CRC32_H
#define WUFFS_CRC32_H

// Code generated by wuffs-c. DO NOT EDIT.

#ifndef WUFFS_BASE_HEADER_H
#define WUFFS_BASE_HEADER_H

// Copyright 2017 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Wuffs requires a word size of at least 32 bits because it assumes that
// converting a u32 to usize will never overflow. For example, the size of a
// decoded image is often represented, explicitly or implicitly in an image
// file, as a u32, and it is convenient to compare that to a buffer size.
//
// Similarly, the word size is at most 64 bits because it assumes that
// converting a usize to u64 will never overflow.
#if __WORDSIZE < 32
#error "Wuffs requires a word size of at least 32 bits"
#elif __WORDSIZE > 64
#error "Wuffs requires a word size of at most 64 bits"
#endif

// WUFFS_VERSION is the major.minor version number as a uint32. The major
// number is the high 16 bits. The minor number is the low 16 bits.
//
// The intention is to bump the version number at least on every API / ABI
// backwards incompatible change.
//
// For now, the API and ABI are simply unstable and can change at any time.
//
// TODO: don't hard code this in base-header.h.
#define WUFFS_VERSION (0x00001)

// ---------------- I/O

// wuffs_base__slice_u8 is a 1-dimensional buffer (a pointer and length).
//
// A value with all fields NULL or zero is a valid, empty slice.
typedef struct {
  uint8_t* ptr;
  size_t len;
} wuffs_base__slice_u8;

// wuffs_base__buf1 is a 1-dimensional buffer (a pointer and length), plus
// additional indexes into that buffer, plus an opened / closed flag.
//
// A value with all fields NULL or zero is a valid, empty buffer.
typedef struct {
  uint8_t* ptr;  // Pointer.
  size_t len;    // Length.
  size_t wi;     // Write index. Invariant: wi <= len.
  size_t ri;     // Read  index. Invariant: ri <= wi.
  bool closed;   // No further writes are expected.
} wuffs_base__buf1;

// wuffs_base__limit1 provides a limited view of a 1-dimensional byte stream:
// its first N bytes. That N can be greater than a buffer's current read or
// write capacity. N decreases naturally over time as bytes are read from or
// written to the stream.
//
// A value with all fields NULL or zero is a valid, unlimited view.
typedef struct wuffs_base__limit1 {
  uint64_t* ptr_to_len;             // Pointer to N.
  struct wuffs_base__limit1* next;  // Linked list of limits.
} wuffs_base__limit1;

typedef struct {
  // TODO: move buf into private_impl? As it is, it looks like users can modify
  // the buf field to point to a different buffer, which can turn the limit and
  // mark fields into dangling pointers.
  wuffs_base__buf1* buf;
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__limit1 limit;
    uint8_t* mark;
  } private_impl;
} wuffs_base__reader1;

typedef struct {
  // TODO: move buf into private_impl? As it is, it looks like users can modify
  // the buf field to point to a different buffer, which can turn the limit and
  // mark fields into dangling pointers.
  wuffs_base__buf1* buf;
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__limit1 limit;
    uint8_t* mark;
  } private_impl;
} wuffs_base__writer1;

// ---------------- Images

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    uint32_t flags;
    uint32_t w;
    uint32_t h;
    // TODO: color model, including both packed RGBA and planar,
    // chroma-subsampled YCbCr.
  } private_impl;
} wuffs_base__image_config;

static inline void wuffs_base__image_config__invalidate(
    wuffs_base__image_config* c) {
  if (c) {
    *c = ((wuffs_base__image_config){});
  }
}

static inline bool wuffs_base__image_config__valid(
    wuffs_base__image_config* c) {
  if (!c || !(c->private_impl.flags & 1)) {
    return false;
  }
  uint64_t wh = ((uint64_t)c->private_impl.w) * ((uint64_t)c->private_impl.h);
  // TODO: handle things other than 1 byte per pixel.
  return wh <= ((uint64_t)SIZE_MAX);
}

static inline uint32_t wuffs_base__image_config__width(
    wuffs_base__image_config* c) {
  return wuffs_base__image_config__valid(c) ? c->private_impl.w : 0;
}

static inline uint32_t wuffs_base__image_config__height(
    wuffs_base__image_config* c) {
  return wuffs_base__image_config__valid(c) ? c->private_impl.h : 0;
}

// TODO: this is the right API for planar (not packed) pixbufs? Should it allow
// decoding into a color model different from the format's intrinsic one? For
// example, decoding a JPEG image straight to RGBA instead of to YCbCr?
static inline size_t wuffs_base__image_config__pixbuf_size(
    wuffs_base__image_config* c) {
  if (wuffs_base__image_config__valid(c)) {
    uint64_t wh = ((uint64_t)c->private_impl.w) * ((uint64_t)c->private_impl.h);
    // TODO: handle things other than 1 byte per pixel.
    return (size_t)wh;
  }
  return 0;
}

static inline void wuffs_base__image_config__initialize(
    wuffs_base__image_config* c,
    uint32_t width,
    uint32_t height,
    uint32_t TODO_color_model) {
  if (!c) {
    return;
  }
  c->private_impl.flags = 1;
  c->private_impl.w = width;
  c->private_impl.h = height;
  // TODO: color model.
}

#endif  // WUFFS_BASE_HEADER_H

  // ---------------- Use Declarations

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

// Status codes are int32_t values:
//  - the sign bit indicates a non-recoverable status code: an error
//  - bits 10-30 hold the packageid: a namespace
//  - bits 8-9 are reserved
//  - bits 0-7 are a package-namespaced numeric code
//
// Do not manipulate these bits directly. Use the API functions such as
// wuffs_crc32__status__is_error instead.
typedef int32_t wuffs_crc32__status;

#define wuffs_crc32__packageid 810620  // 0x000C5E7C

#define WUFFS_CRC32__STATUS_OK 0                                   // 0x00000000
#define WUFFS_CRC32__ERROR_BAD_WUFFS_VERSION -2147483647           // 0x80000001
#define WUFFS_CRC32__ERROR_BAD_RECEIVER -2147483646                // 0x80000002
#define WUFFS_CRC32__ERROR_BAD_ARGUMENT -2147483645                // 0x80000003
#define WUFFS_CRC32__ERROR_INITIALIZER_NOT_CALLED -2147483644      // 0x80000004
#define WUFFS_CRC32__ERROR_INVALID_I_O_OPERATION -2147483643       // 0x80000005
#define WUFFS_CRC32__ERROR_CLOSED_FOR_WRITES -2147483642           // 0x80000006
#define WUFFS_CRC32__ERROR_UNEXPECTED_EOF -2147483641              // 0x80000007
#define WUFFS_CRC32__SUSPENSION_SHORT_READ 8                       // 0x00000008
#define WUFFS_CRC32__SUSPENSION_SHORT_WRITE 9                      // 0x00000009
#define WUFFS_CRC32__ERROR_CANNOT_RETURN_A_SUSPENSION -2147483638  // 0x8000000A
#define WUFFS_CRC32__ERROR_INVALID_CALL_SEQUENCE -2147483637       // 0x8000000B
#define WUFFS_CRC32__SUSPENSION_END_OF_DATA 12                     // 0x0000000C

bool wuffs_crc32__status__is_error(wuffs_crc32__status s);

const char* wuffs_crc32__status__string(wuffs_crc32__status s);

// ---------------- Public Consts

// ---------------- Structs

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so. Instead, use the
  // wuffs_crc32__ieee__etc functions.
  //
  // In C++, these fields would be "private", but C does not support that.
  //
  // It is a struct, not a struct*, so that it can be stack allocated.
  struct {
    wuffs_crc32__status status;
    uint32_t magic;

    uint32_t f_state;

  } private_impl;
} wuffs_crc32__ieee;

// ---------------- Public Initializer Prototypes

// wuffs_crc32__ieee__initialize is an initializer function.
//
// It should be called before any other wuffs_crc32__ieee__* function.
//
// Pass WUFFS_VERSION and 0 for wuffs_version and for_internal_use_only.
void wuffs_crc32__ieee__initialize(wuffs_crc32__ieee* self,
                                   uint32_t wuffs_version,
                                   uint32_t for_internal_use_only);

// ---------------- Public Function Prototypes

uint32_t wuffs_crc32__ieee__update(wuffs_crc32__ieee* self,
                                   wuffs_base__slice_u8 a_x);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WUFFS_CRC32_H

// C HEADER ENDS HERE.

#ifndef WUFFS_BASE_IMPL_H
#define WUFFS_BASE_IMPL_H

// Copyright 2017 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// wuffs_base__empty_struct is used when a Wuffs function returns an empty
// struct. In C, if a function f returns void, you can't say "x = f()", but in
// Wuffs, if a function g returns empty, you can say "y = g()".
typedef struct {
} wuffs_base__empty_struct;

#define WUFFS_BASE__IGNORE_POTENTIALLY_UNUSED_VARIABLE(x) (void)(x)

// WUFFS_BASE__MAGIC is a magic number to check that initializers are called.
// It's not foolproof, given C doesn't automatically zero memory before use,
// but it should catch 99.99% of cases.
//
// Its (non-zero) value is arbitrary, based on md5sum("wuffs").
#define WUFFS_BASE__MAGIC (0x3CCB6C71U)

// WUFFS_BASE__ALREADY_ZEROED is passed from a container struct's initializer
// to a containee struct's initializer when the container has already zeroed
// the containee's memory.
//
// Its (non-zero) value is arbitrary, based on md5sum("zeroed").
#define WUFFS_BASE__ALREADY_ZEROED (0x68602EF1U)

// Denote intentional fallthroughs for -Wimplicit-fallthrough.
//
// The order matters here. Clang also defines "__GNUC__".
#if defined(__clang__) && __cplusplus >= 201103L
#define WUFFS_BASE__FALLTHROUGH [[clang::fallthrough]]
#elif !defined(__clang__) && defined(__GNUC__) && (__GNUC__ >= 7)
#define WUFFS_BASE__FALLTHROUGH __attribute__((fallthrough))
#else
#define WUFFS_BASE__FALLTHROUGH
#endif

// Use switch cases for coroutine suspension points, similar to the technique
// in https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
//
// We use trivial macros instead of an explicit assignment and case statement
// so that clang-format doesn't get confused by the unusual "case"s.
#define WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0 case 0:;
#define WUFFS_BASE__COROUTINE_SUSPENSION_POINT(n) \
  coro_susp_point = n;                            \
  WUFFS_BASE__FALLTHROUGH;                        \
  case n:;

#define WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(n) \
  if (status < 0) {                                             \
    goto exit;                                                  \
  } else if (status == 0) {                                     \
    goto ok;                                                    \
  }                                                             \
  coro_susp_point = n;                                          \
  goto suspend;                                                 \
  case n:;

// Clang also defines "__GNUC__".
#if defined(__GNUC__)
#define WUFFS_BASE__LIKELY(expr) (__builtin_expect(!!(expr), 1))
#define WUFFS_BASE__UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#else
#define WUFFS_BASE__LIKELY(expr) (expr)
#define WUFFS_BASE__UNLIKELY(expr) (expr)
#endif

// Uncomment this #include for printf-debugging.
// #include <stdio.h>

// ---------------- Static Inline Functions
//
// The helpers below are functions, instead of macros, because their arguments
// can be an expression that we shouldn't evaluate more than once.
//
// They are in base-impl.h and hence copy/pasted into every generated C file,
// instead of being in some "base.c" file, since a design goal is that users of
// the generated C code can often just #include a single .c file, such as
// "gif.c", without having to additionally include or otherwise build and link
// a "base.c" file.
//
// They are static, so that linking multiple wuffs .o files won't complain about
// duplicate function definitions.
//
// They are explicitly marked inline, even if modern compilers don't use the
// inline attribute to guide optimizations such as inlining, to avoid the
// -Wunused-function warning, and we like to compile with -Wall -Werror.

static inline uint16_t wuffs_base__load_u16be(uint8_t* p) {
  return ((uint16_t)(p[0]) << 8) | ((uint16_t)(p[1]) << 0);
}

static inline uint16_t wuffs_base__load_u16le(uint8_t* p) {
  return ((uint16_t)(p[0]) << 0) | ((uint16_t)(p[1]) << 8);
}

static inline uint32_t wuffs_base__load_u32be(uint8_t* p) {
  return ((uint32_t)(p[0]) << 24) | ((uint32_t)(p[1]) << 16) |
         ((uint32_t)(p[2]) << 8) | ((uint32_t)(p[3]) << 0);
}

static inline uint32_t wuffs_base__load_u32le(uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24);
}

static inline wuffs_base__slice_u8 wuffs_base__slice_u8__subslice_i(
    wuffs_base__slice_u8 s,
    uint64_t i) {
  if ((i <= SIZE_MAX) && (i <= s.len)) {
    return ((wuffs_base__slice_u8){
        .ptr = s.ptr + i,
        .len = s.len - i,
    });
  }
  return ((wuffs_base__slice_u8){});
}

static inline wuffs_base__slice_u8 wuffs_base__slice_u8__subslice_j(
    wuffs_base__slice_u8 s,
    uint64_t j) {
  if ((j <= SIZE_MAX) && (j <= s.len)) {
    return ((wuffs_base__slice_u8){.ptr = s.ptr, .len = j});
  }
  return ((wuffs_base__slice_u8){});
}

static inline wuffs_base__slice_u8 wuffs_base__slice_u8__subslice_ij(
    wuffs_base__slice_u8 s,
    uint64_t i,
    uint64_t j) {
  if ((i <= j) && (j <= SIZE_MAX) && (j <= s.len)) {
    return ((wuffs_base__slice_u8){
        .ptr = s.ptr + i,
        .len = j - i,
    });
  }
  return ((wuffs_base__slice_u8){});
}

// wuffs_base__slice_u8__prefix returns up to the first up_to bytes of s.
static inline wuffs_base__slice_u8 wuffs_base__slice_u8__prefix(
    wuffs_base__slice_u8 s,
    uint64_t up_to) {
  if ((uint64_t)(s.len) > up_to) {
    s.len = up_to;
  }
  return s;
}

// wuffs_base__slice_u8__suffix returns up to the last up_to bytes of s.
static inline wuffs_base__slice_u8 wuffs_base__slice_u8_suffix(
    wuffs_base__slice_u8 s,
    uint64_t up_to) {
  if ((uint64_t)(s.len) > up_to) {
    s.ptr += (uint64_t)(s.len) - up_to;
    s.len = up_to;
  }
  return s;
}

// wuffs_base__slice_u8__copy_from_slice calls memmove(dst.ptr, src.ptr,
// length) where length is the minimum of dst.len and src.len.
//
// Passing a wuffs_base__slice_u8 with all fields NULL or zero (a valid, empty
// slice) is valid and results in a no-op.
static inline uint64_t wuffs_base__slice_u8__copy_from_slice(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 src) {
  size_t length = dst.len < src.len ? dst.len : src.len;
  if (length > 0) {
    memmove(dst.ptr, src.ptr, length);
  }
  return length;
}

static inline uint32_t wuffs_base__writer1__copy_from_history32(
    uint8_t** ptr_ptr,
    uint8_t* start,  // May be NULL, meaning an unmarked writer1.
    uint8_t* end,
    uint32_t distance,
    uint32_t length) {
  if (!start || !distance) {
    return 0;
  }
  uint8_t* ptr = *ptr_ptr;
  if ((size_t)(ptr - start) < (size_t)(distance)) {
    return 0;
  }
  start = ptr - distance;
  size_t n = end - ptr;
  if ((size_t)(length) > n) {
    length = n;
  } else {
    n = length;
  }
  // TODO: unrolling by 3 seems best for the std/deflate benchmarks, but that
  // is mostly because 3 is the minimum length for the deflate format. This
  // function implementation shouldn't overfit to that one format. Perhaps the
  // copy_from_history32 Wuffs method should also take an unroll hint argument,
  // and the cgen can look if that argument is the constant expression '3'.
  //
  // See also wuffs_base__writer1__copy_from_history32__bco below.
  //
  // Alternatively, or additionally, have a sloppy_copy_from_history32 method
  // that copies 8 bytes at a time, possibly writing more than length bytes?
  for (; n >= 3; n -= 3) {
    *ptr++ = *start++;
    *ptr++ = *start++;
    *ptr++ = *start++;
  }
  for (; n; n--) {
    *ptr++ = *start++;
  }
  *ptr_ptr = ptr;
  return length;
}

// wuffs_base__writer1__copy_from_history32__bco is a Bounds Check Optimized
// version of the wuffs_base__writer1__copy_from_history32 function above. The
// caller needs to prove that:
//  - start    != NULL
//  - distance >  0
//  - distance <= (*ptr_ptr - start)
//  - length   <= (end      - *ptr_ptr)
static inline uint32_t wuffs_base__writer1__copy_from_history32__bco(
    uint8_t** ptr_ptr,
    uint8_t* start,
    uint8_t* end,
    uint32_t distance,
    uint32_t length) {
  uint8_t* ptr = *ptr_ptr;
  start = ptr - distance;
  uint32_t n = length;
  for (; n >= 3; n -= 3) {
    *ptr++ = *start++;
    *ptr++ = *start++;
    *ptr++ = *start++;
  }
  for (; n; n--) {
    *ptr++ = *start++;
  }
  *ptr_ptr = ptr;
  return length;
}

static inline uint32_t wuffs_base__writer1__copy_from_reader32(
    uint8_t** ptr_wptr,
    uint8_t* wend,
    uint8_t** ptr_rptr,
    uint8_t* rend,
    uint32_t length) {
  uint8_t* wptr = *ptr_wptr;
  size_t n = length;
  if (n > wend - wptr) {
    n = wend - wptr;
  }
  uint8_t* rptr = *ptr_rptr;
  if (n > rend - rptr) {
    n = rend - rptr;
  }
  if (n > 0) {
    memmove(wptr, rptr, n);
    *ptr_wptr += n;
    *ptr_rptr += n;
  }
  return n;
}

static inline uint64_t wuffs_base__writer1__copy_from_slice(
    uint8_t** ptr_wptr,
    uint8_t* wend,
    wuffs_base__slice_u8 src) {
  uint8_t* wptr = *ptr_wptr;
  size_t n = src.len;
  if (n > wend - wptr) {
    n = wend - wptr;
  }
  if (n > 0) {
    memmove(wptr, src.ptr, n);
    *ptr_wptr += n;
  }
  return n;
}

static inline uint32_t wuffs_base__writer1__copy_from_slice32(
    uint8_t** ptr_wptr,
    uint8_t* wend,
    wuffs_base__slice_u8 src,
    uint32_t length) {
  uint8_t* wptr = *ptr_wptr;
  size_t n = src.len;
  if (n > length) {
    n = length;
  }
  if (n > wend - wptr) {
    n = wend - wptr;
  }
  if (n > 0) {
    memmove(wptr, src.ptr, n);
    *ptr_wptr += n;
  }
  return n;
}

// Note that the *__limit and *__mark methods are private (in base-impl.h) not
// public (in base-header.h). We assume that, at the boundary between user code
// and Wuffs code, the reader1 and writer1's private_impl fields (including
// limit and mark) are NULL. Otherwise, some internal assumptions break down.
// For example, limits could be represented as pointers, even though
// conceptually they are counts, but that pointer-to-count correspondence
// becomes invalid if a buffer is re-used (e.g. on resuming a coroutine).
//
// Admittedly, some of the Wuffs test code calls these methods, but that test
// code is still Wuffs code, not user code. Other Wuffs test code modifies
// private_impl fields directly.

static inline wuffs_base__reader1 wuffs_base__reader1__limit(
    wuffs_base__reader1* o,
    uint64_t* ptr_to_len) {
  wuffs_base__reader1 ret = *o;
  ret.private_impl.limit.ptr_to_len = ptr_to_len;
  ret.private_impl.limit.next = &o->private_impl.limit;
  return ret;
}

static inline wuffs_base__empty_struct wuffs_base__reader1__mark(
    wuffs_base__reader1* o,
    uint8_t* mark) {
  o->private_impl.mark = mark;
  return ((wuffs_base__empty_struct){});
}

// TODO: static inline wuffs_base__writer1 wuffs_base__writer1__limit()

static inline wuffs_base__empty_struct wuffs_base__writer1__mark(
    wuffs_base__writer1* o,
    uint8_t* mark) {
  o->private_impl.mark = mark;
  return ((wuffs_base__empty_struct){});
}

static const char* wuffs_base__status__strings[13] = {
    "ok",
    "bad wuffs version",
    "bad receiver",
    "bad argument",
    "initializer not called",
    "invalid I/O operation",
    "closed for writes",
    "unexpected EOF",
    "short read",
    "short write",
    "cannot return a suspension",
    "invalid call sequence",
    "end of data",
};

#endif  // WUFFS_BASE_IMPL_H

// ---------------- Status Codes Implementations

bool wuffs_crc32__status__is_error(wuffs_crc32__status s) {
  return s < 0;
}

const char* wuffs_crc32__status__strings[0] = {};

const char* wuffs_crc32__status__string(wuffs_crc32__status s) {
  const char** a = NULL;
  uint32_t n = 0;
  switch ((s >> 10) & 0x1FFFFF) {
    case 0:
      a = wuffs_base__status__strings;
      n = 13;
      break;
    case wuffs_crc32__packageid:
      a = wuffs_crc32__status__strings;
      n = 0;
      break;
  }
  uint32_t i = s & 0xFF;
  return i < n ? a[i] : "unknown status";
}

// ---------------- Private Consts

static const uint32_t wuffs_crc32__ieee_table[256] = {
    0,          1996959894, 3993919788, 2567524794, 124634137,  1886057615,
    3915621685, 2657392035, 249268274,  2044508324, 3772115230, 2547177864,
    162941995,  2125561021, 3887607047, 2428444049, 498536548,  1789927666,
    4089016648, 2227061214, 450548861,  1843258603, 4107580753, 2211677639,
    325883990,  1684777152, 4251122042, 2321926636, 335633487,  1661365465,
    4195302755, 2366115317, 997073096,  1281953886, 3579855332, 2724688242,
    1006888145, 1258607687, 3524101629, 2768942443, 901097722,  1119000684,
    3686517206, 2898065728, 853044451,  1172266101, 3705015759, 2882616665,
    651767980,  1373503546, 3369554304, 3218104598, 565507253,  1454621731,
    3485111705, 3099436303, 671266974,  1594198024, 3322730930, 2970347812,
    795835527,  1483230225, 3244367275, 3060149565, 1994146192, 31158534,
    2563907772, 4023717930, 1907459465, 112637215,  2680153253, 3904427059,
    2013776290, 251722036,  2517215374, 3775830040, 2137656763, 141376813,
    2439277719, 3865271297, 1802195444, 476864866,  2238001368, 4066508878,
    1812370925, 453092731,  2181625025, 4111451223, 1706088902, 314042704,
    2344532202, 4240017532, 1658658271, 366619977,  2362670323, 4224994405,
    1303535960, 984961486,  2747007092, 3569037538, 1256170817, 1037604311,
    2765210733, 3554079995, 1131014506, 879679996,  2909243462, 3663771856,
    1141124467, 855842277,  2852801631, 3708648649, 1342533948, 654459306,
    3188396048, 3373015174, 1466479909, 544179635,  3110523913, 3462522015,
    1591671054, 702138776,  2966460450, 3352799412, 1504918807, 783551873,
    3082640443, 3233442989, 3988292384, 2596254646, 62317068,   1957810842,
    3939845945, 2647816111, 81470997,   1943803523, 3814918930, 2489596804,
    225274430,  2053790376, 3826175755, 2466906013, 167816743,  2097651377,
    4027552580, 2265490386, 503444072,  1762050814, 4150417245, 2154129355,
    426522225,  1852507879, 4275313526, 2312317920, 282753626,  1742555852,
    4189708143, 2394877945, 397917763,  1622183637, 3604390888, 2714866558,
    953729732,  1340076626, 3518719985, 2797360999, 1068828381, 1219638859,
    3624741850, 2936675148, 906185462,  1090812512, 3747672003, 2825379669,
    829329135,  1181335161, 3412177804, 3160834842, 628085408,  1382605366,
    3423369109, 3138078467, 570562233,  1426400815, 3317316542, 2998733608,
    733239954,  1555261956, 3268935591, 3050360625, 752459403,  1541320221,
    2607071920, 3965973030, 1969922972, 40735498,   2617837225, 3943577151,
    1913087877, 83908371,   2512341634, 3803740692, 2075208622, 213261112,
    2463272603, 3855990285, 2094854071, 198958881,  2262029012, 4057260610,
    1759359992, 534414190,  2176718541, 4139329115, 1873836001, 414664567,
    2282248934, 4279200368, 1711684554, 285281116,  2405801727, 4167216745,
    1634467795, 376229701,  2685067896, 3608007406, 1308918612, 956543938,
    2808555105, 3495958263, 1231636301, 1047427035, 2932959818, 3654703836,
    1088359270, 936918000,  2847714899, 3736837829, 1202900863, 817233897,
    3183342108, 3401237130, 1404277552, 615818150,  3134207493, 3453421203,
    1423857449, 601450431,  3009837614, 3294710456, 1567103746, 711928724,
    3020668471, 3272380065, 1510334235, 755167117,
};

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

// ---------------- Initializer Implementations

void wuffs_crc32__ieee__initialize(wuffs_crc32__ieee* self,
                                   uint32_t wuffs_version,
                                   uint32_t for_internal_use_only) {
  if (!self) {
    return;
  }
  if (wuffs_version != WUFFS_VERSION) {
    self->private_impl.status = WUFFS_CRC32__ERROR_BAD_WUFFS_VERSION;
    return;
  }
  if (for_internal_use_only != WUFFS_BASE__ALREADY_ZEROED) {
    memset(self, 0, sizeof(*self));
  }
  self->private_impl.magic = WUFFS_BASE__MAGIC;
}

// ---------------- Function Implementations

uint32_t wuffs_crc32__ieee__update(wuffs_crc32__ieee* self,
                                   wuffs_base__slice_u8 a_x) {
  if (!self) {
    return 0;
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    self->private_impl.status = WUFFS_CRC32__ERROR_INITIALIZER_NOT_CALLED;
  }
  if (self->private_impl.status < 0) {
    return 0;
  }

  uint32_t v_s;

  v_s = (4294967295 ^ self->private_impl.f_state);
  {
    wuffs_base__slice_u8 i_slice_p = a_x;
    uint8_t* v_p = i_slice_p.ptr;
    uint8_t* i_end1_p = i_slice_p.ptr + i_slice_p.len;
    while (v_p < i_end1_p) {
      v_s = (wuffs_crc32__ieee_table[((uint8_t)((v_s & 255))) ^ *v_p] ^
             (v_s >> 8));
      v_p++;
    }
  }
  self->private_impl.f_state = (4294967295 ^ v_s);
  return self->private_impl.f_state;
}