// Copyright 2020 The Wuffs Authors.
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

// ----------------

/*
This test program is typically run indirectly, by the "wuffs test" or "wuffs
bench" commands. These commands take an optional "-mimic" flag to check that
Wuffs' output mimics (i.e. exactly matches) other libraries' output, such as
giflib for GIF, libpng for PNG, etc.

To manually run this test:

for CC in clang gcc; do
  $CC -std=c99 -Wall -Werror json.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "wuffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
the first "./a.out" with "./a.out -bench". Combine these changes with the
"wuffs mimic cflags" to run the mimic benchmarks.
*/

// !! wuffs mimic cflags: -DWUFFS_MIMIC

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c whitelist which parts of Wuffs to build. That file contains
// the entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__JSON

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
// No mimic library.
#endif

// ---------------- String Conversions Tests

// wuffs_base__private_implementation__high_prec_dec__to_debug_string converts
// hpd into a human-readable NUL-terminated C string.
const char*  //
wuffs_base__private_implementation__high_prec_dec__to_debug_string(
    wuffs_base__private_implementation__high_prec_dec* hpd,
    wuffs_base__slice_u8 dst) {
  if (!hpd) {
    return "high_prec_dec__to_debug_string: invalid hpd";
  }
  uint8_t* p = dst.ptr;
  uint8_t* q = dst.ptr + dst.len;

  // Sign bit.
  if ((q - p) < 1) {
    goto too_short;
  }
  *p++ = hpd->negative ? '-' : '+';

  // Digits and decimal point.
  if (hpd->decimal_point >
      +WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
    // We have "infinity".
    if ((q - p) < 3) {
      goto too_short;
    }
    *p++ = 'i';
    *p++ = 'n';
    *p++ = 'f';
    goto nul_terminator;

  } else if (hpd->decimal_point <
             -WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
    // We have "epsilon": a very small number, equivalent to zero.
    if ((q - p) < 3) {
      goto too_short;
    }
    *p++ = 'e';
    *p++ = 'p';
    *p++ = 's';
    goto nul_terminator;

  } else if (hpd->num_digits == 0) {
    // We have "0".
    if ((q - p) < 1) {
      goto too_short;
    }
    *p++ = '0';
    goto nul_terminator;

  } else if (hpd->decimal_point < 0) {
    // Referring to the wuffs_base__private_implementation__high_prec_dec
    // typedef's comment, we have something like ".00789".
    if ((q - p) < (hpd->num_digits + ((uint32_t)(-hpd->decimal_point)) + 1)) {
      goto too_short;
    }
    uint8_t* src = &hpd->digits[0];

    // Step A.1: write the ".".
    *p++ = '.';

    // Step A.2: write the "00".
    uint32_t n = ((uint32_t)(-hpd->decimal_point));
    if (n > 0) {
      memset(p, '0', n);
      p += n;
    }

    // Step A.3: write the "789".
    n = hpd->num_digits;
    while (n--) {
      *p++ = '0' | *src++;
    }

  } else if (((uint32_t)(hpd->decimal_point)) <= hpd->num_digits) {
    // Referring to the wuffs_base__private_implementation__high_prec_dec
    // typedef's comment, we have something like "78.9".
    if ((q - p) < (hpd->num_digits + 1)) {
      goto too_short;
    }
    uint8_t* src = &hpd->digits[0];

    // Step B.1: write the "78".
    uint32_t n = ((uint32_t)(hpd->decimal_point));
    while (n--) {
      *p++ = '0' | *src++;
    }

    // Step B.2: write the ".".
    *p++ = '.';

    // Step B.3: write the "9".
    n = hpd->num_digits - ((uint32_t)(hpd->decimal_point));
    while (n--) {
      *p++ = '0' | *src++;
    }

  } else {
    // Referring to the wuffs_base__private_implementation__high_prec_dec
    // typedef's comment, we have something like "78900.".
    if ((q - p) < (((uint32_t)(hpd->decimal_point)) + 1)) {
      goto too_short;
    }
    uint8_t* src = &hpd->digits[0];

    // Step C.1: write the "789".
    uint32_t n = hpd->num_digits;
    while (n--) {
      *p++ = '0' | *src++;
    }

    // Step C.2: write the "00".
    n = ((uint32_t)(hpd->decimal_point)) - hpd->num_digits;
    if (n > 0) {
      memset(p, '0', n);
      p += n;
    }

    // Step C.3: write the ".".
    *p++ = '.';
  }

  // Truncated bit.
  if (hpd->truncated) {
    if ((q - p) < 1) {
      goto too_short;
    }
    *p++ = '$';
  }

nul_terminator:
  if ((q - p) < 1) {
    goto too_short;
  }
  *p++ = '\x00';
  return NULL;

too_short:
  return "high_prec_dec__to_debug_string: dst buffer is too short";
}

const char*  //
test_strconv_hpd_rounded_integer() {
  CHECK_FOCUS(__func__);

  struct {
    uint64_t want;
    const char* str;
  } test_cases[] = {
      {.want = 4, .str = "-3.9"},          //
      {.want = 3, .str = "-3.14159"},      //
      {.want = 0, .str = "+0"},            //
      {.want = 0, .str = "0.0000000009"},  //
      {.want = 0, .str = "0.1"},           //
      {.want = 1, .str = "0.9"},           //
      {.want = 12, .str = "1234e-2"},      //
      {.want = 57, .str = "5678e-2"},      //
      {.want = 60, .str = "60.0"},         //
      {.want = 60, .str = "60.4999"},      //
      {.want = 60, .str = "60.5"},         //
      {.want = 60, .str = "60.5000"},      //
      {.want = 61, .str = "60.5001"},      //
      {.want = 61, .str = "60.6"},         //
      {.want = 61, .str = "61.0"},         //
      {.want = 61, .str = "61.4999"},      //
      {.want = 62, .str = "61.5"},         //
      {.want = 62, .str = "61.5000"},      //
      {.want = 62, .str = "61.5001"},      //
      {.want = 62, .str = "61.6"},         //
      {.want = 62, .str = "62.0"},         //
      {.want = 62, .str = "62.4999"},      //
      {.want = 62, .str = "62.5"},         //
      {.want = 62, .str = "62.5000"},      //
      {.want = 63, .str = "62.5001"},      //
      {.want = 63, .str = "62.6"},         //
      {.want = 1000, .str = "999.999"},    //
      {.want = 4560000, .str = "456e+4"},  //

      // With round-to-even, ½ rounds to 0 but "a tiny bit more than ½" rounds
      // to 1, even if the HPD struct truncates that "1" digit.
      {.want = 0, .str = "0.5"},  //
      {.want = 1,                 // 50 '0's per row.
       .str = "0.500000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000000"
              "00000000000000000000000000000000000000000000000001"},  //

      // Inputs with exactly 18 decimal digits before the decimal point.
      {.want = 123456789012345679, .str = "123456789012345678.9"},   //
      {.want = 1000000000000000000, .str = "999999999999999999.9"},  //

      // Inputs with exactly 19 decimal digits before the decimal point.
      {.want = UINT64_MAX, .str = "1234567890123456789"},  //
  };

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    wuffs_base__private_implementation__high_prec_dec hpd;
    CHECK_STATUS(
        "hpd__parse",
        wuffs_base__private_implementation__high_prec_dec__parse(
            &hpd, wuffs_base__make_slice_u8((void*)test_cases[tc].str,
                                            strlen(test_cases[tc].str))));
    uint64_t have =
        wuffs_base__private_implementation__high_prec_dec__rounded_integer(
            &hpd);
    if (have != test_cases[tc].want) {
      RETURN_FAIL("\"%s\": have %" PRIu64 ", want %" PRIu64, test_cases[tc].str,
                  have, test_cases[tc].want);
    }
  }
  return NULL;
}

