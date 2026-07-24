/*
 * symspell.c - A clean, self-contained SymSpell implementation for CGIOS
 *
 * This file provides a pure C99 implementation of the Symmetric Delete spelling
 * correction algorithm, originally created by Wolf Garbe. It is designed to be
 * fast, memory-efficient, and have zero external dependencies beyond the C
 * standard library.
 *
 * CONSTITUTIONAL PRINCIPLES:
 * - No Magic Numbers: All constants are defined with clear names.
 * - Robustness: All allocations are checked, and buffer sizes are enforced.
 * - Clarity: The code is commented to explain the "why" behind the algorithms.
 * - Purity: Operates entirely on memory buffers with no side effects.
 *
 * PUBLIC API:
 * - symspell_dict_t* symspell_create(...)
 * - void symspell_destroy(...)
 * - bool symspell_load_dictionary(...)
 * - int symspell_lookup(...)
 * - void symspell_get_stats(...)
 *
 * Copyright (c) 2025 CGIOS Project
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include "posix.h"
#include "xxh3.h"
#include "symspell.h"
#include "hash.h"

/*
 * define DO_SORT 1
 * gcc -DDO_SORT ...
 *
 * The DO_SORT define sorts the candidates by edit_distance, frequency, then alphabetical
 * This makes candidate[0] the top ranking candidate.
 * While this is the default behaviour of SymSpell implementations, unless selected with the
 * DO_SORT define we make a single pass through the candidates and return the single best choice.
 * Note the once built the sysmspell.o will retain this behaviour.
 */

/* --- Constants --- */
#define SYMSPELL_MAX_TERM_LENGTH 128
#define INITIAL_ENTRY_CAPACITY 4
#define DELETE_QUEUE_CAPACITY 10000
#define MAX_LINE_BUFFER 512
#define MAX_PARTS_PER_LINE 10
#define MAX_CANDIDATES_PER_LOOKUP 10000
#define HASH_TABLE_LOAD_WARNING_THRESHOLD 0.75

#define STRING_ARENA_SIZE (128 * 1024 * 1024) // 128MB arena for strings
#define ENTRY_ARENA_SIZE (128 * 1024 * 1024)  // 128MB arena for entry structs

/* Pre-calculated prime numbers for hash table sizes.
 * Chosen to keep load factor < 50% for the 82k-word English dictionary
 * at the given edit distance, ensuring high performance.
 * d=1: ~200k deletes -> 524,287 is sufficient
 * d=2: ~1.8M deletes -> 4,194,301 is required
 * d=3: ~15M deletes -> 33,554,393 is required (estimated)
 */
#define TABLE_SIZE_D1 524287
#define TABLE_SIZE_D2 4194301
#define TABLE_SIZE_D3 33554393

/* Exact match table size - ~500k slots for up to 250k words at 50% load */
#define EXACT_MATCH_TABLE_SIZE 524287

/* A simple memory arena for fast allocation */
typedef struct {
    char* memory;
    size_t capacity;
    size_t used;
} arena_t;

/* Hash table entry for delete -> words mapping */
typedef struct delete_entry {
    const char* delete_str;     /* Points into the string_arena */
    const char** words;         /* Points into the string_arena */
    uint64_t* frequencies;      /* Frequency of each word */
    size_t count;               /* Number of words */
    size_t capacity;            /* Allocated capacity */
} delete_entry_t;

/* Fast exact-match lookup table using 64-bit hashes */
typedef struct {
    uint64_t* hashes;       /* 64-bit word hashes */
    uint64_t* frequencies;  /* Word frequencies */
    float* probabilities;   /* Probabilities */
    float* iwf;             /* Inverse Word Frequency */
    size_t table_size;
} exact_match_table_t;

/* --- Reusable lookup scratch types (added) ------------------------------------------- */
typedef struct {
    char str[SYMSPELL_MAX_TERM_LENGTH];
    int  distance;
} queue_item_t;

/* Open-addressed set keyed on a 64-bit hash, cleared in O(1) via a generation stamp.
 * Replaces the per-lookup ht_create()/ht_destroy() churn. Collision-safe: on a hash
 * match the stored string pointer is strcmp-verified. */
typedef struct {
    uint64_t*    key;
    const char** str;
    uint32_t*    stamp;
    size_t       cap;   /* power of two */
    size_t       mask;
    uint32_t     gen;
} genset_t;

static bool genset_init(genset_t* set, size_t cap_pow2) {
    set->cap = cap_pow2; set->mask = cap_pow2 - 1; set->gen = 0;
    set->key   = malloc(cap_pow2 * sizeof(uint64_t));
    set->str   = malloc(cap_pow2 * sizeof(const char*));
    set->stamp = calloc(cap_pow2, sizeof(uint32_t));
    return set->key && set->str && set->stamp;
}
static inline void genset_reset(genset_t* set) {
    if (++set->gen == 0) { memset(set->stamp, 0, set->cap * sizeof(uint32_t)); set->gen = 1; }
}
/* Returns true if (h,str) already present this generation; else inserts and returns false. */
static inline bool genset_seen_or_add(genset_t* set, uint64_t h, const char* str) {
    size_t idx = (size_t)h & set->mask;
    for (;;) {
        if (set->stamp[idx] != set->gen) {
            set->stamp[idx] = set->gen; set->key[idx] = h; set->str[idx] = str;
            return false;
        }
        if (set->key[idx] == h && strcmp(set->str[idx], str) == 0) return true;
        idx = (idx + 1) & set->mask;
    }
}
static void genset_free(genset_t* set) { free(set->key); free(set->str); free(set->stamp); }

