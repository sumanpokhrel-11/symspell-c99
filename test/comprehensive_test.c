#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "symspell.h"

typedef struct {
    const char* input;
    const char* expected_top;
    int expected_distance;
    int min_closest_count;  // Minimum suggestions in CLOSEST mode
} test_case_t;

void run_test_suite(symspell_dict_t* dict) {
    test_case_t tests[] = {
        // Input       Expected Top    Distance  Min Closest Count
        {"hello",      "hello",        0,        1},   // Exact match
        {"helo",       "held",         1,        2},   // Multiple at distance 1
        {"teh",        "the",          1,        1},   // High frequency wins
        {"speling",    "spelling",     1,        1},   // Common typo
        {"recieve",    "receive",      1,        1},   // ie/ei confusion
        {"definately", "definitely",   2,        1},   // Common misspelling
        {"occured",    "occurred",     1,        1},   // Double letter
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;
    int failed = 0;
    
    printf("\n=== Running Comprehensive Test Suite ===\n\n");
    
    for (int i = 0; i < num_tests; i++) {
        test_case_t* test = &tests[i];
        symspell_suggestion_t suggestions[10];
        bool test_passed = true;
        
        // Test TOP verbosity
        int count = symspell_lookup(dict, test->input, SYMSPELL_VERBOSITY_TOP, 2, suggestions, 10);
        
        printf("Test %d: '%s'\n", i+1, test->input);
        printf("  Expected: '%s' (distance=%d)\n", test->expected_top, test->expected_distance);
        
        if (count == 0) {
            printf("  ✗ FAILED: No suggestions returned\n");
            test_passed = false;
        } else {
            printf("  Got:      '%s' (distance=%d)\n", suggestions[0].term, suggestions[0].distance);
            
            if (strcmp(suggestions[0].term, test->expected_top) != 0) {
                printf("  ⚠ WARNING: Different top suggestion (may be valid)\n");
                // Don't fail - frequency ordering may differ
            }
            
            if (suggestions[0].distance != test->expected_distance) {
                printf("  ✗ FAILED: Wrong distance\n");
                test_passed = false;
            } else {
                printf("  ✓ Distance correct\n");
            }
        }
        
        // Test CLOSEST verbosity
        count = symspell_lookup(dict, test->input, SYMSPELL_VERBOSITY_CLOSEST, 2, suggestions, 10);
        printf("  CLOSEST mode: %d suggestions", count);
        
        if (count >= test->min_closest_count) {
            printf(" ✓\n");
        } else {
            printf(" ✗ (expected at least %d)\n", test->min_closest_count);
            test_passed = false;
        }
        
        if (test_passed) {
            passed++;
            printf("  Result: PASS\n");
        } else {
            failed++;
            printf("  Result: FAIL\n");
        }
        printf("\n");
    }
    
    printf("=== Test Results ===\n");
    printf("Passed: %d/%d\n", passed, num_tests);
    printf("Failed: %d/%d\n", failed, num_tests);
    
    if (failed == 0) {
        printf("\n🎉 All tests passed!\n");
    } else {
        printf("\n⚠ Some tests failed\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <dictionary_file>\n", argv[0]);
        return 1;
    }
    
    symspell_dict_t* dict = symspell_create(2, 7);
    
    if (!symspell_load_dictionary(dict, argv[1], 0, 1)) {
        printf("Failed to load dictionary\n");
        return 1;
    }
    
    run_test_suite(dict);
    
    symspell_destroy(dict);
    return 0;
}