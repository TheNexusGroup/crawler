#include "hash_table.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

// Simple hash function (djb2)
uint32_t hash_table_djb2_hash(const char* key) {
    if (!key) return 0;
    
    uint32_t hash = 5381;
    int c;
    
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash;
}

HashTable* hash_table_create(size_t initial_size) {
    if (initial_size == 0) {
        initial_size = DEFAULT_TABLE_SIZE;
    }
    
    HashTable* table = malloc(sizeof(HashTable));
    if (!table) {
        logr(ERROR, "[HashTable] Failed to allocate hash table");
        return NULL;
    }
    
    memset(table, 0, sizeof(HashTable));
    
    table->buckets = calloc(initial_size, sizeof(HashBucket));
    if (!table->buckets) {
        logr(ERROR, "[HashTable] Failed to allocate buckets");
        free(table);
        return NULL;
    }
    
    table->bucket_count = initial_size;
    table->entry_count = 0;
    table->collision_count = 0;
    table->auto_resize = true;
    table->max_load_factor = MAX_LOAD_FACTOR;
    table->min_load_factor = MIN_LOAD_FACTOR;
    table->hash_function = hash_table_djb2_hash;
    table->key_compare = strcmp;
    table->value_destroyer = NULL;
    table->value_copier = NULL;
    table->is_initialized = true;
    
    logr(DEBUG, "[HashTable] Created hash table with %zu buckets", initial_size);
    return table;
}

void hash_table_destroy(HashTable* table) {
    if (!table) return;
    
    hash_table_clear(table);
    free(table->buckets);
    free(table);
    
    logr(DEBUG, "[HashTable] Hash table destroyed");
}

int hash_table_put(HashTable* table, const char* key, void* value) {
    if (!table || !table->is_initialized || !key) {
        logr(ERROR, "[HashTable] Invalid parameters for put operation");
        return -1;
    }
    
    uint32_t hash = table->hash_function(key);
    size_t bucket_index = hash % table->bucket_count;
    HashBucket* bucket = &table->buckets[bucket_index];
    
    // Check if key already exists
    HashEntry* current = bucket->head;
    while (current) {
        if (table->key_compare(current->key, key) == 0) {
            // Update existing entry
            if (table->value_destroyer && current->value) {
                table->value_destroyer(current->value);
            }
            current->value = value;
            return 0;
        }
        current = current->next;
    }
    
    // Create new entry
    HashEntry* entry = malloc(sizeof(HashEntry));
    if (!entry) {
        logr(ERROR, "[HashTable] Failed to allocate hash entry");
        return -1;
    }
    
    entry->key = strdup(key);
    entry->value = value;
    entry->key_hash = hash;
    entry->next = bucket->head;
    
    bucket->head = entry;
    bucket->count++;
    table->entry_count++;
    
    if (bucket->count > 1) {
        table->collision_count++;
    }
    
    // Check if resize is needed
    if (table->auto_resize && 
        (float)table->entry_count / table->bucket_count > table->max_load_factor) {
        hash_table_resize(table, table->bucket_count * RESIZE_FACTOR);
    }
    
    return 0;
}