const char*  //
test_strconv_hpd_shift() {
  CHECK_FOCUS(__func__);

  struct {
    const char* str;
    int32_t shift;  // -ve means left shift, +ve means right shift.
    const char* want;
  } test_cases[] = {
      {.str = "0", .shift = +2, .want = "+0"},                  //
      {.str = "1", .shift = +3, .want = "+.125"},               //
      {.str = "12e3", .shift = +5, .want = "+375."},            //
      {.str = "-0.007", .shift = +8, .want = "-.00002734375"},  //
      {.str = "3.14159E+26",
       .shift = +60,
       .want = "+272489496.244698869986677891574800014495849609375"},  //

      {.str = "0", .shift = -2, .want = "+0"},                //
      {.str = ".125", .shift = -3, .want = "+1."},            //
      {.str = "3750e-1", .shift = -5, .want = "+12000."},     //
      {.str = "-2.734375e-5", .shift = -8, .want = "-.007"},  //
      {.str = "+272489496.244698869986677891574800014495849609375",
       .shift = -60,
       .want = "+314159000000000000000000000."},  //
  };

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    wuffs_base__private_implementation__high_prec_dec hpd;
    CHECK_STATUS(
        "hpd__parse",
        wuffs_base__private_implementation__high_prec_dec__parse(
            &hpd, wuffs_base__make_slice_u8((void*)test_cases[tc].str,
                                            strlen(test_cases[tc].str))));
    int32_t shift = test_cases[tc].shift;
    if (shift > 0) {
      wuffs_base__private_implementation__high_prec_dec__small_rshift(
          &hpd, (uint32_t)(+shift));
    } else if (shift < 0) {
      wuffs_base__private_implementation__high_prec_dec__small_lshift(
          &hpd, (uint32_t)(-shift));
    }

    uint8_t have[1024];
    CHECK_STRING(
        wuffs_base__private_implementation__high_prec_dec__to_debug_string(
            &hpd,
            wuffs_base__make_slice_u8(have, WUFFS_TESTLIB_ARRAY_SIZE(have))));
    if (strcmp(((void*)(have)), test_cases[tc].want)) {
      RETURN_FAIL("\"%s\" %s %" PRId32 ":\n    have: \"%s\"\n    want: \"%s\"",
                  test_cases[tc].str, ((shift > 0) ? ">>" : "<<"),
                  ((shift > 0) ? +shift : -shift), have, test_cases[tc].want);
    }
  }
  return NULL;
}

const char*  //
test_strconv_parse_number_f64() {
  CHECK_FOCUS(__func__);

  const uint64_t fail = 0xDEADBEEF;

  struct {
    uint64_t want;
    const char* str;
  } test_cases[] = {
      {.want = 0x0000000000000000, .str = "+0.0"},
      {.want = 0x0000000000000000, .str = "0"},
      {.want = 0x0000000000000000, .str = "0e0"},
      {.want = 0x0000000000000001, .str = "4.9406564584124654e-324"},
      {.want = 0x000FFFFFFFFFFFFF, .str = "2.2250738585072009E-308"},
      {.want = 0x0010000000000000, .str = "2.2250738585072014E-308"},
      {.want = 0x369C314ABE948EB1,
       .str = "0.0000000000000000000000000000000000000000000012345678900000"},
      {.want = 0x3F88000000000000, .str = "0.01171875"},
      {.want = 0x3FD0000000000000, .str = ".25"},
      {.want = 0x3FD3333333333333,
       .str = "0.2999999999999999888977697537484345957636833190917968750000"},
      {.want = 0x3FD3333333333333, .str = "0.3"},
      {.want = 0x3FD3333333333334, .str = "0.30000000000000004"},
      {.want = 0x3FD3333333333334,
       .str = "0.3000000000000000444089209850062616169452667236328125000000"},
      {.want = 0x3FD5555555555555, .str = "0.333333333333333333333333333333"},
      {.want = 0x3FEFFFFFFFFFFFFF, .str = "0.99999999999999988898"},
      {.want = 0x3FF0000000000000, .str = "0.999999999999999999999999999999"},
      {.want = 0x3FF0000000000000, .str = "1"},
      {.want = 0x3FF0000000000001, .str = "1.0000000000000002"},
      {.want = 0x3FF0000000000002, .str = "1.0000000000000004"},
      {.want = 0x3FF4000000000000, .str = "1.25"},
      {.want = 0x3FF8000000000000, .str = "+1.5"},
      {.want = 0x4000000000000000, .str = "2"},
      {.want = 0x400921FB54442D18, .str = "3.141592653589793238462643383279"},
      {.want = 0x400C000000000000, .str = "3.5"},
      {.want = 0x4014000000000000, .str = "5"},
      {.want = 0x4036000000000000, .str = "22"},
      {.want = 0x4036000000000000, .str = "_+__2_2__."},
      {.want = 0x4037000000000000, .str = "23"},
      {.want = 0x4038000000000000, .str = "2.4E+00000000001"},
      {.want = 0x4038000000000000, .str = "2.4E001"},
      {.want = 0x4038000000000000, .str = "2.4E1"},
      {.want = 0x4038000000000000, .str = "24"},
      {.want = 0x4038000000000000, .str = "2400_00000_00000.00000_e-_1_2"},
      {.want = 0x40FE240C9FCB0C02, .str = "123456.789012"},
      {.want = 0x4330000000000000, .str = "4503599627370496"},  // 1 << 52.
      {.want = 0x4330000000000000, .str = "4503599627370496.5"},
      {.want = 0x4330000000000001, .str = "4503599627370497"},
      {.want = 0x4330000000000002, .str = "4503599627370497.5"},
      {.want = 0x4330000000000002, .str = "4503599627370498"},
      {.want = 0x4340000000000000, .str = "9007199254740992"},  // 1 << 53.
      {.want = 0x4340000000000000, .str = "9007199254740993"},
      {.want = 0x4340000000000001, .str = "9007199254740994"},
      {.want = 0x4340000000000002, .str = "9007199254740995"},
      {.want = 0x4340000000000002, .str = "9007199254740996"},
      {.want = 0x4340000000000002, .str = "9_007__199_254__740_996"},
      {.want = 0x54B249AD2594C37D, .str = "+1E+100"},
      {.want = 0x54B249AD2594C37D, .str = "+_1_E_+_1_0_0_"},
      {.want = 0x7FEFFFFFFFFFFFFF, .str = "1.7976931348623157e308"},
      {.want = 0x7FF0000000000000, .str = "1.8e308"},
      {.want = 0x7FF0000000000000, .str = "1e999"},
      {.want = 0x7FF0000000000000, .str = "__InFinity__"},
      {.want = 0x7FF0000000000000, .str = "inf"},
      {.want = 0x7FFFFFFFFFFFFFFF, .str = "+nan"},
      {.want = 0x7FFFFFFFFFFFFFFF, .str = "_+_NaN_"},
      {.want = 0x7FFFFFFFFFFFFFFF, .str = "nan"},
      {.want = 0x8000000000000000, .str = "-0.000e0"},
      {.want = 0xC008000000000000, .str = "-3"},
      {.want = 0xFFF0000000000000, .str = "-2e308"},
      {.want = 0xFFF0000000000000, .str = "-inf"},
      {.want = 0xFFFFFFFFFFFFFFFF, .str = "-NAN"},

      // We accept either ',' or '.'.
      {.want = 0x3FFC000000000000, .str = "1,75"},
      {.want = 0x3FFC000000000000, .str = "1.75"},

      {.want = fail, .str = " 0"},
      {.want = fail, .str = ""},
      {.want = fail, .str = "."},
      {.want = fail, .str = "00"},
      {.want = fail, .str = "001.2"},
      {.want = fail, .str = "06.44"},
      {.want = fail, .str = "0644"},
      {.want = fail, .str = "1234 67.8e9"},
      {.want = fail, .str = "2,345,678"},  // Two ','s.
      {.want = fail, .str = "2.345,678"},  // One '.' and one ','.
      {.want = fail, .str = "7 "},
      {.want = fail, .str = "7 .9"},
      {.want = fail, .str = "7e"},
      {.want = fail, .str = "7e-"},
      {.want = fail, .str = "7e-+1"},
      {.want = fail, .str = "7e++1"},
      {.want = fail, .str = "NAN "},
      {.want = fail, .str = "NANA"},
      {.want = fail, .str = "inf_inity"},
      {.want = fail, .str = "nun"},
  };

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    wuffs_base__result_f64 r =
        wuffs_base__parse_number_f64(wuffs_base__make_slice_u8(
            (void*)test_cases[tc].str, strlen(test_cases[tc].str)));
    uint64_t have =
        (r.status.repr == NULL)
            ? wuffs_base__ieee_754_bit_representation__from_f64(r.value)
            : fail;
    if (have != test_cases[tc].want) {
      RETURN_FAIL("\"%s\": have 0x%" PRIX64 ", want 0x%" PRIX64,
                  test_cases[tc].str, have, test_cases[tc].want);
    }
  }

  return NULL;
}