/* SymSpell dictionary structure with pre-allocated work buffers */
struct symspell_dict {
    delete_entry_t** table;           /* Hash table for deletes */
    exact_match_table_t* exact_table; /* Fast O(1) exact match */
    size_t table_size;                /* Hash table size */
    int max_edit_distance;            /* Max distance */
    int prefix_length;                /* Prefix optimization */
    size_t word_count;                /* Total unique words */
    size_t entry_count;               /* Total delete entries */

    pthread_mutex_t lookup_mutex;     /* Mutex for thread safety */

    /* Arenas for fast, contiguous allocation during load */
    arena_t string_arena;
    arena_t entry_arena;
    
    /* Reusable work buffers for lookup path */
    char** delete_work_buffer;
    size_t delete_buffer_capacity;
    symspell_suggestion_t* candidate_buffer;

    /* reusable lookup scratch (added) */
    queue_item_t* delete_queue;
    char*         delete_pool;
    genset_t      delete_seen;
    genset_t      cand_seen;    
};

/* --- Arena Allocator Functions --- */

/* Allocate from arena, ensuring 8-byte alignment */
static void* arena_alloc(arena_t* arena, size_t size) {
    size_t align_mask = 7;
    size_t aligned_used = (arena->used + align_mask) & ~align_mask;
    if (aligned_used + size > arena->capacity) {
        fprintf(stderr, "\n--- MEMORY ARENA FULL ---\n");
        fprintf(stderr, "Error: Failed to allocate %zu bytes (%.2f MB).\n", 
                size, (double)size / 1024 / 1024);
        fprintf(stderr, "Arena Capacity: %zu bytes (%.2f MB)\n", 
                arena->capacity, (double)arena->capacity / 1024 / 1024);
        fprintf(stderr, "Arena Used:     %zu bytes (%.2f MB)\n", 
                arena->used, (double)arena->used / 1024 / 1024);
        fprintf(stderr, "Next Alloc:   %zu bytes\n", aligned_used + size);
        fprintf(stderr, "\nAction: Increase arena size (e.g., STRING_ARENA_SIZE) in symspell.c.\n");
        exit(1);
        return NULL;
    }
    void* ptr = arena->memory + aligned_used;
    arena->used = aligned_used + size;
    return ptr;
}

/* Fast calloc replacement (memory is already zeroed from main calloc) */
static void* arena_calloc(arena_t* arena, size_t num, size_t size) {
    return arena_alloc(arena, num * size);
}

/* Fast strdup replacement */
static const char* arena_strdup(arena_t* arena, const char* s) {
    size_t len = strlen(s) + 1;
    char* new_str = arena_alloc(arena, len);
    if (!new_str) return NULL;
    memcpy(new_str, s, len);
    return new_str;
}

/* str_dup removed: delete generation now writes into the reusable delete_pool */

/* Convert string to lowercase in-place */
static void str_tolower(char* str) {
    for (; *str; str++) {
        *str = tolower((unsigned char)*str);
    }
}

/* Calculate IWF from probability */
float calculate_iwf(const float probability) {
    if (probability > 0.0f) {
        return fabsf(-logf(probability));
    } else {
        return 99.0f;
    }
}

/* delete_in_suggestion_prefix - is `del` a subsequence of the first prefix_length chars of
 * `sug`? Faithful port of SymSpell.DeleteInSuggestionPrefix. */
static bool delete_in_suggestion_prefix(const char* del, int del_len,
                                        const char* sug, int sug_len, int prefix_length) {
    if (del_len == 0) return true;
    if (prefix_length < sug_len) sug_len = prefix_length;
    int j = 0;
    for (int i = 0; i < del_len; i++) {
        char dch = del[i];
        while (j < sug_len && dch != sug[j]) j++;
        if (j++ == sug_len) return false;
    }
    return true;
}

/* damerau_osa - Damerau-Levenshtein optimal string alignment distance, bounded by max_distance.
 * Faithful C port of SoftWx.Match DamerauOSA.Distance(s1,s2,maxDistance): shorter string first,
 * common prefix/suffix trimmed, then a banded (or, when it fits, full) single-vector DP.
 * Returns the distance, or max_distance+1 if it exceeds max_distance. */
