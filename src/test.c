#include <stdio.h>
#include "symspell.h"

int main() {
    symspell_dict_t* dict = symspell_create(2, 7);
    symspell_load_dictionary(dict, "dictionaries/dictionary.txt", 0, 1);
    
    // Test if "hello" itself is recognized as correct
    printf("Testing 'hello' (correct spelling):\n");
    symspell_suggestion_t suggs[10];
    int count = symspell_lookup(dict, "hello", 2, suggs, 10);
    printf("Found %d suggestions:\n", count);
    for (int i = 0; i < count; i++) {
        printf("  %s (distance=%d, freq=%llu)\n", 
               suggs[i].term, suggs[i].distance, suggs[i].frequency);
    }
    
    printf("\nTesting 'helo' again:\n");
    count = symspell_lookup(dict, "helo", 2, suggs, 10);
    printf("Found %d suggestions:\n", count);
    for (int i = 0; i < count; i++) {
        printf("  %s (distance=%d, freq=%llu)\n", 
               suggs[i].term, suggs[i].distance, suggs[i].frequency);
    }
    
    symspell_destroy(dict);
    return 0;
}