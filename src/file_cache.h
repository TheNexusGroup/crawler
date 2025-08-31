#ifndef FILE_CACHE_H
#define FILE_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// File cache configuration
#define MAX_CACHE_ENTRIES 10000
#define DEFAULT_CACHE_SIZE (64 * 1024 * 1024)  // 64MB
#define CACHE_ENTRY_TIMEOUT 3600  // 1 hour
#define HASH_TABLE_SIZE 4096

// File metadata for cache validation
typedef struct FileMetadata {
    char* file_path;
    time_t last_modified;
    off_t file_size;
    uint64_t content_hash;
    bool is_valid;
} FileMetadata;

// Cached file content
typedef struct CachedFile {
    FileMetadata metadata;
    char* content;
    size_t content_size;
    time_t cached_at;
    size_t access_count;
    time_t last_accessed;
    struct CachedFile* next;
    struct CachedFile* prev;
    struct CachedFile* hash_next;
} CachedFile;

// Hash table entry
typedef struct HashEntry {
    CachedFile* head;
    size_t count;
} HashEntry;

// Main file cache structure
typedef struct FileCache {
    HashEntry* hash_table;
    CachedFile* lru_head;
    CachedFile* lru_tail;
    
    size_t max_size_bytes;
    size_t current_size_bytes;
    size_t max_entries;
    size_t current_entries;
    
    // Statistics
    size_t cache_hits;
    size_t cache_misses;
    size_t cache_evictions;
    size_t files_invalidated;
    
    // Configuration
    time_t entry_timeout;
    bool enable_compression;
    bool enable_memory_mapping;
    
    // Synchronization
    bool is_initialized;
} FileCache;

// Cache statistics
typedef struct CacheStats {
    size_t total_entries;
    size_t cache_hits;
    size_t cache_misses;
    size_t hit_ratio_percent;
    size_t memory_usage_bytes;
    size_t average_file_size;
    size_t eviction_count;
} CacheStats;

// File cache functions
FileCache* file_cache_create(size_t max_size_bytes, size_t max_entries);
void file_cache_destroy(FileCache* cache);

// Cache operations
char* file_cache_get(FileCache* cache, const char* file_path, size_t* content_size);
int file_cache_put(FileCache* cache, const char* file_path, const char* content, size_t content_size);
bool file_cache_contains(FileCache* cache, const char* file_path);
int file_cache_invalidate(FileCache* cache, const char* file_path);
void file_cache_clear(FileCache* cache);

// Cache management
int file_cache_evict_lru(FileCache* cache);
int file_cache_cleanup_expired(FileCache* cache);
int file_cache_validate_entries(FileCache* cache);
void file_cache_defragment(FileCache* cache);

// Statistics and monitoring
CacheStats file_cache_get_stats(const FileCache* cache);
void file_cache_debug_print(const FileCache* cache);
float file_cache_get_hit_ratio(const FileCache* cache);

// Configuration
void file_cache_set_timeout(FileCache* cache, time_t timeout_seconds);
void file_cache_set_compression(FileCache* cache, bool enabled);
void file_cache_set_memory_mapping(FileCache* cache, bool enabled);

// Utility functions
uint64_t file_cache_hash_content(const char* content, size_t size);
uint32_t file_cache_hash_path(const char* file_path);
bool file_cache_is_file_modified(const FileMetadata* metadata);

#endif // FILE_CACHE_H