static int damerau_osa(const char* s1, int l1, const char* s2, int l2, int max_distance) {
    if (l1 > l2) { const char* ts = s1; s1 = s2; s2 = ts; int tl = l1; l1 = l2; l2 = tl; }
    if (l2 - l1 > max_distance) return max_distance + 1;

    int len1 = l1, len2 = l2;
    while (len1 != 0 && s1[len1 - 1] == s2[len2 - 1]) { len1--; len2--; }
    int start = 0;
    while (start != len1 && s1[start] == s2[start]) start++;
    if (start != 0) { len1 -= start; len2 -= start; }
    if (len1 == 0) return (len2 <= max_distance) ? len2 : (max_distance + 1);

    int c1[SYMSPELL_MAX_TERM_LENGTH];
    int pc1[SYMSPELL_MAX_TERM_LENGTH];
    memset(pc1, 0, (size_t)len2 * sizeof(int));
    int i, j, currentCost = 0;

    if (max_distance < len2) {
        for (j = 0; j < max_distance; j++) c1[j] = j + 1;
        for (; j < len2;) c1[j++] = max_distance + 1;
        int lenDiff = len2 - len1;
        int jStartOffset = max_distance - lenDiff;
        int jStart = 0, jEnd = max_distance;
        char char1 = ' ';
        for (i = 0; i < len1; ++i) {
            char prevChar1 = char1;
            char1 = s1[start + i];
            char char2 = ' ';
            int leftCharCost = i, aboveCharCost = i;
            int nextTransCost = 0;
            jStart += (i > jStartOffset) ? 1 : 0;
            jEnd += (jEnd < len2) ? 1 : 0;
            for (j = jStart; j < jEnd; ++j) {
                int thisTransCost = nextTransCost;
                nextTransCost = pc1[j];
                pc1[j] = currentCost = leftCharCost;
                leftCharCost = c1[j];
                char prevChar2 = char2;
                char2 = s2[start + j];
                if (char1 != char2) {
                    if (aboveCharCost < currentCost) currentCost = aboveCharCost;
                    if (leftCharCost < currentCost) currentCost = leftCharCost;
                    ++currentCost;
                    if ((i != 0) && (j != 0)
                        && (char1 == prevChar2) && (prevChar1 == char2)
                        && (thisTransCost + 1 < currentCost)) {
                        currentCost = thisTransCost + 1;
                    }
                }
                c1[j] = aboveCharCost = currentCost;
            }
            if (c1[i + lenDiff] > max_distance) return max_distance + 1;
        }
        return (currentCost <= max_distance) ? currentCost : (max_distance + 1);
    } else {
        for (j = 0; j < len2; j++) c1[j] = j + 1;
        char char1 = ' ';
        for (i = 0; i < len1; ++i) {
            char prevChar1 = char1;
            char1 = s1[start + i];
            char char2 = ' ';
            int leftCharCost = i, aboveCharCost = i;
            int nextTransCost = 0;
            for (j = 0; j < len2; ++j) {
                int thisTransCost = nextTransCost;
                nextTransCost = pc1[j];
                pc1[j] = currentCost = leftCharCost;
                leftCharCost = c1[j];
                char prevChar2 = char2;
                char2 = s2[start + j];
                if (char1 != char2) {
                    if (aboveCharCost < currentCost) currentCost = aboveCharCost;
                    if (leftCharCost < currentCost) currentCost = leftCharCost;
                    ++currentCost;
                    if ((i != 0) && (j != 0)
                        && (char1 == prevChar2) && (prevChar1 == char2)
                        && (thisTransCost + 1 < currentCost)) {
                        currentCost = thisTransCost + 1;
                    }
                }
                c1[j] = aboveCharCost = currentCost;
            }
        }
        return currentCost;
    }
}
/*
 * Internal shared function to generate all unique deletes for a term
 * Uses caller-provided buffer to avoid malloc/free in hot paths
 * Now uses portable hash table instead of hsearch_r
 */
