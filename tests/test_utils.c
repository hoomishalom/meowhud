#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../include/utils.h"

void test_parse_uint32() {
    uint32_t val;
    
    // Valid cases
    assert(parse_uint32("0", &val, 10) == true && val == 0);
    assert(parse_uint32("4294967295", &val, 10) == true && val == 4294967295U); // UINT32_MAX
    
    // Hex parsing
    assert(parse_uint32("ff", &val, 16) == true && val == 255);
    
    // Invalid characters
    assert(parse_uint32("123abc", &val, 10) == false);
    assert(parse_uint32("abc", &val, 10) == false);
    assert(parse_uint32("", &val, 10) == false);
    
    // Out of bounds
    assert(parse_uint32("4294967296", &val, 10) == false); // UINT32_MAX + 1
    assert(parse_uint32("-1", &val, 10) == false);
    
    printf("test_parse_uint32 passed.\n");
}

void test_parse_int32() {
    int32_t val;
    
    // Valid cases
    assert(parse_int32("0", &val, 10) == true && val == 0);
    assert(parse_int32("2147483647", &val, 10) == true && val == 2147483647); // INT32_MAX
    assert(parse_int32("-2147483648", &val, 10) == true && val == -2147483648LL); // INT32_MIN
    
    // Invalid characters
    assert(parse_int32("123abc", &val, 10) == false);
    
    // Out of bounds
    assert(parse_int32("2147483648", &val, 10) == false); // INT32_MAX + 1
    assert(parse_int32("-2147483649", &val, 10) == false); // INT32_MIN - 1
    
    printf("test_parse_int32 passed.\n");
}

void test_parse_uint64() {
    uint64_t val;
    
    // Valid cases
    assert(parse_uint64("0", &val, 10) == true && val == 0);
    assert(parse_uint64("18446744073709551615", &val, 10) == true && val == 18446744073709551615ULL); // UINT64_MAX
    
    // Binary string parsing (used for anchors like "1101")
    assert(parse_uint64("1101", &val, 2) == true && val == 13);
    
    // Out of bounds
    assert(parse_uint64("18446744073709551616", &val, 10) == false); // UINT64_MAX + 1
    
    printf("test_parse_uint64 passed.\n");
}

int main() {
    printf("Running unit tests...\n");
    
    test_parse_uint32();
    test_parse_int32();
    test_parse_uint64();
    
    printf("All tests passed successfully!\n");
    return 0;
}
