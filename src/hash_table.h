#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Hash table configuration
#define DEFAULT_TABLE_SIZE 1024
#define MAX_LOAD_FACTOR 0.75
#define MIN_LOAD_FACTOR 0.25
#define RESIZE_FACTOR 2

// Hash table entry
typedef struct HashEntry {
    char* key;
    void* value;
    uint32_t key_hash;
    struct HashEntry* next;
} HashEntry;

// Hash table bucket
typedef struct HashBucket {
    HashEntry* head;
    size_t count;
} HashBucket;

// Main hash table structure
typedef struct HashTable {
    HashBucket* buckets;
    size_t bucket_count;
    size_t entry_count;
    size_t collision_count;
    
    // Configuration
    bool auto_resize;
    float max_load_factor;
    float min_load_factor;
    
    // Function pointers for custom behavior
    uint32_t (*hash_function)(const char* key);
    int (*key_compare)(const char* key1, const char* key2);
    void (*value_destroyer)(void* value);
    void* (*value_copier)(const void* value);
    
    bool is_initialized;
} HashTable;

// Hash table iterator
typedef struct HashIterator {
    const HashTable* table;
    size_t bucket_index;
    HashEntry* current_entry;
    size_t visited_count;
} HashIterator;

// Hash table statistics
typedef struct HashStats {
    size_t total_entries;
    size_t total_buckets;
    size_t used_buckets;
    size_t max_bucket_size;
    size_t collision_count;
    float load_factor;
    float average_bucket_size;
} HashStats;

// Hash table functions
HashTable* hash_table_create(size_t initial_size);
HashTable* hash_table_create_with_options(
    size_t initial_size,
    uint32_t (*hash_func)(const char*),
    int (*key_cmp)(const char*, const char*),
    void (*value_destroy)(void*),
    void* (*value_copy)(const void*)
);
void hash_table_destroy(HashTable* table);

// Core operations
int hash_table_put(HashTable* table, const char* key, void* value);
void* hash_table_get(const HashTable* table, const char* key);
bool hash_table_contains(const HashTable* table, const char* key);
int hash_table_remove(HashTable* table, const char* key);
void hash_table_clear(HashTable* table);

// Batch operations
int hash_table_put_batch(HashTable* table, const char** keys, void** values, size_t count);
size_t hash_table_get_keys(const HashTable* table, char*** keys);
size_t hash_table_get_values(const HashTable* table, void*** values);

// Table management
int hash_table_resize(HashTable* table, size_t new_size);
int hash_table_rehash(HashTable* table);
void hash_table_optimize(HashTable* table);

// Iterator functions
HashIterator hash_table_iterator_create(const HashTable* table);
bool hash_table_iterator_has_next(HashIterator* iterator);
HashEntry* hash_table_iterator_next(HashIterator* iterator);
void hash_table_iterator_reset(HashIterator* iterator);

// Statistics and debugging
HashStats hash_table_get_stats(const HashTable* table);
void hash_table_debug_print(const HashTable* table);
float hash_table_get_load_factor(const HashTable* table);
size_t hash_table_get_collision_count(const HashTable* table);

// Hash functions
uint32_t hash_table_djb2_hash(const char* key);
uint32_t hash_table_fnv1a_hash(const char* key);
uint32_t hash_table_murmur3_hash(const char* key);
uint32_t hash_table_sdbm_hash(const char* key);

// Utility functions
size_t hash_table_size(const HashTable* table);
bool hash_table_is_empty(const HashTable* table);
void hash_table_set_auto_resize(HashTable* table, bool enabled);
void hash_table_set_load_factors(HashTable* table, float min_factor, float max_factor);

#endif // HASH_TABLE_H