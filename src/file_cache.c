#include "file_cache.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Simple hash function for file paths
uint32_t file_cache_hash_path(const char* file_path) {
    if (!file_path) return 0;
    
    uint32_t hash = 5381;
    int c;
    
    while ((c = *file_path++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash % HASH_TABLE_SIZE;
}

FileCache* file_cache_create(size_t max_size_bytes, size_t max_entries) {
    FileCache* cache = malloc(sizeof(FileCache));
    if (!cache) {
        logr(ERROR, "[FileCache] Failed to allocate file cache");
        return NULL;
    }
    
    memset(cache, 0, sizeof(FileCache));
    
    cache->hash_table = calloc(HASH_TABLE_SIZE, sizeof(HashEntry));
    if (!cache->hash_table) {
        logr(ERROR, "[FileCache] Failed to allocate hash table");
        free(cache);
        return NULL;
    }
    
    cache->max_size_bytes = max_size_bytes;
    cache->current_size_bytes = 0;
    cache->max_entries = max_entries;
    cache->current_entries = 0;
    cache->cache_hits = 0;
    cache->cache_misses = 0;
    cache->entry_timeout = CACHE_ENTRY_TIMEOUT;
    cache->enable_compression = false;
    cache->enable_memory_mapping = false;
    cache->is_initialized = true;
    
    logr(DEBUG, "[FileCache] Created file cache with %zu max entries, %zu max bytes", 
         max_entries, max_size_bytes);
    return cache;
}

void file_cache_destroy(FileCache* cache) {
    if (!cache) return;
    
    file_cache_clear(cache);
    free(cache->hash_table);
    free(cache);
    
    logr(DEBUG, "[FileCache] File cache destroyed");
}

char* file_cache_get(FileCache* cache, const char* file_path, size_t* content_size) {
    if (!cache || !cache->is_initialized || !file_path) {
        cache->cache_misses++;
        return NULL;
    }
    
    uint32_t hash = file_cache_hash_path(file_path);
    CachedFile* current = cache->hash_table[hash].head;
    
    while (current) {
        if (strcmp(current->metadata.file_path, file_path) == 0) {
            // Check if file is still valid
            if (file_cache_is_file_modified(&current->metadata)) {
                // File has been modified, invalidate cache entry
                file_cache_invalidate(cache, file_path);
                break;
            }
            
            // Update access information
            current->access_count++;
            current->last_accessed = time(NULL);
            
            // Move to front of LRU list
            if (current != cache->lru_head) {
                // Remove from current position
                if (current->prev) current->prev->next = current->next;
                if (current->next) current->next->prev = current->prev;
                if (current == cache->lru_tail) cache->lru_tail = current->prev;
                
                // Add to front
                current->prev = NULL;
                current->next = cache->lru_head;
                if (cache->lru_head) cache->lru_head->prev = current;
                cache->lru_head = current;
                if (!cache->lru_tail) cache->lru_tail = current;
            }
            
            cache->cache_hits++;
            if (content_size) *content_size = current->content_size;
            return current->content;
        }
        current = current->hash_next;
    }
    
    cache->cache_misses++;
    return NULL;
}

int file_cache_put(FileCache* cache, const char* file_path, const char* content, size_t content_size) {
    if (!cache || !cache->is_initialized || !file_path || !content) {
        logr(ERROR, "[FileCache] Invalid parameters for put operation");
        return -1;
    }
    
    // Check if we need to evict entries
    while ((cache->current_entries >= cache->max_entries) ||
           (cache->current_size_bytes + content_size > cache->max_size_bytes)) {
        if (file_cache_evict_lru(cache) != 0) {
            break;
        }
    }
    
    // Create new cached file
    CachedFile* cached_file = malloc(sizeof(CachedFile));
    if (!cached_file) {
        logr(ERROR, "[FileCache] Failed to allocate cached file");
        return -1;
    }
    
    memset(cached_file, 0, sizeof(CachedFile));
    
    // Set up metadata
    cached_file->metadata.file_path = strdup(file_path);
    cached_file->metadata.file_size = content_size;
    cached_file->metadata.last_modified = time(NULL);
    cached_file->metadata.is_valid = true;
    
    // Copy content
    cached_file->content = malloc(content_size + 1);
    if (!cached_file->content) {
        free(cached_file->metadata.file_path);
        free(cached_file);
        return -1;
    }
    
    memcpy(cached_file->content, content, content_size);
    cached_file->content[content_size] = '\0';
    cached_file->content_size = content_size;
    cached_file->cached_at = time(NULL);
    cached_file->last_accessed = time(NULL);
    cached_file->access_count = 1;
    
    // Add to hash table
    uint32_t hash = file_cache_hash_path(file_path);
    cached_file->hash_next = cache->hash_table[hash].head;
    cache->hash_table[hash].head = cached_file;
    cache->hash_table[hash].count++;
    
    // Add to front of LRU list
    cached_file->next = cache->lru_head;
    if (cache->lru_head) cache->lru_head->prev = cached_file;
    cache->lru_head = cached_file;
    if (!cache->lru_tail) cache->lru_tail = cached_file;
    
    cache->current_entries++;
    cache->current_size_bytes += content_size;
    
    logr(VERBOSE, "[FileCache] Cached file: %s (%zu bytes)", file_path, content_size);
    return 0;
}

bool file_cache_contains(FileCache* cache, const char* file_path) {
    if (!cache || !file_path) return false;
    
    size_t size;
    return file_cache_get(cache, file_path, &size) != NULL;
}

int file_cache_invalidate(FileCache* cache, const char* file_path) {
    if (!cache || !cache->is_initialized || !file_path) {
        return -1;
    }
    
    uint32_t hash = file_cache_hash_path(file_path);
    CachedFile* current = cache->hash_table[hash].head;
    CachedFile* previous = NULL;
    
    while (current) {
        if (strcmp(current->metadata.file_path, file_path) == 0) {
            // Remove from hash table
            if (previous) {
                previous->hash_next = current->hash_next;
            } else {
                cache->hash_table[hash].head = current->hash_next;
            }
            cache->hash_table[hash].count--;
            
            // Remove from LRU list
            if (current->prev) current->prev->next = current->next;
            if (current->next) current->next->prev = current->prev;
            if (current == cache->lru_head) cache->lru_head = current->next;
            if (current == cache->lru_tail) cache->lru_tail = current->prev;
            
            // Update cache stats
            cache->current_entries--;
            cache->current_size_bytes -= current->content_size;
            cache->files_invalidated++;
            
            // Free memory
            free(current->metadata.file_path);
            free(current->content);
            free(current);
            
            logr(VERBOSE, "[FileCache] Invalidated file: %s", file_path);
            return 0;
        }
        
        previous = current;
        current = current->hash_next;
    }
    
    return -1; // Not found
}

void file_cache_clear(FileCache* cache) {
    if (!cache || !cache->is_initialized) return;
    
    for (size_t i = 0; i < HASH_TABLE_SIZE; i++) {
        CachedFile* current = cache->hash_table[i].head;
        while (current) {
            CachedFile* next = current->hash_next;
            free(current->metadata.file_path);
            free(current->content);
            free(current);
            current = next;
        }
        cache->hash_table[i].head = NULL;
        cache->hash_table[i].count = 0;
    }
    
    cache->lru_head = NULL;
    cache->lru_tail = NULL;
    cache->current_entries = 0;
    cache->current_size_bytes = 0;
    
    logr(DEBUG, "[FileCache] Cache cleared");
}

int file_cache_evict_lru(FileCache* cache) {
    if (!cache || !cache->lru_tail) return -1;
    
    CachedFile* lru = cache->lru_tail;
    return file_cache_invalidate(cache, lru->metadata.file_path);
}

CacheStats file_cache_get_stats(const FileCache* cache) {
    CacheStats stats = {0};
    
    if (!cache || !cache->is_initialized) {
        return stats;
    }
    
    stats.total_entries = cache->current_entries;
    stats.cache_hits = cache->cache_hits;
    stats.cache_misses = cache->cache_misses;
    stats.hit_ratio_percent = (cache->cache_hits + cache->cache_misses) > 0 ? 
        (cache->cache_hits * 100) / (cache->cache_hits + cache->cache_misses) : 0;
    stats.memory_usage_bytes = cache->current_size_bytes;
    stats.average_file_size = cache->current_entries > 0 ? 
        cache->current_size_bytes / cache->current_entries : 0;
    stats.eviction_count = cache->cache_evictions;
    
    return stats;
}

bool file_cache_is_file_modified(const FileMetadata* metadata) {
    if (!metadata || !metadata->file_path) return true;
    
    struct stat file_stat;
    if (stat(metadata->file_path, &file_stat) != 0) {
        return true; // File doesn't exist or can't be accessed
    }
    
    return (file_stat.st_mtime != metadata->last_modified) ||
           (file_stat.st_size != metadata->file_size);
}

void file_cache_debug_print(const FileCache* cache) {
    if (!cache) return;
    
    CacheStats stats = file_cache_get_stats(cache);
    
    logr(INFO, "[FileCache] Debug Info:");
    logr(INFO, "  Total Entries: %zu", stats.total_entries);
    logr(INFO, "  Cache Hits: %zu", stats.cache_hits);
    logr(INFO, "  Cache Misses: %zu", stats.cache_misses);
    logr(INFO, "  Hit Ratio: %zu%%", stats.hit_ratio_percent);
    logr(INFO, "  Memory Usage: %zu bytes", stats.memory_usage_bytes);
    logr(INFO, "  Average File Size: %zu bytes", stats.average_file_size);
}

// Placeholder implementations
uint64_t file_cache_hash_content(const char* content, size_t size) {
    // Simple hash for content
    uint64_t hash = 5381;
    for (size_t i = 0; i < size; i++) {
        hash = ((hash << 5) + hash) + content[i];
    }
    return hash;
}

float file_cache_get_hit_ratio(const FileCache* cache) {
    if (!cache) return 0.0f;
    
    size_t total = cache->cache_hits + cache->cache_misses;
    return total > 0 ? (float)cache->cache_hits / total : 0.0f;
}