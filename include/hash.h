/*
 * Portable Hash Table Implementation (C99)
 * Header-only, high-performance hash table
 * Compatible with Mac and Linux
 *
 * ============================================================================
 * NAME
 *     ht_create, ht_search, ht_delete, ht_destroy, ht_iterate - hash table
 *     management
 *
 * SYNOPSIS
 *     #include "hash.h"
 *
 *     HT_TABLE *ht_create(size_t initial_size);
 *     HT_ENTRY *ht_search(HT_TABLE *table, HT_ENTRY item, HT_ACTION action);
 *     int ht_delete(HT_TABLE *table, const char *key);
 *     void ht_destroy(HT_TABLE *table);
 *     void ht_destroy_free_keys(HT_TABLE *table);
 *     size_t ht_count(const HT_TABLE *table);
 *     void ht_iterate(HT_TABLE *table, ht_iterator_fn callback, void *user_data);
 *
 * DESCRIPTION
 *     These functions provide a portable, high-performance hash table
 *     implementation compatible with Mac and Linux systems. The API is
 *     similar to POSIX hsearch(3) but supports multiple independent tables
 *     and additional operations.
 *
 *     ht_create() creates a new hash table with the specified initial_size.
 *     If initial_size is 0, a default size (32) is used. The size is
 *     automatically rounded up to the next power of 2. Returns a pointer to
 *     the new table, or NULL on allocation failure.
 *
 *     ht_search() searches for or inserts an entry in the hash table. The
 *     item parameter contains the key and data to search for or insert. The
 *     action parameter specifies the operation:
 *
 *         HT_FIND   - Search for an entry with the given key
 *         HT_ENTER  - Insert a new entry or update an existing one
 *
 *     Returns a pointer to the found or inserted entry, or NULL if not found
 *     (when action is HT_FIND) or on allocation failure.
 *
 *     ht_delete() removes the entry with the specified key from the table.
 *     Returns 1 if the entry was found and deleted, 0 otherwise. The key and
 *     data pointers are not freed; the caller is responsible for memory
 *     management.
 *
 *     ht_destroy() frees all memory associated with the hash table, including
 *     the table structure itself. This function does NOT free the keys or
 *     data stored in the entries; the caller must manage that memory.
 *
 *     ht_destroy_free_keys() is similar to ht_destroy() but also calls
 *     free() on all keys in the table. The data pointers must still be freed
 *     by the caller if necessary.
 *
 *     ht_count() returns the current number of entries in the hash table.
 *
 *     ht_iterate() calls the provided callback function for each entry in the
 *     table. The callback receives the key, data, and user_data parameters.
 *     The iteration order is unspecified.
 *
 * STRUCTURES
 *     typedef struct {
 *         char *key;
 *         void *data;
 *     } HT_ENTRY;
 *
 *     The HT_ENTRY structure represents a key-value pair in the hash table.
 *
 *     typedef enum {
 *         HT_FIND,
 *         HT_ENTER
 *     } HT_ACTION;
 *
 *     The HT_ACTION enum specifies the operation for ht_search().
 *
 * RETURN VALUE
 *     ht_create() returns a pointer to the new hash table, or NULL on failure.
 *
 *     ht_search() returns a pointer to the entry, or NULL if not found or on
 *     allocation failure.
 *
 *     ht_delete() returns 1 on success, 0 if the key was not found.
 *
 *     ht_count() returns the number of entries in the table.
 *
 * NOTES
 *     - Keys are stored as pointers, not copied. The caller must ensure keys
 *       remain valid for the lifetime of the hash table entry.
 *     - The hash table automatically resizes when the load factor exceeds 75%.
 *     - All functions use linear probing for collision resolution.
 *     - The implementation uses FNV-1a hashing for good distribution.
 *     - All functions are inline for maximum performance.
 *     - Thread safety: Multiple threads can safely operate on different
 *       tables, but concurrent access to the same table requires external
 *       synchronization.
 *
 * EXAMPLES
 *     Create and use a hash table:
 *
 *         HT_TABLE *table = ht_create(0);
 *         if (!table) {
 *             // Handle allocation error
 *         }
 *
 *         // Insert an entry
 *         HT_ENTRY item = {"mykey", my_data_ptr};
 *         HT_ENTRY *result = ht_search(table, item, HT_ENTER);
 *
 *         // Find an entry
 *         HT_ENTRY find = {"mykey", NULL};
 *         result = ht_search(table, find, HT_FIND);
 *         if (result) {
 *             void *data = result->data;
 *         }
 *
 *         // Delete an entry
 *         if (ht_delete(table, "mykey")) {
 *             // Key was deleted
 *         }
 *
 *         // Iterate over all entries
 *         void print_entry(const char *key, void *data, void *user_data) {
 *             printf("Key: %s\n", key);
 *         }
 *         ht_iterate(table, print_entry, NULL);
 *
 *         // Cleanup
 *         ht_destroy(table);
 *
 * CONFIGURATION
 *     The following macros can be defined before including this header:
 *
 *     HT_INITIAL_SIZE - Default initial size (default: 32)
 *     HT_LOAD_FACTOR  - Resize threshold (default: 0.75)
 *
 * SEE ALSO
 *     hsearch(3), hcreate(3), hdestroy(3)
 *
 * AUTHORS
 *     This implementation is designed for portability and performance.
 * ============================================================================
 */