const char*  //
test_strconv_parse_number_i64() {
  CHECK_FOCUS(__func__);

  const int64_t fail = 0xDEADBEEF;

  struct {
    int64_t want;
    const char* str;
  } test_cases[] = {
      {.want = +0x0000000000000000, .str = "+0"},
      {.want = +0x0000000000000000, .str = "-0"},
      {.want = +0x0000000000000000, .str = "0"},
      {.want = +0x000000000000012C, .str = "+300"},
      {.want = +0x7FFFFFFFFFFFFFFF, .str = "+9223372036854775807"},
      {.want = +0x7FFFFFFFFFFFFFFF, .str = "9223372036854775807"},
      {.want = -0x0000000000000002, .str = "-2"},
      {.want = -0x00000000000000AB, .str = "_-_0x_AB"},
      {.want = -0x7FFFFFFFFFFFFFFF, .str = "-9223372036854775807"},
      {.want = -0x8000000000000000, .str = "-9223372036854775808"},

      {.want = fail, .str = "+ 1"},
      {.want = fail, .str = "++1"},
      {.want = fail, .str = "+-1"},
      {.want = fail, .str = "+9223372036854775808"},  // 1 << 63.
      {.want = fail, .str = "-"},
      {.want = fail, .str = "-+1"},
      {.want = fail, .str = "-0x8000000000000001"},   // -((1 << 63) + 1).
      {.want = fail, .str = "-9223372036854775809"},  // -((1 << 63) + 1).
      {.want = fail, .str = "0x8000000000000000"},    // 1 << 63.
      {.want = fail, .str = "1-"},
      {.want = fail, .str = "9223372036854775808"},  // 1 << 63.
  };

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    wuffs_base__result_i64 r =
        wuffs_base__parse_number_i64(wuffs_base__make_slice_u8(
            (void*)test_cases[tc].str, strlen(test_cases[tc].str)));
    int64_t have = (r.status.repr == NULL) ? r.value : fail;
    if (have != test_cases[tc].want) {
      RETURN_FAIL("\"%s\": have 0x%" PRIX64 ", want 0x%" PRIX64,
                  test_cases[tc].str, have, test_cases[tc].want);
    }
  }

  return NULL;
}

const char*  //
test_strconv_parse_number_u64() {
  CHECK_FOCUS(__func__);

  const uint64_t fail = 0xDEADBEEF;

  struct {
    uint64_t want;
    const char* str;
  } test_cases[] = {
      {.want = 0x0000000000000000, .str = "0"},
      {.want = 0x0000000000000000, .str = "0_"},
      {.want = 0x0000000000000000, .str = "0d0"},
      {.want = 0x0000000000000000, .str = "0x000"},
      {.want = 0x0000000000000000, .str = "_0"},
      {.want = 0x0000000000000000, .str = "__0__"},
      {.want = 0x000000000000004A, .str = "0x4A"},
      {.want = 0x000000000000004B, .str = "0x__4_B_"},
      {.want = 0x000000000000007B, .str = "123"},
      {.want = 0x000000000000007C, .str = "12_4"},
      {.want = 0x000000000000007D, .str = "_1__2________5_"},
      {.want = 0x00000000000001F4, .str = "0d500"},
      {.want = 0x00000000000001F5, .str = "0D___5_01__"},
      {.want = 0x00000000FFFFFFFF, .str = "4294967295"},
      {.want = 0x0000000100000000, .str = "4294967296"},
      {.want = 0x0123456789ABCDEF, .str = "0x0123456789ABCDEF"},
      {.want = 0x0123456789ABCDEF, .str = "0x0123456789abcdef"},
      {.want = 0xFFFFFFFFFFFFFFF9, .str = "18446744073709551609"},
      {.want = 0xFFFFFFFFFFFFFFFA, .str = "18446744073709551610"},
      {.want = 0xFFFFFFFFFFFFFFFE, .str = "0xFFFFffffFFFFfffe"},
      {.want = 0xFFFFFFFFFFFFFFFE, .str = "18446744073709551614"},
      {.want = 0xFFFFFFFFFFFFFFFF, .str = "0xFFFF_FFFF_FFFF_FFFF"},
      {.want = 0xFFFFFFFFFFFFFFFF, .str = "18446744073709551615"},

      {.want = fail, .str = " "},
      {.want = fail, .str = " 0"},
      {.want = fail, .str = " 12 "},
      {.want = fail, .str = ""},
      {.want = fail, .str = "+0"},
      {.want = fail, .str = "+1"},
      {.want = fail, .str = "-0"},
      {.want = fail, .str = "-1"},
      {.want = fail, .str = "0 "},
      {.want = fail, .str = "00"},
      {.want = fail, .str = "000000x"},
      {.want = fail, .str = "000000x0"},
      {.want = fail, .str = "007"},
      {.want = fail, .str = "0644"},
      {.want = fail, .str = "0_0"},
      {.want = fail, .str = "0_x1"},
      {.want = fail, .str = "0d___"},
      {.want = fail, .str = "0x"},
      {.want = fail, .str = "0x10000000000000000"},      // 1 << 64.
      {.want = fail, .str = "0x1_0000_0000_0000_0000"},  // 1 << 64.
      {.want = fail, .str = "1 23"},
      {.want = fail, .str = "1,23"},
      {.want = fail, .str = "1.23"},
      {.want = fail, .str = "123 "},
      {.want = fail, .str = "123456789012345678901234"},
      {.want = fail, .str = "12a3"},
      {.want = fail, .str = "18446744073709551616"},  // UINT64_MAX.
      {.want = fail, .str = "18446744073709551617"},
      {.want = fail, .str = "18446744073709551618"},
      {.want = fail, .str = "18446744073709551619"},
      {.want = fail, .str = "18446744073709551620"},
      {.want = fail, .str = "18446744073709551621"},
      {.want = fail, .str = "_"},
      {.want = fail, .str = "d"},
      {.want = fail, .str = "x"},
  };

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    wuffs_base__result_u64 r =
        wuffs_base__parse_number_u64(wuffs_base__make_slice_u8(
            (void*)test_cases[tc].str, strlen(test_cases[tc].str)));
    uint64_t have = (r.status.repr == NULL) ? r.value : fail;
    if (have != test_cases[tc].want) {
      RETURN_FAIL("\"%s\": have 0x%" PRIX64 ", want 0x%" PRIX64,
                  test_cases[tc].str, have, test_cases[tc].want);
    }
  }

  return NULL;
}