static size_t _generate_all_deletes_reuse(
    symspell_dict_t* dict,
    const char* word,
    int max_distance,
    int prefix_length,
    char** deletes_out,
    size_t max_deletes
) {
    int word_len = strlen(word);
    if (word_len == 0) return 0;

    char prefix[SYMSPELL_MAX_TERM_LENGTH];
    snprintf(prefix, sizeof(prefix), "%.*s",
             (word_len > prefix_length) ? prefix_length : word_len, word);
    int prefix_len = strlen(prefix);

    queue_item_t* queue = dict->delete_queue;   /* reused, no per-lookup malloc */
    genset_t* seen = &dict->delete_seen;
    genset_reset(seen);                          /* O(1) clear, no ht_create/destroy */

    size_t delete_count = 0;

    /* Empty string (only if within max_distance of the prefix). */
    if (prefix_len <= max_distance && delete_count < max_deletes) {
        char* slot = deletes_out[delete_count];
        slot[0] = '\0';
        if (!genset_seen_or_add(seen, xxh3("", 0), slot)) delete_count++;
    }

    /* The prefix itself. */
    if (delete_count < max_deletes) {
        char* slot = deletes_out[delete_count];
        memcpy(slot, prefix, (size_t)prefix_len + 1);
        if (!genset_seen_or_add(seen, xxh3(slot, (size_t)prefix_len), slot)) delete_count++;
    }

    /* BFS over deletions of the prefix, capped at max_distance, deduped via the stamped set.
     * Each unique delete is written straight into the caller's pooled slot (no str_dup) and
     * enqueued once for the next level. */
    int queue_start = 0, queue_end = 0;
    memcpy(queue[queue_end].str, prefix, (size_t)prefix_len + 1);
    queue[queue_end].distance = 0;
    queue_end++;

    while (queue_start < queue_end && delete_count < max_deletes) {
        queue_item_t current = queue[queue_start++];
        int cur_len = (int)strlen(current.str);
        if (current.distance >= max_distance || cur_len <= 1) continue;

        for (int i = 0; i < cur_len && delete_count < max_deletes; i++) {
            char deleted[SYMSPELL_MAX_TERM_LENGTH];
            int k = 0;
            for (int j = 0; j < cur_len; j++) {
                if (j != i) deleted[k++] = current.str[j];
            }
            deleted[k] = '\0';

            uint64_t h = xxh3(deleted, (size_t)k);
            char* slot = deletes_out[delete_count];
            memcpy(slot, deleted, (size_t)k + 1);
            if (!genset_seen_or_add(seen, h, slot)) {
                delete_count++;                              /* newly generated delete */
                if (queue_end < DELETE_QUEUE_CAPACITY) {     /* schedule next level */
                    memcpy(queue[queue_end].str, deleted, (size_t)k + 1);
                    queue[queue_end].distance = current.distance + 1;
                    queue_end++;
                }
            }
            /* already seen: pooled slot is overwritten on the next iteration */
        }
    }
    return delete_count;
}
/* Add word to delete entry */
static bool add_to_entry(symspell_dict_t* dict, delete_entry_t* entry, const char* word, uint64_t freq) {
    for (size_t i = 0; i < entry->count; i++) {
        if (strcmp(entry->words[i], word) == 0) {
            if (freq > entry->frequencies[i]) entry->frequencies[i] = freq;
            return true;
        }
    }
    
    if (entry->count >= entry->capacity) {
        size_t new_cap = entry->capacity == 0 ? INITIAL_ENTRY_CAPACITY : entry->capacity * 2;
        const char** new_words = realloc(entry->words, new_cap * sizeof(const char*));
        if (!new_words) return false;
        entry->words = new_words;
        
        uint64_t* new_freqs = realloc(entry->frequencies, new_cap * sizeof(uint64_t));
        if (!new_freqs) return false;
        entry->frequencies = new_freqs;
        entry->capacity = new_cap;
    }
    
    entry->words[entry->count] = arena_strdup(&dict->string_arena, word);
    if (!entry->words[entry->count]) return false;
    
    entry->frequencies[entry->count] = freq;
    entry->count++;
    return true;
}

/* Add word to exact match table during dictionary load */
static bool add_exact_match(symspell_dict_t* dict, const char* word, uint64_t freq) {
    uint64_t word_hash = xxh3(word, strlen(word));
    size_t idx = word_hash % dict->exact_table->table_size;
    
    for (size_t probe = 0; probe < dict->exact_table->table_size; probe++) {
        size_t pos = (idx + probe) % dict->exact_table->table_size;
        
        if (dict->exact_table->hashes[pos] == 0) {
            dict->exact_table->hashes[pos] = word_hash;
            dict->exact_table->frequencies[pos] = freq;
            return true;
        }
        
        if (dict->exact_table->hashes[pos] == word_hash) {
            if (freq > dict->exact_table->frequencies[pos]) {
                dict->exact_table->frequencies[pos] = freq;
            }
            return true;
        }
    }
    
    return false;
}

/* Add delete variant to hash table */
static bool add_delete(symspell_dict_t* dict, const char* delete_str,
                       const char* word, uint64_t freq) {

    uint64_t hash = xxh3(delete_str, strlen(delete_str));

    for (size_t i = 0; i < dict->table_size; i++) {
        size_t idx = (hash + i) % dict->table_size;

        if (dict->table[idx] == NULL) {
            delete_entry_t* entry = arena_calloc(&dict->entry_arena, 1, sizeof(delete_entry_t));
            if (!entry) return false;

            entry->delete_str = arena_strdup(&dict->string_arena, delete_str);
            if (!entry->delete_str) return false;

            if (!add_to_entry(dict, entry, word, freq)) {
                return false;
            }
            dict->table[idx] = entry;
            dict->entry_count++;
            return true;
        }

        if (strcmp(dict->table[idx]->delete_str, delete_str) == 0) {
            return add_to_entry(dict, dict->table[idx], word, freq);
        }
    }
    return false;
}

/* Generate all deletes for a word and add to dictionary */
static bool generate_deletes(symspell_dict_t* dict, const char* word, uint64_t freq) {
    size_t delete_count = _generate_all_deletes_reuse(
        dict, word, dict->max_edit_distance, dict->prefix_length,
        dict->delete_work_buffer, dict->delete_buffer_capacity
    );

    for (size_t i = 0; i < delete_count; i++) {
        add_delete(dict, dict->delete_work_buffer[i], word, freq);
    }
    return true;
}

