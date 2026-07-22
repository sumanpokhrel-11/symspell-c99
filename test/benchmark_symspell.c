/*
 * benchmark_symspell.c - Benchmark tool for the clean SymSpell implementation.
 *
 * Measures dictionary load time and average lookup performance against a
 * test file of misspellings.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "symspell.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define SYMSPELL_MAX_TERM_LENGTH 128
#define MAX_LINE_BUFFER 8192        
#define ITERATIONS 100
#define EDIT_DISTANCE 2
#define PREFIX_LENGTH 7

/* High-precision timing function */
static double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <dictionary_file> <test_file>\n", argv[0]);
        return 1;
    }

    /* --- 0. Check dictionary and test file exist --- */
    FILE* check_dict = fopen(argv[1], "r");
    if (!check_dict) {
        fprintf(stderr, "Error: Dictionary file not found: %s\n", argv[1]);
        return 1;
    }
    fclose(check_dict);

    FILE* check_test = fopen(argv[2], "r");
    if (!check_test) {
        fprintf(stderr, "Error: Test file not found: %s\n", argv[2]);
        return 1;
    }
    fclose(check_test);

    /* --- 1. Measure Dictionary Load Time --- */
    printf("Loading dictionary: %s\n", argv[1]);
    double start_load = get_time_ms();
    
    symspell_dict_t* dict = symspell_create(EDIT_DISTANCE, PREFIX_LENGTH);
    if (!dict || !symspell_load_dictionary(dict, argv[1], 0, 1)) {
        fprintf(stderr, "Failed to load dictionary\n");
        if (dict) symspell_destroy(dict);
        return 1;
    }
    
    double end_load = get_time_ms();
    double load_time_ms = end_load - start_load;
    
    size_t word_count, entry_count;
    symspell_get_stats(dict, &word_count, &entry_count);
    printf("Loaded %zu words and %zu deletes in %.2f ms\n\n", 
           word_count, entry_count, load_time_ms);

    /* --- 2. Measure Lookup Performance --- */
    FILE* fp = fopen(argv[2], "r");
    if (!fp) {
        fprintf(stderr, "Failed to open test file: %s\n", argv[2]);
        symspell_destroy(dict);
        return 1;
    }

    FILE* errors_fp = fopen("errors.txt", "w");
    if (!errors_fp) {
        fprintf(stderr, "Failed to open errors.txt for writing\n");
        fclose(fp);
        symspell_destroy(dict);
        return 1;
    }

    printf("Running benchmark against: %s\n", argv[2]);
    
    int total = 0, correct = 0;
    double total_lookup_time_ms = 0;
    char line[MAX_LINE_BUFFER];
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;

        char misspelled[SYMSPELL_MAX_TERM_LENGTH], expected[SYMSPELL_MAX_TERM_LENGTH];
        if (sscanf(line, "%127s\t%127s", misspelled, expected) != 2) continue;
        
        total++;
        
        symspell_suggestion_t suggestions[5];
        
        double start_lookup = get_time_ms();
        int count = symspell_lookup(dict, misspelled, 2, suggestions, 5);
        double end_lookup = get_time_ms();
        
        total_lookup_time_ms += (end_lookup - start_lookup);
        
        if (count > 0 && strcmp(suggestions[0].term, expected) == 0) {
            correct++;
        } else {
            /* Write error to errors.txt */
            const char* got = (count > 0) ? suggestions[0].term : "(none)";
            fprintf(errors_fp, "%s\t%s\t%s\n", expected, misspelled, got);
        }
        
        if (total % 100 == 0) {
            fprintf(stderr, "\rProcessed: %d...", total);
            fflush(stderr);
        }
    }
    fprintf(stderr, "\rProcessed: %d... Done.\n\n", total);
    
    fclose(fp);
    fclose(errors_fp);

    /* --- 3. Print Final Results --- */
    double avg_lookup_ms = total_lookup_time_ms / total;
    double avg_lookup_us = avg_lookup_ms * 1000.0;

    printf("--- Accuracy Results ---\n");
    printf("Total test cases: %d\n", total);
    printf("Correctly solved: %d (%.1f%%)\n", correct, 100.0 * correct / total);
    printf("Wrong: %d (%.1f%%)\n\n", total - correct, 100.0 * (total - correct) / total);
    
    printf("--- Performance Results ---\n");
    printf("Dictionary load time: %.2f ms\n", load_time_ms);
    printf("Total lookup time:    %.2f ms (for %d lookups)\n", total_lookup_time_ms, total);
    printf("Average lookup time:  %.3f ms (%.1f µs)\n", avg_lookup_ms, avg_lookup_us);
    
    printf("\nError cases written to errors.txt\n");
    
    symspell_destroy(dict);
    return 0;
}