const char*  //
test_strconv_utf_8_next() {
  CHECK_FOCUS(__func__);

  // Special case the "\x00" string, which is valid UTF-8 but its strlen is
  // zero, not one.
  uint8_t the_nul_byte[1];
  the_nul_byte[0] = '\x00';

  struct {
    uint32_t want_cp;
    uint32_t want_bl;
    const char* str;
  } test_cases[] = {
      {.want_cp = 0x00000000, .want_bl = 0, .str = ""},
      {.want_cp = 0x00000000, .want_bl = 1, .str = "The <NUL> byte"},
      {.want_cp = 0x00000009, .want_bl = 1, .str = "\t"},
      {.want_cp = 0x00000041, .want_bl = 1, .str = "A"},
      {.want_cp = 0x00000061, .want_bl = 1, .str = "abdefghij"},
      {.want_cp = 0x0000007F, .want_bl = 1, .str = "\x7F"},
      {.want_cp = 0x00000080, .want_bl = 2, .str = "\xC2\x80"},
      {.want_cp = 0x000007FF, .want_bl = 2, .str = "\xDF\xBF"},
      {.want_cp = 0x00000800, .want_bl = 3, .str = "\xE0\xA0\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 3, .str = "\xEF\xBF\xBD"},
      {.want_cp = 0x0000FFFF, .want_bl = 3, .str = "\xEF\xBF\xBF"},
      {.want_cp = 0x00010000, .want_bl = 4, .str = "\xF0\x90\x80\x80"},
      {.want_cp = 0x0010FFFF, .want_bl = 4, .str = "\xF4\x8F\xBF\xBF"},

      // U+00000394 GREEK CAPITAL LETTER DELTA.
      {.want_cp = 0x00000394, .want_bl = 2, .str = "\xCE\x94"},
      {.want_cp = 0x00000394, .want_bl = 2, .str = "\xCE\x94+"},
      {.want_cp = 0x00000394, .want_bl = 2, .str = "\xCE\x94++"},
      {.want_cp = 0x00000394, .want_bl = 2, .str = "\xCE\x94+++"},
      {.want_cp = 0x00000394, .want_bl = 2, .str = "\xCE\x94++++"},
      {.want_cp = 0x00000394, .want_bl = 2, .str = "\xCE\x94\x80"},
      {.want_cp = 0x00000394, .want_bl = 2, .str = "\xCE\x94\x80\x80"},
      {.want_cp = 0x00000394, .want_bl = 2, .str = "\xCE\x94\x80\x80\x80"},
      {.want_cp = 0x00000394, .want_bl = 2, .str = "\xCE\x94\x80\x80\x80\x80"},

      // U+00002603 SNOWMAN.
      {.want_cp = 0x00002603, .want_bl = 3, .str = "\xE2\x98\x83"},
      {.want_cp = 0x00002603, .want_bl = 3, .str = "\xE2\x98\x83+"},
      {.want_cp = 0x00002603, .want_bl = 3, .str = "\xE2\x98\x83++"},
      {.want_cp = 0x00002603, .want_bl = 3, .str = "\xE2\x98\x83+++"},
      {.want_cp = 0x00002603, .want_bl = 3, .str = "\xE2\x98\x83++++"},
      {.want_cp = 0x00002603, .want_bl = 3, .str = "\xE2\x98\x83\xFF"},

      // U+0001F4A9 PILE OF POO.
      {.want_cp = 0x0001F4A9, .want_bl = 4, .str = "\xF0\x9F\x92\xA9"},
      {.want_cp = 0x0001F4A9, .want_bl = 4, .str = "\xF0\x9F\x92\xA9+"},
      {.want_cp = 0x0001F4A9, .want_bl = 4, .str = "\xF0\x9F\x92\xA9++"},
      {.want_cp = 0x0001F4A9, .want_bl = 4, .str = "\xF0\x9F\x92\xA9+++"},
      {.want_cp = 0x0001F4A9, .want_bl = 4, .str = "\xF0\x9F\x92\xA9++++"},
      {.want_cp = 0x0001F4A9, .want_bl = 4, .str = "\xF0\x9F\x92\xA9\xFF"},

      // Invalid.
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xBF"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xC0\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xC1\xBF"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xC2"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xC2\x7F"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xC2\xC0"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xC2\xFF"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xCE"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xDF\xC0"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xDF\xFF"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xE0\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xE0\x80\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xE0\x9F\xBF"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xE2"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xF0"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xF0\x80\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xF0\x80\x80\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xF0\x8F\xBF\xBF"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xF4\x90\x80\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xF5"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xF6\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xF7\x80\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xFF\xFF\xFF\xFF"},

      // Invalid. UTF-8 cannot contain the surrogates U+D800 ..= U+DFFF.
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xED\xA0\x80"},
      {.want_cp = 0x0000FFFD, .want_bl = 1, .str = "\xED\xBF\xBF"},
  };

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    wuffs_base__slice_u8 s = wuffs_base__make_slice_u8(
        (void*)test_cases[tc].str, strlen(test_cases[tc].str));

    // Override "The <NUL> byte" with "\x00".
    if ((test_cases[tc].want_cp == 0) && (test_cases[tc].want_bl == 1)) {
      s = wuffs_base__make_slice_u8(&the_nul_byte[0], 1);
    }

    wuffs_base__utf_8__next__output have = wuffs_base__utf_8__next(s);
    if ((have.code_point != test_cases[tc].want_cp) ||
        (have.byte_length != test_cases[tc].want_bl)) {
      RETURN_FAIL("\"%s\": have cp=0x%" PRIX32 " bl=%" PRIu32
                  ", want cp=0x%" PRIX32 " bl=%" PRIu32,
                  test_cases[tc].str, have.code_point, have.byte_length,
                  test_cases[tc].want_cp, test_cases[tc].want_bl);
    }
  }

  return NULL;
}

// ---------------- Golden Tests

golden_test json_australian_abc_gt = {
    .want_filename = "test/data/australian-abc-local-stations.tokens",  //
    .src_filename = "test/data/australian-abc-local-stations.json",     //
};

golden_test json_file_sizes_gt = {
    .src_filename = "test/data/file-sizes.json",  //
};

golden_test json_github_tags_gt = {
    .src_filename = "test/data/github-tags.json",  //
};

golden_test json_json_things_unformatted_gt = {
    .want_filename = "test/data/json-things.unformatted.tokens",  //
    .src_filename = "test/data/json-things.unformatted.json",     //
};

golden_test json_nobel_prizes_gt = {
    .src_filename = "test/data/nobel-prizes.json",  //
};

// ---------------- JSON Tests

