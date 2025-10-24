/*
 * symspell.h - Clean SymSpell Implementation for CGIOS
 * 
 * Based on the Symmetric Delete spelling correction algorithm
 * Original algorithm by Wolf Garbe (MIT License)
 * https://github.com/wolfgarbe/SymSpell
 * 
 * This is a from-scratch implementation following the published algorithm,
 * optimized for CGIOS requirements (single-word correction only).
 * 
 * Copyright (c) 2025 CGIOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef SYMSPELL_H
#define SYMSPELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Maximum edit distance supported */
#define SYMSPELL_MAX_EDIT_DISTANCE 3

/* Suggestion structure */
typedef struct {
    char term[128];        /* Suggested word */
    int distance;          /* Edit distance from query */
    uint64_t frequency;    /* Word frequency */
    float probability;     /* Word probability */
    float iwf;             /* Inverse Word Frequency */
} symspell_suggestion_t;

/* SymSpell dictionary handle */
typedef struct symspell_dict symspell_dict_t;

/*
 * Create new SymSpell dictionary
 * 
 * max_edit_distance: Maximum edit distance (1 or 2)
 * prefix_length: Length of prefix to use for optimization (7 recommended)
 * 
 * Returns: Dictionary handle or NULL on error
 */
symspell_dict_t* symspell_create(int max_edit_distance, int prefix_length);

/*
 * Load dictionary from file
 * 
 * Format: word frequency (tab or space separated, one per line)
 * All words must be lowercase
 * 
 * dict: Dictionary handle
 * filepath: Path to dictionary file
 * term_index: Column index of term (0-based)
 * count_index: Column index of frequency (0-based)
 * 
 * Returns: true on success
 */
bool symspell_load_dictionary(
    symspell_dict_t* dict,
    const char* filepath,
    int term_index,
    int count_index
);

/*
 * Find spelling suggestions for a term
 * 
 * dict: Dictionary handle
 * term: Input term (will be lowercased internally)
 * max_edit_distance: Maximum edit distance to search
 * suggestions: Output array for suggestions
 * max_suggestions: Maximum number of suggestions to return
 * 
 * Returns: Number of suggestions found
 */
int symspell_lookup(
    const symspell_dict_t* dict,
    const char* term,
    int max_edit_distance,
    symspell_suggestion_t* suggestions,
    int max_suggestions
);

/*
 * Free dictionary
 */
void symspell_destroy(symspell_dict_t* dict);

/*
 * Get dictionary statistics
 */
void symspell_get_stats(
    const symspell_dict_t* dict,
    size_t* word_count,
    size_t* entry_count
);

/* 
 * Get word probability by hash (0.0 if not in dictionary)
 */
float symspell_get_probability(
    const symspell_dict_t* dict,
    uint64_t word_hash
);

float symspell_get_iwf(
    const symspell_dict_t* dict, 
    const char* word
);

#ifdef __cplusplus
}
#endif

#endif /* SYMSPELL_H */
