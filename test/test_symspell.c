/*
 * test_symspell.c - Test program for clean SymSpell implementation
 */

#include "symspell.h"
#include <stdio.h>
#include <string.h>

#define MAX_EDIT_DISTANCE 2
#define MAX_SUGGESTIONS 5
#define PREFIX_LENGTH 7

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <dictionary_file> [word expected word expected ...]\n", argv[0]);
        fprintf(stderr, "\nExamples:\n");
        fprintf(stderr, "  Interactive: %s dictionaries/dictionary.txt\n", argv[0]);
        fprintf(stderr, "  Batch test:  %s dictionaries/dictionary.txt helo hello recieve receive\n", argv[0]);
        return 1;
    }
    
    printf("Creating SymSpell dictionary...\n");
    symspell_dict_t* dict = symspell_create(MAX_EDIT_DISTANCE, PREFIX_LENGTH);
    if (!dict) {
        fprintf(stderr, "Failed to create dictionary\n");
        return 1;
    }
    
    printf("Loading dictionary from: %s\n", argv[1]);
    if (!symspell_load_dictionary(dict, argv[1], 0, 1)) {
        fprintf(stderr, "Failed to load dictionary\n");
        symspell_destroy(dict);
        return 1;
    }
    
    size_t word_count, entry_count;
    symspell_get_stats(dict, &word_count, &entry_count);
    printf("Loaded %zu words, %zu delete entries\n\n", word_count, entry_count);
    
    /* Batch test mode: pairs of (misspelled, expected) */
    if (argc > 2) {
        printf("=== Batch Test Mode ===\n");
        int tests = 0;
        int passed = 0;
        
        for (int i = 2; i < argc; i += 2) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Warning: Odd number of test arguments, ignoring '%s'\n", argv[i]);
                break;
            }
            
            const char* input = argv[i];
            const char* expected = argv[i + 1];
            
            symspell_suggestion_t suggestions[MAX_SUGGESTIONS];
            int count = symspell_lookup(dict, input, MAX_EDIT_DISTANCE, suggestions, MAX_SUGGESTIONS);
            
            tests++;
            
            if (count > 0 && strcmp(suggestions[0].term, expected) == 0) {
                printf("✓ \"%s\" -> \"%s\"\n", input, suggestions[0].term);
                passed++;
            } else {
                printf("✗ \"%s\" -> expected \"%s\", got ", input, expected);
                if (count > 0) {
                    printf("\"%s\"\n", suggestions[0].term);
                } else {
                    printf("no suggestions\n");
                }
            }
        }
        
        printf("\n=== Results ===\n");
        printf("Tests: %d/%d passed\n", passed, tests);
        
        symspell_destroy(dict);
        return (passed == tests) ? 0 : 1;
    }
    
    /* Interactive test mode */
    printf("=== Interactive Mode ===\n");
    printf("Enter words to correct (or 'quit' to exit):\n");
    char line[256];
    
    while (fgets(line, sizeof(line), stdin)) {
        /* Remove newline */
        line[strcspn(line, "\n")] = 0;
        
        if (strcmp(line, "quit") == 0) break;
        if (strlen(line) == 0) continue;
        
        symspell_suggestion_t suggestions[MAX_SUGGESTIONS];
        int count = symspell_lookup(dict, line, MAX_EDIT_DISTANCE, suggestions, MAX_SUGGESTIONS);
        
        if (count == 0) {
            printf("  No suggestions\n");
        } else {
            printf("  Suggestions:\n");
            for (int i = 0; i < count; i++) {
                printf("    %s (distance=%d, iwf=%f, prob=%f, freq=%llu)\n",
                    suggestions[i].term,
                    suggestions[i].distance,
                    suggestions[i].iwf,
                    suggestions[i].probability,
                    (unsigned long long)suggestions[i].frequency);
            }
        }
    }
    
    symspell_destroy(dict);
    return 0;
}