const char*  //
test_wuffs_json_decode_interface() {
  CHECK_FOCUS(__func__);

  {
    wuffs_json__decoder dec;
    CHECK_STATUS("initialize",
                 wuffs_json__decoder__initialize(
                     &dec, sizeof dec, WUFFS_VERSION,
                     WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
    CHECK_STRING(do_test__wuffs_base__token_decoder(
        wuffs_json__decoder__upcast_as__wuffs_base__token_decoder(&dec),
        &json_json_things_unformatted_gt));
  }

  {
    wuffs_json__decoder dec;
    CHECK_STATUS("initialize",
                 wuffs_json__decoder__initialize(
                     &dec, sizeof dec, WUFFS_VERSION,
                     WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
    CHECK_STRING(do_test__wuffs_base__token_decoder(
        wuffs_json__decoder__upcast_as__wuffs_base__token_decoder(&dec),
        &json_australian_abc_gt));
  }

  return NULL;
}

const char*  //
wuffs_json_decode(wuffs_base__token_buffer* tok,
                  wuffs_base__io_buffer* src,
                  uint32_t wuffs_initialize_flags,
                  uint64_t wlimit,
                  uint64_t rlimit) {
  wuffs_json__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_json__decoder__initialize(&dec, sizeof dec, WUFFS_VERSION,
                                               wuffs_initialize_flags));

  while (true) {
    wuffs_base__token_buffer limited_tok =
        make_limited_token_writer(*tok, wlimit);
    wuffs_base__io_buffer limited_src = make_limited_reader(*src, rlimit);

    wuffs_base__status status =
        wuffs_json__decoder__decode_tokens(&dec, &limited_tok, &limited_src);

    tok->meta.wi += limited_tok.meta.wi;
    src->meta.ri += limited_src.meta.ri;

    if (((wlimit < UINT64_MAX) &&
         (status.repr == wuffs_base__suspension__short_write)) ||
        ((rlimit < UINT64_MAX) &&
         (status.repr == wuffs_base__suspension__short_read))) {
      continue;
    }
    return status.repr;
  }
}

const char*  //
test_wuffs_json_decode_long_numbers() {
  CHECK_FOCUS(__func__);

  // Spelling "false" with four letters helps clang-format align the test
  // cases, when viewed in a fixed width font.
  const bool fals = false;

  // Each test case produces multiple test strings: the test_cases[tc].suffix
  // field is prefixed with N '9's, for multiple values of N, so that the test
  // string's total length is near WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL.
  // For example, a ".2e4" suffix means an overall string of "999etc999.2e4".
  //
  // The test_cases[tc].valid field holds the whether overall string is a valid
  // JSON number.
  struct {
    bool valid;
    const char* suffix;
  } test_cases[] = {
      {.valid = true, .suffix = ""},           //
      {.valid = true, .suffix = " "},          //
      {.valid = fals, .suffix = "."},          //
      {.valid = fals, .suffix = ". "},         //
      {.valid = fals, .suffix = "E"},          //
      {.valid = fals, .suffix = "E "},         //
      {.valid = fals, .suffix = "E-"},         //
      {.valid = fals, .suffix = "E- "},        //
      {.valid = true, .suffix = "e2"},         //
      {.valid = true, .suffix = "e2 "},        //
      {.valid = true, .suffix = "e+34"},       //
      {.valid = true, .suffix = "e+34 "},      //
      {.valid = true, .suffix = ".2"},         //
      {.valid = true, .suffix = ".2 "},        //
      {.valid = fals, .suffix = ".2e"},        //
      {.valid = fals, .suffix = ".2e "},       //
      {.valid = fals, .suffix = ".2e+"},       //
      {.valid = fals, .suffix = ".2e+ "},      //
      {.valid = true, .suffix = ".2e4"},       //
      {.valid = true, .suffix = ".2e4 "},      //
      {.valid = true, .suffix = ".2E+5"},      //
      {.valid = true, .suffix = ".2E+5 "},     //
      {.valid = true, .suffix = ".2e-5678"},   //
      {.valid = true, .suffix = ".2e-5678 "},  //
  };

  // src_array holds the overall test string. 119 is arbitrary but long enough.
  // See the "if (suffix_length > etc)" check below. 102 is also arbitrary but
  // larger than WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL.
  //
  // See also test_wuffs_json_decode_src_io_buffer_length.
  uint8_t src_array[119];
  memset(&src_array[0], '9', 102);
  if (102 <= WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL) {
    RETURN_FAIL("insufficient number_length test case coverage");
  }

  wuffs_json__decoder dec;

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    size_t suffix_length = strlen(test_cases[tc].suffix);
    if ((suffix_length + 1) > (119 - 102)) {  // +1 for the terminal NUL.
      RETURN_FAIL("tc=%d: src_array is too short", tc);
    }
    bool ends_with_space = (suffix_length > 0) &&
                           (test_cases[tc].suffix[suffix_length - 1] == ' ');

    // Copying the terminal NUL isn't necessary for Wuffs' slices (which are a
    // pointer-length pair), but this backstop can help debugging with printf
    // where "%s" takes a C string (a bare pointer).
    memcpy(&src_array[102], test_cases[tc].suffix, suffix_length + 1);

    size_t nines_length;
    for (nines_length = 90; nines_length < 102; nines_length++) {
      wuffs_base__slice_u8 src_data = ((wuffs_base__slice_u8){
          .ptr = &src_array[102 - nines_length],
          .len = nines_length + suffix_length,
      });
      size_t number_length = src_data.len - (ends_with_space ? 1 : 0);

      int closed;
      for (closed = 0; closed < 2; closed++) {
        CHECK_STATUS(
            "initialize",
            wuffs_json__decoder__initialize(
                &dec, sizeof dec, WUFFS_VERSION,
                WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

        wuffs_base__token_buffer tok = ((wuffs_base__token_buffer){
            .data = global_have_token_slice,
        });

        wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
            .data = src_data,
            .meta = wuffs_base__make_io_buffer_meta(src_data.len, 0, 0,
                                                    closed != 0),
        });

        const char* have =
            wuffs_json__decoder__decode_tokens(&dec, &tok, &src).repr;

        size_t total_length = 0;
        while (tok.meta.ri < tok.meta.wi) {
          wuffs_base__token* t = &tok.data.ptr[tok.meta.ri++];
          total_length += wuffs_base__token__length(t);
        }
        if (total_length != src.meta.ri) {
          RETURN_FAIL(
              "tc=%d, nines_length=%zu, closed=%d: total_length: have %zu, "
              "want %zu",
              tc, nines_length, closed, total_length, src.meta.ri);
        }

        const char* want;
        if (number_length > WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL) {
          want = wuffs_json__error__unsupported_number_length;
        } else if (closed || ends_with_space) {
          want = test_cases[tc].valid ? NULL : wuffs_json__error__bad_input;
        } else {
          want = wuffs_base__suspension__short_read;
        }

        if (have != want) {
          RETURN_FAIL(
              "tc=%d, nines_length=%zu, closed=%d: have \"%s\", want \"%s\"",
              tc, nines_length, closed, have, want);
        }
      }
    }
  }

  return NULL;
}

// test_wuffs_json_decode_prior_valid_utf_8 tests that when encountering
// invalid or incomplete UTF-8, or a backslash-escape, any prior valid UTF-8 is
// still output. The decoder batches output so that, ignoring the quotation
// marks, "abc\xCE\x94efg" can be a single 8-length token instead of multiple
// (e.g. 3+2+3) tokens. On the other hand, while "abc\xFF" ends with one byte
// of invalid UTF-8, the 3 good bytes before that should still be output.
const char*  //
test_wuffs_json_decode_prior_valid_utf_8() {
  CHECK_FOCUS(__func__);

  // The test cases contain combinations of valid, partial and invalid UTF-8:
  //  - "\xCE\x94"         is U+00000394 GREEK CAPITAL LETTER DELTA.
  //  - "\xE2\x98\x83"     is U+00002603 SNOWMAN.
  //  - "\xF0\x9F\x92\xA9" is U+0001F4A9 PILE OF POO.
  //
  // The code below can also add trailing 's' bytes, which change e.g. the
  // partial multi-byte UTF-8 "\xE2" to be the invalid UTF-8 "\xE2s".
  const char* test_cases[] = {
      "",
      "\\t",
      "\\u",
      "\\u1234",
      "\x1F",  // Valid UTF-8 but invalid in a JSON string.
      "\x20",
      "\xCE",
      "\xCE\x94",
      "\xE2",
      "\xE2\x98",
      "\xE2\x98\x83",
      "\xE2\x98\x83\xCE",
      "\xE2\x98\x83\xCE\x94",
      "\xF0",
      "\xF0\x9F",
      "\xF0\x9F\x92",
      "\xF0\x9F\x92\xA9",
      "\xF0\x9F\x92\xA9\xCE",
      "\xF0\x9F\x92\xA9\xCE\x94",
  };

  size_t prefixes[] = {
      0,
      1,
      15,
      WUFFS_BASE__TOKEN__LENGTH__MAX_INCL - 9,
      WUFFS_BASE__TOKEN__LENGTH__MAX_INCL - 8,
      WUFFS_BASE__TOKEN__LENGTH__MAX_INCL - 7,
      WUFFS_BASE__TOKEN__LENGTH__MAX_INCL - 6,
      WUFFS_BASE__TOKEN__LENGTH__MAX_INCL - 5,
      WUFFS_BASE__TOKEN__LENGTH__MAX_INCL - 4,
      WUFFS_BASE__TOKEN__LENGTH__MAX_INCL - 3,
      WUFFS_BASE__TOKEN__LENGTH__MAX_INCL - 2,
      WUFFS_BASE__TOKEN__LENGTH__MAX_INCL - 1,
      WUFFS_BASE__TOKEN__LENGTH__MAX_INCL + 0,
  };

  size_t suffixes[] = {0, 1, 17};

  wuffs_json__decoder dec;

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    size_t n = strlen(test_cases[tc]);
    size_t num_preceding = 0;
    while (num_preceding < n) {
      wuffs_base__utf_8__next__output x =
          wuffs_base__utf_8__next(wuffs_base__make_slice_u8(
              (void*)(test_cases[tc]) + num_preceding, n - num_preceding));
      if (!wuffs_base__utf_8__next__output__is_valid(&x) ||
          (x.code_point < 0x20) || (x.code_point == '\\')) {
        break;
      }
      num_preceding += x.byte_length;
      if (num_preceding > n) {
        RETURN_FAIL("tc=%d: utf_8__next overflow", tc);
      }
    }

    int pre;
    for (pre = 0; pre < WUFFS_TESTLIB_ARRAY_SIZE(prefixes); pre++) {
      size_t prefix = prefixes[pre];

      int suf;
      for (suf = 0; suf < WUFFS_TESTLIB_ARRAY_SIZE(suffixes); suf++) {
        size_t suffix = suffixes[suf];

        // Set src to "\"ppp...pppMIDDLEsss...sss", with a leading quotation
        // mark, where prefix and suffix are the number of 'p's and 's's and
        // test_cases[tc] is the "MIDDLE".
        wuffs_base__slice_u8 src_data = ((wuffs_base__slice_u8){
            .ptr = global_src_array,
            .len = 1 + prefix + n + suffix,
        });
        if (src_data.len > IO_BUFFER_ARRAY_SIZE) {
          RETURN_FAIL("total src length is too long");
        }
        src_data.ptr[0] = '\"';
        memset(&src_data.ptr[1], 'p', prefix);
        memcpy(&src_data.ptr[1 + prefix], test_cases[tc], n);
        memset(&src_data.ptr[1 + prefix + n], 's', suffix);

        int closed;
        for (closed = 0; closed < 2; closed++) {
          CHECK_STATUS("initialize", wuffs_json__decoder__initialize(
                                         &dec, sizeof dec, WUFFS_VERSION, 0));

          wuffs_base__token_buffer tok = ((wuffs_base__token_buffer){
              .data = global_have_token_slice,
          });

          wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
              .data = src_data,
              .meta = wuffs_base__make_io_buffer_meta(src_data.len, 0, 0,
                                                      closed != 0),
          });

          wuffs_json__decoder__decode_tokens(&dec, &tok, &src);

          size_t have = 0;
          while (tok.meta.ri < tok.meta.wi) {
            wuffs_base__token* t = &tok.data.ptr[tok.meta.ri++];
            uint64_t vbc = wuffs_base__token__value_base_category(t);
            if (vbc == WUFFS_BASE__TOKEN__VBC__UNICODE_CODE_POINT) {
              break;
            } else if (vbc == WUFFS_BASE__TOKEN__VBC__STRING) {
              have += wuffs_base__token__length(t);
            } else {
              RETURN_FAIL(
                  "tc=%d, prefix=%zu, suffix=%zu, closed=%d: unexpected token",
                  tc, prefix, suffix, closed);
            }
          }
          size_t want = 1 + prefix + num_preceding;  // 1 for the leading '\"'.
          if (num_preceding == n) {
            want += suffix;
          }
          if (have != want) {
            RETURN_FAIL(
                "tc=%d, prefix=%zu, suffix=%zu, closed=%d: have %zu, want %zu",
                tc, prefix, suffix, closed, have, want);
          }
        }
      }
    }
  }

  return NULL;
}

