#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "symspell.h"

void test_verbosity_modes(symspell_dict_t* dict) {
    printf("\n=== Testing Verbosity Modes ===\n\n");
    
    const char* test_words[] = {"helo", "teh", "speling", "recieve"};
    int num_tests = sizeof(test_words) / sizeof(test_words[0]);
    
    for (int i = 0; i < num_tests; i++) {
        printf("Input: '%s'\n", test_words[i]);
        printf("----------------------------------------\n");
        
        symspell_suggestion_t suggestions[10];
        int count;
        
        // Test TOP (auto-correct)
        count = symspell_lookup(dict, test_words[i], SYMSPELL_VERBOSITY_TOP, 2, suggestions, 10);
        printf("TOP (auto-correct):\n");
        if (count > 0) {
            printf("  → %s (distance=%d, freq=%llu)\n", 
                   suggestions[0].term, suggestions[0].distance, suggestions[0].frequency);
        } else {
            printf("  No suggestions\n");
        }
        
        // Test CLOSEST (UI alternatives)
        count = symspell_lookup(dict, test_words[i], SYMSPELL_VERBOSITY_CLOSEST, 2, suggestions, 10);
        printf("\nCLOSEST (UI alternatives):\n");
        if (count > 0) {
            for (int j = 0; j < count; j++) {
                printf("  %d. %s (distance=%d, freq=%llu)\n", 
                       j+1, suggestions[j].term, suggestions[j].distance, suggestions[j].frequency);
            }
        } else {
            printf("  No suggestions\n");
        }
        
        // Test ALL (comprehensive)
        count = symspell_lookup(dict, test_words[i], SYMSPELL_VERBOSITY_ALL, 2, suggestions, 10);
        printf("\nALL (comprehensive, max 10):\n");
        if (count > 0) {
            for (int j = 0; j < count; j++) {
                printf("  %d. %s (distance=%d, freq=%llu)\n", 
                       j+1, suggestions[j].term, suggestions[j].distance, suggestions[j].frequency);
            }
        } else {
            printf("  No suggestions\n");
        }
        
        printf("\n");
    }
}

void test_edge_cases(symspell_dict_t* dict) {
    printf("\n=== Testing Edge Cases ===\n\n");
    
    symspell_suggestion_t suggestions[10];
    int count;
    
    // Test 1: Correct spelling
    printf("1. Correct spelling (should return distance=0):\n");
    count = symspell_lookup(dict, "hello", SYMSPELL_VERBOSITY_TOP, 2, suggestions, 10);
    printf("   'hello' → ");
    if (count > 0 && suggestions[0].distance == 0) {
        printf("✓ Correct (distance=%d)\n", suggestions[0].distance);
    } else {
        printf("✗ FAILED\n");
    }
    
    // Test 2: Word not in dictionary
    printf("\n2. Word not in dictionary:\n");
    count = symspell_lookup(dict, "zxqwerty", SYMSPELL_VERBOSITY_ALL, 2, suggestions, 10);
    printf("   'zxqwerty' → %d suggestions (expected: 0 or very few)\n", count);
    
    // Test 3: Single character
    printf("\n3. Single character:\n");
    count = symspell_lookup(dict, "a", SYMSPELL_VERBOSITY_CLOSEST, 2, suggestions, 10);
    printf("   'a' → %d suggestions\n", count);
    
    // Test 4: Empty string (should fail gracefully)
    printf("\n4. Empty string (should return 0):\n");
    count = symspell_lookup(dict, "", SYMSPELL_VERBOSITY_TOP, 2, suggestions, 10);
    printf("   '' → %d suggestions %s\n", count, (count == 0) ? "✓" : "✗ FAILED");
    
    // Test 5: Very long word
    printf("\n5. Very long word:\n");
    count = symspell_lookup(dict, "antidisestablishmentarianism", SYMSPELL_VERBOSITY_TOP, 2, suggestions, 10);
    printf("   'antidisestablishmentarianism' → %d suggestions\n", count);
    if (count > 0) {
        printf("   → %s (distance=%d)\n", suggestions[0].term, suggestions[0].distance);
    }
}