/* 
 * symspell_create function.
 */
symspell_dict_t* symspell_create(int max_edit_distance, int prefix_length) {
    if (max_edit_distance < 1 || max_edit_distance > SYMSPELL_MAX_EDIT_DISTANCE) {
        fprintf(stderr, "Error: max_edit_distance must be between 1 and %d\n", SYMSPELL_MAX_EDIT_DISTANCE);
        return NULL;
    }
    
    symspell_dict_t* dict = calloc(1, sizeof(symspell_dict_t));
    if (!dict) {
        perror("symspell_create failed: calloc dict");
        return NULL;
    }

    if (pthread_mutex_init(&dict->lookup_mutex, NULL) != 0) {
        perror("symspell_create failed: pthread_mutex_init");
        symspell_destroy(dict);
        return NULL;
    }
    
    dict->max_edit_distance = max_edit_distance;
    dict->prefix_length = prefix_length;
    
    if (max_edit_distance == 1) {
        dict->table_size = TABLE_SIZE_D1;
    } else if (max_edit_distance == 2) {
        dict->table_size = TABLE_SIZE_D2;
    } else {
        dict->table_size = TABLE_SIZE_D3;
    }
    
    dict->table = calloc(dict->table_size, sizeof(delete_entry_t*));
    if (!dict->table) {
        perror("symspell_create failed: calloc dict->table");
        symspell_destroy(dict);
        return NULL;
    }
    
    dict->exact_table = calloc(1, sizeof(exact_match_table_t));
    if (!dict->exact_table) {
        perror("symspell_create failed: calloc dict->exact_table");
        symspell_destroy(dict);
        return NULL;
    }
    
    dict->exact_table->table_size = EXACT_MATCH_TABLE_SIZE;

    dict->exact_table->hashes = calloc(dict->exact_table->table_size, sizeof(uint64_t));
    if (!dict->exact_table->hashes) {
        perror("symspell_create failed: calloc dict->exact_table->hashes");
        symspell_destroy(dict);
        return NULL;
    }
    
    dict->exact_table->frequencies = calloc(dict->exact_table->table_size, sizeof(uint64_t));
    if (!dict->exact_table->frequencies) {
        perror("symspell_create failed: calloc dict->exact_table->frequencies");
        symspell_destroy(dict);
        return NULL;
    }

    dict->exact_table->probabilities = calloc(dict->exact_table->table_size, sizeof(float));
    if (!dict->exact_table->probabilities) {
        perror("symspell_create failed: calloc dict->exact_table->probabilities");
        symspell_destroy(dict);
        return NULL;
    }

    dict->exact_table->iwf = calloc(dict->exact_table->table_size, sizeof(float));
    if (!dict->exact_table->iwf) {
        perror("symspell_create failed: calloc dict->exact_table->iwf");
        symspell_destroy(dict);
        return NULL;
    }
    
    dict->delete_buffer_capacity = DELETE_QUEUE_CAPACITY;
    dict->delete_work_buffer = calloc(dict->delete_buffer_capacity, sizeof(char*));
    if (!dict->delete_work_buffer) {
        perror("symspell_create failed: calloc dict->delete_work_buffer");
        symspell_destroy(dict);
        return NULL;
    }
    
    dict->candidate_buffer = malloc(MAX_CANDIDATES_PER_LOOKUP * sizeof(symspell_suggestion_t));
    if (!dict->candidate_buffer) {
        perror("symspell_create failed: malloc dict->candidate_buffer");
        symspell_destroy(dict);
        return NULL;
    }

    /* Reusable lookup scratch (added): eliminates per-lookup malloc / ht_create / str_dup. */
    dict->delete_queue = malloc((size_t)DELETE_QUEUE_CAPACITY * sizeof(queue_item_t));
    dict->delete_pool  = malloc((size_t)dict->delete_buffer_capacity * SYMSPELL_MAX_TERM_LENGTH);
    if (!dict->delete_queue || !dict->delete_pool) {
        perror("symspell_create failed: malloc lookup scratch");
        symspell_destroy(dict);
        return NULL;
    }
    for (size_t i = 0; i < dict->delete_buffer_capacity; i++) {
        dict->delete_work_buffer[i] = dict->delete_pool + i * SYMSPELL_MAX_TERM_LENGTH;
    }
    if (!genset_init(&dict->delete_seen, 32768) || !genset_init(&dict->cand_seen, 32768)) {
        perror("symspell_create failed: genset_init");
        symspell_destroy(dict);
        return NULL;
    }

    dict->string_arena.capacity = STRING_ARENA_SIZE;
    dict->string_arena.memory = calloc(1, dict->string_arena.capacity);
    if (!dict->string_arena.memory) {
        perror("symspell_create failed: calloc dict->string_arena.memory");
        symspell_destroy(dict);
        return NULL;
    }

    dict->entry_arena.capacity = ENTRY_ARENA_SIZE;
    dict->entry_arena.memory = calloc(1, dict->entry_arena.capacity);
    if (!dict->entry_arena.memory) {
        perror("symspell_create failed: calloc dict->entry_arena.memory");
        symspell_destroy(dict);
        return NULL;
    }
    
    return dict;
}