const char*  //
test_wuffs_json_decode_unicode4_escapes() {
  CHECK_FOCUS(__func__);

  const uint32_t fail = 0xDEADBEEF;

  struct {
    uint32_t want;
    const char* str;
  } test_cases[] = {
      // Simple (non-surrogate) successes.
      {.want = 0x0000000A, .str = "\"\\u000a\""},
      {.want = 0x0000005C, .str = "\"\\\\u1234\""},  // U+005C is '\\'.
      {.want = 0x00001000, .str = "\"\\u10002345\""},
      {.want = 0x00001000, .str = "\"\\u1000234\""},
      {.want = 0x00001000, .str = "\"\\u100023\""},
      {.want = 0x00001000, .str = "\"\\u10002\""},
      {.want = 0x00001234, .str = "\"\\u1234\""},
      {.want = 0x0000D7FF, .str = "\"\\ud7ff\""},
      {.want = 0x0000E000, .str = "\"\\uE000\""},
      {.want = 0x0000FFFF, .str = "\"\\uFffF\""},

      // Unicode surrogate pair. U+0001F4A9 PILE OF POO is (U+D83D, U+DCA9),
      // because ((0x03D << 10) | 0x0A9) is 0xF4A9:
      //  - High surrogates are in the range U+D800 ..= U+DBFF.
      //  - Low  surrogates are in the range U+DC00 ..= U+DFFF.
      {.want = 0x0001F4A9, .str = "\"\\uD83D\\udca9\""},

      // More surrogate pairs.
      {.want = 0x00010000, .str = "\"\\uD800\\uDC00\""},
      {.want = 0x0010FFFF, .str = "\"\\uDBFF\\uDFFF\""},

      // Simple (non-surrogate) failures.
      {.want = fail, .str = "\"\\U1234\""},
      {.want = fail, .str = "\"\\u123"},
      {.want = fail, .str = "\"\\u123\""},
      {.want = fail, .str = "\"\\u123x\""},
      {.want = fail, .str = "\"u1234\""},

      // Invalid surrogate pairs.
      {.want = fail, .str = "\"\\uD800\""},         // High alone.
      {.want = fail, .str = "\"\\uD83D?udca9\""},   // High then not "\\u".
      {.want = fail, .str = "\"\\uD83D\\ud7ff\""},  // High then non-surrogate.
      {.want = fail, .str = "\"\\uD83D\\udbff\""},  // High then high.
      {.want = fail, .str = "\"\\uD83D\\ue000\""},  // High then non-surrogate.
      {.want = fail, .str = "\"\\uDC00\""},         // Low alone.
      {.want = fail, .str = "\"\\uDC00\\u0000\""},  // Low then non-surrogate.
      {.want = fail, .str = "\"\\uDC00\\ud800\""},  // Low then high.
      {.want = fail, .str = "\"\\uDC00\\udfff\""},  // Low then low.
      {.want = fail, .str = "\"\\uDFFF1234\""},     // Low alone.
  };

  wuffs_json__decoder dec;
  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    CHECK_STATUS("initialize",
                 wuffs_json__decoder__initialize(
                     &dec, sizeof dec, WUFFS_VERSION,
                     WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

    wuffs_base__token_buffer tok = ((wuffs_base__token_buffer){
        .data = global_have_token_slice,
    });
    size_t n = strlen(test_cases[tc].str);
    wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
        .data = wuffs_base__make_slice_u8((void*)(test_cases[tc].str), n),
        .meta = wuffs_base__make_io_buffer_meta(n, 0, 0, true),
    });

    wuffs_json__decoder__decode_tokens(&dec, &tok, &src);

    uint32_t have = fail;
    uint64_t total_length = 0;
    size_t i;
    for (i = tok.meta.ri; i < tok.meta.wi; i++) {
      wuffs_base__token* t = &tok.data.ptr[i];
      total_length =
          wuffs_base__u64__sat_add(total_length, wuffs_base__token__length(t));

      // Set have to the first Unicode code point token.
      if ((have == fail) && ((wuffs_base__token__value_base_category(t) ==
                              WUFFS_BASE__TOKEN__VBC__UNICODE_CODE_POINT))) {
        have = wuffs_base__token__value_base_detail(t);
        if (have > 0x10FFFF) {  // This also catches "have == fail".
          RETURN_FAIL("%s: invalid Unicode code point", test_cases[tc].str);
        }

        uint64_t have_length = wuffs_base__token__length(t);
        uint64_t want_length = (have == 0x5C) ? 2 : ((have <= 0xFFFF) ? 6 : 12);
        if (have_length != want_length) {
          RETURN_FAIL("%s: token length: have %" PRIu64 ", want %" PRIu64,
                      test_cases[tc].str, have_length, want_length);
        }
      }
    }

    if (have != test_cases[tc].want) {
      RETURN_FAIL("%s: have 0x%" PRIX32 ", want 0x%" PRIX32, test_cases[tc].str,
                  have, test_cases[tc].want);
    }

    if (total_length != src.meta.ri) {
      RETURN_FAIL("%s: total length: have %" PRIu64 ", want %" PRIu64,
                  test_cases[tc].str, total_length, src.meta.ri);
    }
  }

  return NULL;
}