void* hash_table_get(const HashTable* table, const char* key) {
    if (!table || !table->is_initialized || !key) {
        return NULL;
    }
    
    uint32_t hash = table->hash_function(key);
    size_t bucket_index = hash % table->bucket_count;
    HashBucket* bucket = &table->buckets[bucket_index];
    
    HashEntry* current = bucket->head;
    while (current) {
        if (table->key_compare(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    
    return NULL;
}

bool hash_table_contains(const HashTable* table, const char* key) {
    return hash_table_get(table, key) != NULL;
}

int hash_table_remove(HashTable* table, const char* key) {
    if (!table || !table->is_initialized || !key) {
        return -1;
    }
    
    uint32_t hash = table->hash_function(key);
    size_t bucket_index = hash % table->bucket_count;
    HashBucket* bucket = &table->buckets[bucket_index];
    
    HashEntry* current = bucket->head;
    HashEntry* previous = NULL;
    
    while (current) {
        if (table->key_compare(current->key, key) == 0) {
            // Remove from linked list
            if (previous) {
                previous->next = current->next;
            } else {
                bucket->head = current->next;
            }
            
            bucket->count--;
            table->entry_count--;
            
            // Clean up entry
            free(current->key);
            if (table->value_destroyer && current->value) {
                table->value_destroyer(current->value);
            }
            free(current);
            
            return 0;
        }
        
        previous = current;
        current = current->next;
    }
    
    return -1; // Not found
}

void hash_table_clear(HashTable* table) {
    if (!table || !table->is_initialized) return;
    
    for (size_t i = 0; i < table->bucket_count; i++) {
        HashBucket* bucket = &table->buckets[i];
        HashEntry* current = bucket->head;
        
        while (current) {
            HashEntry* next = current->next;
            free(current->key);
            if (table->value_destroyer && current->value) {
                table->value_destroyer(current->value);
            }
            free(current);
            current = next;
        }
        
        bucket->head = NULL;
        bucket->count = 0;
    }
    
    table->entry_count = 0;
    table->collision_count = 0;
}

int hash_table_resize(HashTable* table, size_t new_size) {
    if (!table || !table->is_initialized || new_size == 0) {
        return -1;
    }
    
    // Save old buckets
    HashBucket* old_buckets = table->buckets;
    size_t old_bucket_count = table->bucket_count;
    
    // Allocate new buckets
    table->buckets = calloc(new_size, sizeof(HashBucket));
    if (!table->buckets) {
        table->buckets = old_buckets;
        return -1;
    }
    
    table->bucket_count = new_size;
    table->entry_count = 0;
    table->collision_count = 0;
    
    // Rehash all entries
    for (size_t i = 0; i < old_bucket_count; i++) {
        HashEntry* current = old_buckets[i].head;
        while (current) {
            HashEntry* next = current->next;
            
            // Rehash entry
            size_t bucket_index = current->key_hash % new_size;
            HashBucket* bucket = &table->buckets[bucket_index];
            
            current->next = bucket->head;
            bucket->head = current;
            bucket->count++;
            table->entry_count++;
            
            if (bucket->count > 1) {
                table->collision_count++;
            }
            
            current = next;
        }
    }
    
    free(old_buckets);
    logr(DEBUG, "[HashTable] Resized hash table to %zu buckets", new_size);
    return 0;
}

HashStats hash_table_get_stats(const HashTable* table) {
    HashStats stats = {0};
    
    if (!table || !table->is_initialized) {
        return stats;
    }
    
    stats.total_entries = table->entry_count;
    stats.total_buckets = table->bucket_count;
    stats.collision_count = table->collision_count;
    stats.load_factor = (float)table->entry_count / table->bucket_count;
    
    // Count used buckets and find max bucket size
    size_t used_buckets = 0;
    size_t max_bucket_size = 0;
    size_t total_bucket_size = 0;
    
    for (size_t i = 0; i < table->bucket_count; i++) {
        if (table->buckets[i].count > 0) {
            used_buckets++;
            if (table->buckets[i].count > max_bucket_size) {
                max_bucket_size = table->buckets[i].count;
            }
            total_bucket_size += table->buckets[i].count;
        }
    }
    
    stats.used_buckets = used_buckets;
    stats.max_bucket_size = max_bucket_size;
    stats.average_bucket_size = used_buckets > 0 ? (float)total_bucket_size / used_buckets : 0.0f;
    
    return stats;
}

size_t hash_table_size(const HashTable* table) {
    return table ? table->entry_count : 0;
}

bool hash_table_is_empty(const HashTable* table) {
    return table ? (table->entry_count == 0) : true;
}

float hash_table_get_load_factor(const HashTable* table) {
    if (!table || table->bucket_count == 0) return 0.0f;
    return (float)table->entry_count / table->bucket_count;
}

void hash_table_debug_print(const HashTable* table) {
    if (!table) return;
    
    HashStats stats = hash_table_get_stats(table);
    
    logr(INFO, "[HashTable] Debug Info:");
    logr(INFO, "  Total Entries: %zu", stats.total_entries);
    logr(INFO, "  Total Buckets: %zu", stats.total_buckets);
    logr(INFO, "  Used Buckets: %zu", stats.used_buckets);
    logr(INFO, "  Load Factor: %.2f", stats.load_factor);
    logr(INFO, "  Max Bucket Size: %zu", stats.max_bucket_size);
    logr(INFO, "  Average Bucket Size: %.2f", stats.average_bucket_size);
    logr(INFO, "  Collision Count: %zu", stats.collision_count);
}

// Placeholder implementations for other hash functions
uint32_t hash_table_fnv1a_hash(const char* key) {
    return hash_table_djb2_hash(key); // Simplified
}

uint32_t hash_table_murmur3_hash(const char* key) {
    return hash_table_djb2_hash(key); // Simplified
}

uint32_t hash_table_sdbm_hash(const char* key) {
    return hash_table_djb2_hash(key); // Simplified
}