/* Load dictionary from file */
bool symspell_load_dictionary(
    symspell_dict_t* dict, const char* filepath, int term_index, int count_index
) {
    printf("Entering load dictionary with filepath %s\n", filepath);

    if (!dict || !filepath) return false;
    
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        printf("Error opening file: %s\n", strerror(errno));
        return false;
    }
    
    char line[MAX_LINE_BUFFER];
    size_t line_num = 0;
    uint64_t total_words = 0;
    uint64_t max_freq = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) == 0) continue;
        
        char* parts[MAX_PARTS_PER_LINE];
        int part_count = 0;
        char* token = strtok(line, " \t");
        
        while (token && part_count < MAX_PARTS_PER_LINE) {
            parts[part_count++] = token;
            token = strtok(NULL, " \t");
        }
        
        if (part_count <= term_index || part_count <= count_index) continue;
        
        char* term = parts[term_index];
        uint64_t freq = strtoull(parts[count_index], NULL, 10);
        if (freq == 0) freq = 1;

        total_words += freq;
        if (!max_freq) {
            max_freq = freq;
        }
        
        str_tolower(term);
        
        add_exact_match(dict, term, freq);
        generate_deletes(dict, term, freq);
        dict->word_count++;
        
        if (line_num % 1000 == 0) {
            double load_factor = (double)dict->entry_count / dict->table_size;
            fprintf(stderr, "\rLoaded %zu words, %zu deletes (%.1f%% full)...", 
                    line_num, dict->entry_count, load_factor * 100);
            fflush(stderr);
            
            if (load_factor > HASH_TABLE_LOAD_WARNING_THRESHOLD) {
                fprintf(stderr, "\nWARNING: Hash table %.1f%% full\n", load_factor * 100);
            }
        }
    }

    fprintf(stderr, "\nCalculating probabilities (total words: %llu)...\n", 
            (unsigned long long)total_words);

    for (size_t i = 0; i < dict->exact_table->table_size; i++) {
        if (dict->exact_table->hashes[i] != 0) {
            float probability = (float)dict->exact_table->frequencies[i] / (float)max_freq;
            dict->exact_table->probabilities[i] = probability;
            dict->exact_table->iwf[i] = calculate_iwf(probability);
        }
    }

    fprintf(stderr, "\rLoaded %zu words, %zu deletes\n", 
            dict->word_count, dict->entry_count);

    fclose(fp);
    return true;
}

#ifdef DO_SORT
/* Comparison function for sorting suggestions */
static int compare_suggestions(const void* a, const void* b) {
    const symspell_suggestion_t* sa = a;
    const symspell_suggestion_t* sb = b;
    if (sa->distance != sb->distance) return sa->distance - sb->distance;
    if (sa->frequency != sb->frequency) return (sa->frequency > sb->frequency) ? -1 : 1;
    return strcmp(sa->term, sb->term);
}
#endif