// test_wuffs_json_decode_src_io_buffer_length tests that given a sufficient
// amount of source data (WUFFS_JSON__DECODER_SRC_IO_BUFFER_LENGTH_MIN_INCL or
// more), decoding will always return a conclusive result, not a suspension
// such as "$short read".
//
// The JSON specification doesn't give a maximum byte length for a number, but
// implementations are permitted to impose one. Wuffs' implementation imposes
// WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL.
const char*  //
test_wuffs_json_decode_src_io_buffer_length() {
  CHECK_FOCUS(__func__);

  if (WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL >=
      WUFFS_JSON__DECODER_SRC_IO_BUFFER_LENGTH_MIN_INCL) {
    RETURN_FAIL(
        "inconsistent WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL vs "
        "WUFFS_JSON__DECODER_SRC_IO_BUFFER_LENGTH_MIN_INCL");
  }

  // src_array holds the test string of repeated '7's. 107 is arbitrary but
  // long enough for the loop below.
  uint8_t src_array[107];
  memset(&src_array[0], '7', 107);

  wuffs_json__decoder dec;

  int i;
  for (i = WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL - 2;
       i <= WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL + 2; i++) {
    if (i < 0) {
      RETURN_FAIL("invalid test case: i=%d", i);
    } else if (i > 107) {
      RETURN_FAIL("invalid test case: i=%d", i);
    }

    wuffs_base__slice_u8 src_data = ((wuffs_base__slice_u8){
        .ptr = &src_array[0],
        .len = i,
    });

    int closed;
    for (closed = 0; closed < 2; closed++) {
      wuffs_base__token_buffer tok = ((wuffs_base__token_buffer){
          .data = global_have_token_slice,
      });
      wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
          .data = src_data,
          .meta =
              wuffs_base__make_io_buffer_meta(src_data.len, 0, 0, closed != 0),
      });
      CHECK_STATUS("initialize",
                   wuffs_json__decoder__initialize(
                       &dec, sizeof dec, WUFFS_VERSION,
                       WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

      wuffs_base__status have =
          wuffs_json__decoder__decode_tokens(&dec, &tok, &src);
      const char* want =
          (i > WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL)
              ? wuffs_json__error__unsupported_number_length
              : (closed ? NULL : wuffs_base__suspension__short_read);
      if (have.repr != want) {
        RETURN_FAIL("i=%d, closed=%d: have \"%s\", want \"%s\"", i, closed,
                    have.repr, want);
      }

      if ((i >= WUFFS_JSON__DECODER_SRC_IO_BUFFER_LENGTH_MIN_INCL) &&
          wuffs_base__status__is_suspension(&have)) {
        RETURN_FAIL("i=%d, closed=%d: have a suspension", i, closed);
      }
    }
  }

  return NULL;
}