#ifndef HASH_H
#define HASH_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Configuration */
#ifndef HT_INITIAL_SIZE
#define HT_INITIAL_SIZE 32
#endif

#ifndef HT_LOAD_FACTOR
#define HT_LOAD_FACTOR 0.75
#endif

/* Entry action for hsearch-compatible API */
typedef enum {
    HT_FIND,
    HT_ENTER
} HT_ACTION;

/* Entry structure (compatible with POSIX ENTRY) */
typedef struct {
    char *key;
    void *data;
} HT_ENTRY;

/* Internal entry with metadata */
typedef struct {
    char *key;
    void *data;
    uint32_t hash;
    int occupied;
} HT_BUCKET;

/* Hash table structure */
typedef struct {
    HT_BUCKET *buckets;
    size_t size;
    size_t count;
} HT_TABLE;

/* FNV-1a hash function (fast and good distribution) */
static inline uint32_t ht_hash(const char *key) {
    uint32_t hash = 2166136261u;
    while (*key) {
        hash ^= (uint8_t)*key++;
        hash *= 16777619u;
    }
    return hash;
}

/* Create a new hash table */
static inline HT_TABLE *ht_create(size_t initial_size) {
    if (initial_size == 0) {
        initial_size = HT_INITIAL_SIZE;
    }
    
    /* Round up to power of 2 for efficient modulo */
    size_t size = 1;
    while (size < initial_size) {
        size <<= 1;
    }
    
    HT_TABLE *table = (HT_TABLE *)malloc(sizeof(HT_TABLE));
    if (!table) return NULL;
    
    table->buckets = (HT_BUCKET *)calloc(size, sizeof(HT_BUCKET));
    if (!table->buckets) {
        free(table);
        return NULL;
    }
    
    table->size = size;
    table->count = 0;
    return table;
}

/* Resize hash table */
static inline int ht_resize(HT_TABLE *table, size_t new_size) {
    HT_BUCKET *old_buckets = table->buckets;
    size_t old_size = table->size;
    
    table->buckets = (HT_BUCKET *)calloc(new_size, sizeof(HT_BUCKET));
    if (!table->buckets) {
        table->buckets = old_buckets;
        return 0;
    }
    
    table->size = new_size;
    table->count = 0;
    
    /* Rehash all entries */
    for (size_t i = 0; i < old_size; i++) {
        if (old_buckets[i].occupied) {
            uint32_t hash = old_buckets[i].hash;
            size_t idx = hash & (new_size - 1);
            
            /* Linear probing */
            while (table->buckets[idx].occupied) {
                idx = (idx + 1) & (new_size - 1);
            }
            
            table->buckets[idx] = old_buckets[i];
            table->count++;
        }
    }
    
    free(old_buckets);
    return 1;
}