/* Lookup suggestions */
int symspell_lookup(
    const symspell_dict_t* dict, const char* term, int max_edit_distance_lookup,
    symspell_suggestion_t* suggestions, int max_suggestions
) {
    if (!dict || !term || !suggestions || max_suggestions <= 0) return 0;
    {
        /* Reject empty/whitespace-only input. Edit distance from a short or empty
         * string to any word is just that word's length (or close to it), so a
         * blank query would otherwise "match" every dictionary word up to
         * max_edit_distance characters long -- meaningless for spell-checking. */
        const char* p = term;
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) return 0;
    }

    pthread_mutex_lock((pthread_mutex_t*)&dict->lookup_mutex);
    
    char query[SYMSPELL_MAX_TERM_LENGTH];
    strncpy(query, term, sizeof(query) - 1);
    query[sizeof(query) - 1] = '\0';
    str_tolower(query);
    
    /* FAST PATH: O(1) exact match via hash comparison */
    uint64_t query_hash = xxh3(query, strlen(query));
    size_t idx = query_hash % dict->exact_table->table_size;
    
    for (size_t probe = 0; probe < dict->exact_table->table_size; probe++) {
        size_t pos = (idx + probe) % dict->exact_table->table_size;
        
        if (dict->exact_table->hashes[pos] == 0) break;
        
        if (dict->exact_table->hashes[pos] == query_hash) {
            suggestions[0].distance = 0;
            suggestions[0].frequency = dict->exact_table->frequencies[pos];
            suggestions[0].probability = dict->exact_table->probabilities[pos];
            suggestions[0].iwf = dict->exact_table->iwf[pos];
            strncpy(suggestions[0].term, query, SYMSPELL_MAX_TERM_LENGTH - 1);
            suggestions[0].term[SYMSPELL_MAX_TERM_LENGTH - 1] = '\0';
            pthread_mutex_unlock((pthread_mutex_t*)&dict->lookup_mutex);
            return 1;
        }
    }
    
    /* SLOW PATH: Not found - do full SymSpell search */
    int max_edit_distance = (max_edit_distance_lookup < dict->max_edit_distance) 
                            ? max_edit_distance_lookup : dict->max_edit_distance;
    
    
    /* ---- Faithful port of SymSpell (C#) Lookup, Verbosity.Closest ---- */
    symspell_suggestion_t* candidates = dict->candidate_buffer;   /* result set (smallest distance) */
    int candidate_count = 0;

    int input_len = (int)strlen(query);
    int max2 = max_edit_distance;                    /* shrinks as closer suggestions are found */
    int input_prefix_len = input_len;

    char** cand_list = dict->delete_work_buffer;     /* pooled slots reused as the delete-candidate list */
    int cand_list_count = 0;

    genset_reset((genset_t*)&dict->delete_seen);     /* hashset1: deletes already generated */
    genset_reset((genset_t*)&dict->cand_seen);       /* hashset2: suggestions already considered */
    genset_seen_or_add((genset_t*)&dict->cand_seen, xxh3(query, (size_t)input_len), query);

    if (input_prefix_len > dict->prefix_length) {
        input_prefix_len = dict->prefix_length;
        memcpy(cand_list[0], query, (size_t)input_prefix_len);
        cand_list[0][input_prefix_len] = '\0';
    } else {
        memcpy(cand_list[0], query, (size_t)input_len + 1);
    }
    cand_list_count = 1;

    for (int cptr = 0; cptr < cand_list_count; cptr++) {
        const char* candidate = cand_list[cptr];
        int candidate_len = (int)strlen(candidate);
        int length_diff = input_prefix_len - candidate_len;

        if (length_diff > max2) break;               /* candidates ordered by delete distance */

        uint64_t chash = xxh3(candidate, (size_t)candidate_len);
        for (size_t probe = 0; probe < dict->table_size; probe++) {
            size_t idx = (chash + probe) % dict->table_size;
            if (!dict->table[idx]) break;
            if (strcmp(dict->table[idx]->delete_str, candidate) != 0) continue;

            delete_entry_t* entry = dict->table[idx];
            for (size_t si = 0; si < entry->count; si++) {
                const char* suggestion = entry->words[si];
                int suggestion_len = (int)strlen(suggestion);
                if (strcmp(suggestion, query) == 0) continue;
                if ((abs(suggestion_len - input_len) > max2)
                    || (suggestion_len < candidate_len)
                    || (suggestion_len == candidate_len && strcmp(suggestion, candidate) != 0))
                    continue;
                int sugg_prefix_len = (suggestion_len < dict->prefix_length) ? suggestion_len : dict->prefix_length;
                if (sugg_prefix_len > input_prefix_len && (sugg_prefix_len - candidate_len) > max2) continue;

                int distance = 0, min = 0;
                if (candidate_len == 0) {
                    distance = (input_len > suggestion_len) ? input_len : suggestion_len;
                    if (distance > max2 ||
                        genset_seen_or_add((genset_t*)&dict->cand_seen, xxh3(suggestion,(size_t)suggestion_len), suggestion)) continue;
                } else if (suggestion_len == 1) {
                    distance = (strchr(query, suggestion[0]) == NULL) ? input_len : (input_len - 1);
                    if (distance > max2 ||
                        genset_seen_or_add((genset_t*)&dict->cand_seen, xxh3(suggestion,(size_t)suggestion_len), suggestion)) continue;
                } else if (((dict->prefix_length - max_edit_distance == candidate_len)
                            && (((min = ((input_len < suggestion_len ? input_len : suggestion_len) - dict->prefix_length)) > 1)
                                && (strcmp(query + (input_len + 1 - min), suggestion + (suggestion_len + 1 - min)) != 0)))
                           || ((min > 0) && (query[input_len - min] != suggestion[suggestion_len - min])
                               && ((query[input_len - min - 1] != suggestion[suggestion_len - min])
                                   || (query[input_len - min] != suggestion[suggestion_len - min - 1])))) {
                    continue;
                } else {
                    if (!delete_in_suggestion_prefix(candidate, candidate_len, suggestion, suggestion_len, dict->prefix_length)
                        || genset_seen_or_add((genset_t*)&dict->cand_seen, xxh3(suggestion,(size_t)suggestion_len), suggestion)) continue;
                    distance = damerau_osa(query, input_len, suggestion, suggestion_len, max2);
                    if (distance > max2) continue;
                }

                if (distance <= max2) {
                    if (candidate_count > 0 && distance < max2) candidate_count = 0;  /* Closest: drop worse */
                    max2 = distance;
                    strncpy(candidates[candidate_count].term, suggestion, SYMSPELL_MAX_TERM_LENGTH - 1);
                    candidates[candidate_count].term[SYMSPELL_MAX_TERM_LENGTH - 1] = '\0';
                    candidates[candidate_count].frequency = entry->frequencies[si];
                    candidates[candidate_count].distance = distance;
                    candidate_count++;
                }
            }
            break;
        }

        if ((length_diff < max_edit_distance) && (candidate_len <= dict->prefix_length)) {
            if (length_diff >= max2) continue;
            for (int i = 0; i < candidate_len; i++) {
                if (cand_list_count >= (int)dict->delete_buffer_capacity) break;
                char* dslot = cand_list[cand_list_count];
                int k = 0;
                for (int jj = 0; jj < candidate_len; jj++) if (jj != i) dslot[k++] = candidate[jj];
                dslot[k] = '\0';
                if (!genset_seen_or_add((genset_t*)&dict->delete_seen, xxh3(dslot,(size_t)k), dslot))
                    cand_list_count++;
            }
        }
    }