const char*  //
test_wuffs_json_decode_string() {
  CHECK_FOCUS(__func__);

  const char* bad_bac = wuffs_json__error__bad_backslash_escape;
  const char* bad_ccc = wuffs_json__error__bad_c0_control_code;
  const char* bad_utf = wuffs_json__error__bad_utf_8;

  struct {
    const char* want_status_repr;
    const char* str;
  } test_cases[] = {
      {.want_status_repr = NULL, .str = "\"+++\\\"+\\/+\\\\+++\""},
      {.want_status_repr = NULL, .str = "\"+++\\b+\\f+\\n+\\r+\\t+++\""},
      {.want_status_repr = NULL, .str = "\"\x20\""},              // U+00000020.
      {.want_status_repr = NULL, .str = "\"\xC2\x80\""},          // U+00000080.
      {.want_status_repr = NULL, .str = "\"\xCE\x94\""},          // U+00000394.
      {.want_status_repr = NULL, .str = "\"\xDF\xBF\""},          // U+000007FF.
      {.want_status_repr = NULL, .str = "\"\xE0\xA0\x80\""},      // U+00000800.
      {.want_status_repr = NULL, .str = "\"\xE2\x98\x83\""},      // U+00002603.
      {.want_status_repr = NULL, .str = "\"\xED\x80\x80\""},      // U+0000D000.
      {.want_status_repr = NULL, .str = "\"\xED\x9F\xBF\""},      // U+0000D7FF.
      {.want_status_repr = NULL, .str = "\"\xEE\x80\x80\""},      // U+0000E000.
      {.want_status_repr = NULL, .str = "\"\xEF\xBF\xBD\""},      // U+0000FFFD.
      {.want_status_repr = NULL, .str = "\"\xEF\xBF\xBF\""},      // U+0000FFFF.
      {.want_status_repr = NULL, .str = "\"\xF0\x90\x80\x80\""},  // U+00010000.
      {.want_status_repr = NULL, .str = "\"\xF0\x9F\x92\xA9\""},  // U+0001F4A9.
      {.want_status_repr = NULL, .str = "\"\xF0\xB0\x80\x81\""},  // U+00030001.
      {.want_status_repr = NULL, .str = "\"\xF1\xB0\x80\x82\""},  // U+00070002.
      {.want_status_repr = NULL, .str = "\"\xF3\xB0\x80\x83\""},  // U+000F0003.
      {.want_status_repr = NULL, .str = "\"\xF4\x80\x80\x84\""},  // U+00100004.
      {.want_status_repr = NULL, .str = "\"\xF4\x8F\xBF\xBF\""},  // U+0010FFFF.
      {.want_status_repr = NULL, .str = "\"abc\""},
      {.want_status_repr = NULL, .str = "\"i\x6Ak\""},
      {.want_status_repr = NULL, .str = "\"space+\x20+space\""},
      {.want_status_repr = NULL, .str = "\"tab+\\t+tab\""},
      {.want_status_repr = NULL, .str = "\"tab+\\u0009+tab\""},

      {.want_status_repr = bad_bac, .str = "\"\\uIJKL\""},
      {.want_status_repr = bad_bac, .str = "\"space+\\x20+space\""},

      {.want_status_repr = bad_ccc, .str = "\"\x1F\""},
      {.want_status_repr = bad_ccc, .str = "\"tab+\t+tab\""},

      {.want_status_repr = bad_utf, .str = "\"\x80\""},
      {.want_status_repr = bad_utf, .str = "\"\xBF\""},
      {.want_status_repr = bad_utf, .str = "\"\xC1\x80\""},
      {.want_status_repr = bad_utf, .str = "\"\xC2\x7F\""},
      {.want_status_repr = bad_utf, .str = "\"\xDF\xC0\""},
      {.want_status_repr = bad_utf, .str = "\"\xDF\xFF\""},
      {.want_status_repr = bad_utf, .str = "\"\xE0\x9F\xBF\""},
      {.want_status_repr = bad_utf, .str = "\"\xED\xA0\x80\""},  // U+0000D800.
      {.want_status_repr = bad_utf, .str = "\"\xED\xAF\xBF\""},  // U+0000DBFF.
      {.want_status_repr = bad_utf, .str = "\"\xED\xB0\x80\""},  // U+0000DC00.
      {.want_status_repr = bad_utf, .str = "\"\xED\xBF\xBF\""},  // U+0000DFFF.
      {.want_status_repr = bad_utf, .str = "\"\xF0\x80\x80\""},
      {.want_status_repr = bad_utf, .str = "\"\xF0\x8F\xBF\xBF\""},
      {.want_status_repr = bad_utf, .str = "\"\xF2\x7F\x80\x80\""},
      {.want_status_repr = bad_utf, .str = "\"\xF2\x80\x7F\x80\""},
      {.want_status_repr = bad_utf, .str = "\"\xF2\x80\x80\x7F\""},
      {.want_status_repr = bad_utf, .str = "\"\xF4\x90\x80\x80\""},
      {.want_status_repr = bad_utf, .str = "\"\xF5\""},
      {.want_status_repr = bad_utf, .str = "\"\xFF\xFF\xFF\xFF\""},
  };

  wuffs_json__decoder dec;
  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    CHECK_STATUS("initialize",
                 wuffs_json__decoder__initialize(
                     &dec, sizeof dec, WUFFS_VERSION,
                     WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

    wuffs_base__token_buffer tok = ((wuffs_base__token_buffer){
        .data = global_have_token_slice,
    });
    size_t n = strlen(test_cases[tc].str);
    wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
        .data = wuffs_base__make_slice_u8((void*)(test_cases[tc].str), n),
        .meta = wuffs_base__make_io_buffer_meta(n, 0, 0, true),
    });

    wuffs_base__status have_status =
        wuffs_json__decoder__decode_tokens(&dec, &tok, &src);

    uint64_t total_length = 0;
    size_t i;
    for (i = tok.meta.ri; i < tok.meta.wi; i++) {
      wuffs_base__token* t = &tok.data.ptr[i];
      total_length =
          wuffs_base__u64__sat_add(total_length, wuffs_base__token__length(t));
    }

    if (have_status.repr != test_cases[tc].want_status_repr) {
      RETURN_FAIL("%s: have \"%s\", want \"%s\"", test_cases[tc].str,
                  have_status.repr, test_cases[tc].want_status_repr);
    }

    if (total_length != src.meta.ri) {
      RETURN_FAIL("%s: total length: have %" PRIu64 ", want %" PRIu64,
                  test_cases[tc].str, total_length, src.meta.ri);
    }
  }

  return NULL;
}

  // ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

  // No mimic tests.

#endif  // WUFFS_MIMIC

// ---------------- JSON Benches

const char*  //
bench_wuffs_json_decode_1k() {
  CHECK_FOCUS(__func__);
  return do_bench_token_decoder(
      wuffs_json_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      tcounter_src, &json_github_tags_gt, UINT64_MAX, UINT64_MAX, 10000);
}

const char*  //
bench_wuffs_json_decode_21k_formatted() {
  CHECK_FOCUS(__func__);
  return do_bench_token_decoder(
      wuffs_json_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      tcounter_src, &json_file_sizes_gt, UINT64_MAX, UINT64_MAX, 300);
}

const char*  //
bench_wuffs_json_decode_26k_compact() {
  CHECK_FOCUS(__func__);
  return do_bench_token_decoder(
      wuffs_json_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      tcounter_src, &json_australian_abc_gt, UINT64_MAX, UINT64_MAX, 250);
}

const char*  //
bench_wuffs_json_decode_217k_stringy() {
  CHECK_FOCUS(__func__);
  return do_bench_token_decoder(
      wuffs_json_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      tcounter_src, &json_nobel_prizes_gt, UINT64_MAX, UINT64_MAX, 25);
}

  // ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

  // No mimic benches.

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    // These strconv tests are really testing the Wuffs base library. They
    // aren't specific to the std/json code, but putting them here is as good
    // as any other place.
    test_strconv_hpd_rounded_integer,  //
    test_strconv_hpd_shift,            //
    test_strconv_parse_number_f64,     //
    test_strconv_parse_number_i64,     //
    test_strconv_parse_number_u64,     //
    test_strconv_utf_8_next,           //

    test_wuffs_json_decode_interface,             //
    test_wuffs_json_decode_long_numbers,          //
    test_wuffs_json_decode_prior_valid_utf_8,     //
    test_wuffs_json_decode_src_io_buffer_length,  //
    test_wuffs_json_decode_string,                //
    test_wuffs_json_decode_unicode4_escapes,      //

#ifdef WUFFS_MIMIC

// No mimic tests.

#endif  // WUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_json_decode_1k,             //
    bench_wuffs_json_decode_21k_formatted,  //
    bench_wuffs_json_decode_26k_compact,    //
    bench_wuffs_json_decode_217k_stringy,   //

#ifdef WUFFS_MIMIC

// No mimic benches.

#endif  // WUFFS_MIMIC

    NULL,
};

int  //
main(int argc, char** argv) {
  proc_package_name = "std/json";
  return test_main(argc, argv, tests, benches);
}