/* Search or insert entry */
static inline HT_ENTRY *ht_search(HT_TABLE *table, HT_ENTRY item, HT_ACTION action) {
    if (!table || !item.key) return NULL;
    
    /* Check if resize needed */
    if (action == HT_ENTER && 
        (double)table->count / table->size >= HT_LOAD_FACTOR) {
        if (!ht_resize(table, table->size * 2)) {
            return NULL;
        }
    }
    
    uint32_t hash = ht_hash(item.key);
    size_t idx = hash & (table->size - 1);
    size_t start_idx = idx;
    
    /* Linear probing */
    while (1) {
        HT_BUCKET *bucket = &table->buckets[idx];
        
        if (!bucket->occupied) {
            if (action == HT_ENTER) {
                /* Insert new entry */
                bucket->key = item.key;
                bucket->data = item.data;
                bucket->hash = hash;
                bucket->occupied = 1;
                table->count++;
                return (HT_ENTRY *)bucket;
            }
            return NULL;
        }
        
        /* Check if keys match */
        if (bucket->hash == hash && strcmp(bucket->key, item.key) == 0) {
            if (action == HT_ENTER) {
                /* Update existing entry */
                bucket->data = item.data;
            }
            return (HT_ENTRY *)bucket;
        }
        
        idx = (idx + 1) & (table->size - 1);
        
        /* Table full (shouldn't happen with proper load factor) */
        if (idx == start_idx) {
            return NULL;
        }
    }
}

/* Delete entry from hash table */
static inline int ht_delete(HT_TABLE *table, const char *key) {
    if (!table || !key) return 0;
    
    uint32_t hash = ht_hash(key);
    size_t idx = hash & (table->size - 1);
    size_t start_idx = idx;
    
    while (table->buckets[idx].occupied) {
        HT_BUCKET *bucket = &table->buckets[idx];
        
        if (bucket->hash == hash && strcmp(bucket->key, key) == 0) {
            /* Mark as deleted */
            bucket->occupied = 0;
            bucket->key = NULL;
            bucket->data = NULL;
            table->count--;
            
            /* Rehash subsequent entries in the cluster */
            size_t next_idx = (idx + 1) & (table->size - 1);
            while (table->buckets[next_idx].occupied) {
                HT_BUCKET temp = table->buckets[next_idx];
                table->buckets[next_idx].occupied = 0;
                table->count--;
                
                HT_ENTRY item = {temp.key, temp.data};
                ht_search(table, item, HT_ENTER);
                
                next_idx = (next_idx + 1) & (table->size - 1);
            }
            
            return 1;
        }
        
        idx = (idx + 1) & (table->size - 1);
        if (idx == start_idx) break;
    }
    
    return 0;
}

/* Destroy hash table (does not free keys/data) */
static inline void ht_destroy(HT_TABLE *table) {
    if (table) {
        free(table->buckets);
        free(table);
    }
}

/* Destroy hash table and free keys (data must be freed by caller) */
static inline void ht_destroy_free_keys(HT_TABLE *table) {
    if (table) {
        for (size_t i = 0; i < table->size; i++) {
            if (table->buckets[i].occupied && table->buckets[i].key) {
                free(table->buckets[i].key);
            }
        }
        free(table->buckets);
        free(table);
    }
}

/* Get current entry count */
static inline size_t ht_count(const HT_TABLE *table) {
    return table ? table->count : 0;
}

/* Iterate over all entries */
typedef void (*ht_iterator_fn)(const char *key, void *data, void *user_data);

static inline void ht_iterate(HT_TABLE *table, ht_iterator_fn callback, void *user_data) {
    if (!table || !callback) return;
    
    for (size_t i = 0; i < table->size; i++) {
        if (table->buckets[i].occupied) {
            callback(table->buckets[i].key, table->buckets[i].data, user_data);
        }
    }
}

#endif /* HASH_H */
