#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Memory pool configuration
#define POOL_ALIGNMENT 8
#define POOL_MIN_BLOCK_SIZE 64
#define POOL_MAX_BLOCKS 1024
#define DEFAULT_POOL_SIZE (1024 * 1024) // 1MB default pool size

// Memory pool block structure
typedef struct MemoryBlock {
    size_t size;
    bool is_free;
    struct MemoryBlock* next;
    struct MemoryBlock* prev;
} MemoryBlock;

// Memory pool structure for efficient allocation
typedef struct MemoryPool {
    void* memory;
    size_t total_size;
    size_t used_size;
    size_t free_size;
    MemoryBlock* free_list;
    MemoryBlock* used_list;
    size_t block_count;
    size_t allocation_count;
    size_t fragmentation_count;
    bool is_initialized;
} MemoryPool;

// Pool statistics for monitoring
typedef struct PoolStats {
    size_t total_allocations;
    size_t total_frees;
    size_t peak_usage;
    size_t current_usage;
    size_t fragmentation_ratio;
    size_t block_count;
} PoolStats;

// Memory pool functions
MemoryPool* memory_pool_create(size_t pool_size);
void memory_pool_destroy(MemoryPool* pool);
void* memory_pool_alloc(MemoryPool* pool, size_t size);
void memory_pool_free(MemoryPool* pool, void* ptr);
void* memory_pool_realloc(MemoryPool* pool, void* ptr, size_t new_size);
void memory_pool_reset(MemoryPool* pool);
bool memory_pool_defragment(MemoryPool* pool);
PoolStats memory_pool_stats(const MemoryPool* pool);

// Aligned allocation helpers
void* memory_pool_alloc_aligned(MemoryPool* pool, size_t size, size_t alignment);
size_t memory_pool_get_block_size(const void* ptr);

// Pool validation and debugging
bool memory_pool_validate(const MemoryPool* pool);
void memory_pool_debug_print(const MemoryPool* pool);

#endif // MEMORY_POOL_H