void test_specific_cases(symspell_dict_t* dict) {
    printf("\n=== Testing Specific Cases from Reddit ===\n\n");
    
    symspell_suggestion_t suggestions[10];
    int count;
    
    // Test 1: "helo" should find "hello"
    printf("1. 'helo' should include 'hello':\n");
    count = symspell_lookup(dict, "helo", SYMSPELL_VERBOSITY_ALL, 2, suggestions, 10);
    bool found_hello = false;
    for (int i = 0; i < count; i++) {
        if (strcmp(suggestions[i].term, "hello") == 0) {
            found_hello = true;
            printf("   ✓ Found 'hello' at position %d (distance=%d, freq=%llu)\n",
                   i+1, suggestions[i].distance, suggestions[i].frequency);
            break;
        }
    }
    if (!found_hello) {
        printf("   ✗ FAILED: 'hello' not found\n");
    }
    
    // Test 2: "teh" should prefer "the" over "tea"
    printf("\n2. 'teh' should prefer 'the' (higher frequency):\n");
    count = symspell_lookup(dict, "teh", SYMSPELL_VERBOSITY_CLOSEST, 2, suggestions, 10);
    if (count > 0) {
        printf("   Top suggestion: '%s' (distance=%d, freq=%llu)\n",
               suggestions[0].term, suggestions[0].distance, suggestions[0].frequency);
        if (strcmp(suggestions[0].term, "the") == 0) {
            printf("   ✓ Correct\n");
        } else {
            printf("   ⚠ Got '%s' instead (may be correct by frequency)\n", suggestions[0].term);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <dictionary_file> [test_words...]\n", argv[0]);
        printf("\nExamples:\n");
        printf("  %s dictionaries/dictionary.txt\n", argv[0]);
        printf("  %s dictionaries/dictionary.txt helo speling\n", argv[0]);
        return 1;
    }
    
    printf("Creating SymSpell dictionary...\n");
    symspell_dict_t* dict = symspell_create(2, 7);
    
    printf("Loading dictionary from: %s\n", argv[1]);
    int result = symspell_load_dictionary(dict, argv[1], 0, 1);
    if (result < 0) {
        printf("Failed to load dictionary\n");
        symspell_destroy(dict);
        return 1;
    }
    
    if (argc == 2) {
        // Run all tests
        test_verbosity_modes(dict);
        test_edge_cases(dict);
        test_specific_cases(dict);
        
        printf("\n=== Interactive Mode ===\n");
        printf("Enter words to correct (or 'quit' to exit):\n\n");
        
        char input[256];
        while (1) {
            printf("> ");
            if (!fgets(input, sizeof(input), stdin)) break;
            
            // Remove newline
            input[strcspn(input, "\n")] = 0;
            
            if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
                break;
            }
            
            if (strlen(input) == 0) continue;
            
            symspell_suggestion_t suggestions[5];
            int count = symspell_lookup(dict, input, SYMSPELL_VERBOSITY_CLOSEST, 2, suggestions, 5);
            
            if (count > 0) {
                printf("  Suggestions:\n");
                for (int i = 0; i < count; i++) {
                    printf("    %d. %s (distance=%d, freq=%llu)\n",
                           i+1, suggestions[i].term, suggestions[i].distance, suggestions[i].frequency);
                }
            } else {
                printf("  No suggestions found\n");
            }
            printf("\n");
        }
    } else {
        // Batch test mode
        printf("\n=== Batch Test Mode ===\n");
        for (int i = 2; i < argc; i++) {
            symspell_suggestion_t suggestions[5];
            
            // Test with TOP verbosity (auto-correct)
            int count = symspell_lookup(dict, argv[i], SYMSPELL_VERBOSITY_TOP, 2, suggestions, 1);
            
            if (count > 0) {
                printf("'%s' → '%s' (distance=%d)\n", 
                       argv[i], suggestions[0].term, suggestions[0].distance);
            } else {
                printf("'%s' → No suggestions\n", argv[i]);
            }
        }
    }
    
    symspell_destroy(dict);
    return 0;
}