#ifdef DO_SORT
    if (candidate_count > 0) {
        qsort(candidates, candidate_count, sizeof(symspell_suggestion_t), compare_suggestions);
    }
    
    int result_count = (candidate_count < max_suggestions) ? candidate_count : max_suggestions;
    for (int i = 0; i < result_count; i++) {
        suggestions[i] = candidates[i];
    }
    pthread_mutex_unlock((pthread_mutex_t*)&dict->lookup_mutex);
    return result_count;
#else
    if (candidate_count > 0) {
        symspell_suggestion_t best_suggestion = candidates[0];
        for (int i = 1; i < candidate_count; i++) {
            if (candidates[i].distance < best_suggestion.distance) {
                best_suggestion = candidates[i];
            } else if (candidates[i].distance == best_suggestion.distance &&
                       candidates[i].frequency > best_suggestion.frequency) {
                best_suggestion = candidates[i];
            }
        }
        
        uint64_t best_hash = xxh3(best_suggestion.term, strlen(best_suggestion.term));
        float probability = symspell_get_probability(dict, best_hash);
        best_suggestion.probability = probability;
        best_suggestion.iwf = calculate_iwf(probability);

        suggestions[0] = best_suggestion;
        pthread_mutex_unlock((pthread_mutex_t*)&dict->lookup_mutex);
        return 1;
    }
    pthread_mutex_unlock((pthread_mutex_t*)&dict->lookup_mutex);
    return 0;
#endif
}

/* Get probability for a word hash */
float symspell_get_probability(const symspell_dict_t* dict, uint64_t word_hash) {
    if (!dict || !dict->exact_table) return 0.0f;

    size_t idx = word_hash % dict->exact_table->table_size;

    for (size_t probe = 0; probe < dict->exact_table->table_size; probe++) {
        size_t pos = (idx + probe) % dict->exact_table->table_size;

        if (dict->exact_table->hashes[pos] == 0) {
            return 0.0f;
        }

        if (dict->exact_table->hashes[pos] == word_hash) {
            return dict->exact_table->probabilities[pos];
        }
    }

    return 0.0f;
}

/* Get IWF for a word */
float symspell_get_iwf(const symspell_dict_t* dict, const char* word) {
    if (!dict || !dict->exact_table || !word) return 0.0f;

    uint64_t word_hash = xxh3(word, strlen(word));
    size_t idx = word_hash % dict->exact_table->table_size;

    for (size_t probe = 0; probe < dict->exact_table->table_size; probe++) {
        size_t pos = (idx + probe) % dict->exact_table->table_size;

        if (dict->exact_table->hashes[pos] == 0) {
            return 0.0f;
        }

        if (dict->exact_table->hashes[pos] == word_hash) {
            return dict->exact_table->iwf[pos];
        }
    }

    return 0.0f;
}

/* Destroy dictionary */
void symspell_destroy(symspell_dict_t* dict) {
    if (!dict) return;
    
    if (dict->exact_table) {
        free(dict->exact_table->hashes);
        free(dict->exact_table->frequencies);
        free(dict->exact_table->probabilities);
        free(dict->exact_table->iwf);
        free(dict->exact_table);
    }
    
    free(dict->delete_work_buffer);   /* slot array; slots point into delete_pool */
    free(dict->delete_pool);
    free(dict->delete_queue);
    genset_free(&dict->delete_seen);
    genset_free(&dict->cand_seen);
    free(dict->candidate_buffer);
    
    if (dict->table) {
        for (size_t i = 0; i < dict->table_size; i++) {
            if (dict->table[i]) {
                delete_entry_t* entry = dict->table[i];
                free(entry->words); 
                free(entry->frequencies);
            }
        }
        free(dict->table);
    }

    free(dict->string_arena.memory);
    free(dict->entry_arena.memory);
    
    pthread_mutex_destroy(&dict->lookup_mutex);
    free(dict);
}

/* Get statistics */
void symspell_get_stats(
    const symspell_dict_t* dict, size_t* word_count, size_t* entry_count
) {
    if (dict) {
        if (word_count) *word_count = dict->word_count;
        if (entry_count) *entry_count = dict->entry_count;
    }
}
