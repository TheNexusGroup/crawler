#include "memory_pool.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

// Helper macros for alignment
#define ALIGN_SIZE(size, alignment) (((size) + (alignment) - 1) & ~((alignment) - 1))
#define IS_ALIGNED(ptr, alignment) (((uintptr_t)(ptr) & ((alignment) - 1)) == 0)

// Helper function to get block header from user pointer
static inline MemoryBlock* get_block_from_ptr(void* ptr) {
    return (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
}

// Helper function to get user pointer from block
static inline void* get_ptr_from_block(MemoryBlock* block) {
    return (char*)block + sizeof(MemoryBlock);
}

MemoryPool* memory_pool_create(size_t pool_size) {
    if (pool_size < POOL_MIN_BLOCK_SIZE * 2) {
        logr(ERROR, "[MemoryPool] Pool size too small: %zu", pool_size);
        return NULL;
    }

    MemoryPool* pool = malloc(sizeof(MemoryPool));
    if (!pool) {
        logr(ERROR, "[MemoryPool] Failed to allocate memory pool structure");
        return NULL;
    }

    // Align pool size to system boundary
    pool_size = ALIGN_SIZE(pool_size, POOL_ALIGNMENT);

    // Allocate the main memory block
    pool->memory = malloc(pool_size);
    if (!pool->memory) {
        logr(ERROR, "[MemoryPool] Failed to allocate pool memory of size %zu", pool_size);
        free(pool);
        return NULL;
    }

    // Initialize pool structure
    pool->total_size = pool_size;
    pool->used_size = 0;
    pool->free_size = pool_size;
    pool->block_count = 1;
    pool->allocation_count = 0;
    pool->fragmentation_count = 0;
    pool->is_initialized = true;

    // Create initial free block covering the entire pool
    MemoryBlock* initial_block = (MemoryBlock*)pool->memory;
    initial_block->size = pool_size - sizeof(MemoryBlock);
    initial_block->is_free = true;
    initial_block->next = NULL;
    initial_block->prev = NULL;

    pool->free_list = initial_block;
    pool->used_list = NULL;

    logr(DEBUG, "[MemoryPool] Created memory pool with %zu bytes", pool_size);
    return pool;
}

void memory_pool_destroy(MemoryPool* pool) {
    if (!pool) return;

    if (pool->is_initialized && pool->allocation_count > 0) {
        logr(WARN, "[MemoryPool] Destroying pool with %zu outstanding allocations", 
             pool->allocation_count);
    }

    if (pool->memory) {
        free(pool->memory);
    }
    
    free(pool);
    logr(DEBUG, "[MemoryPool] Memory pool destroyed");
}

void* memory_pool_alloc(MemoryPool* pool, size_t size) {
    if (!pool || !pool->is_initialized || size == 0) {
        logr(ERROR, "[MemoryPool] Invalid allocation request");
        return NULL;
    }

    // Align size and add overhead for block header
    size_t aligned_size = ALIGN_SIZE(size, POOL_ALIGNMENT);
    size_t total_size = sizeof(MemoryBlock) + aligned_size;

    // Find suitable free block using first-fit strategy
    MemoryBlock* current = pool->free_list;
    while (current) {
        if (current->is_free && current->size >= aligned_size) {
            // Found suitable block
            break;
        }
        current = current->next;
    }

    if (!current) {
        logr(WARN, "[MemoryPool] No suitable block found for size %zu (pool usage: %zu/%zu)", 
             size, pool->used_size, pool->total_size);
        return NULL;
    }

    // Split block if it's significantly larger than needed
    if (current->size > aligned_size + sizeof(MemoryBlock) + POOL_MIN_BLOCK_SIZE) {
        MemoryBlock* new_block = (MemoryBlock*)((char*)current + sizeof(MemoryBlock) + aligned_size);
        new_block->size = current->size - aligned_size - sizeof(MemoryBlock);
        new_block->is_free = true;
        new_block->next = current->next;
        new_block->prev = current;

        if (current->next) {
            current->next->prev = new_block;
        }
        current->next = new_block;

        current->size = aligned_size;
        pool->block_count++;
    }

    // Mark block as used and update lists
    current->is_free = false;

    // Remove from free list
    if (current->prev) {
        current->prev->next = current->next;
    } else {
        pool->free_list = current->next;
    }

    if (current->next) {
        current->next->prev = current->prev;
    }

    // Add to used list
    current->prev = NULL;
    current->next = pool->used_list;
    if (pool->used_list) {
        pool->used_list->prev = current;
    }
    pool->used_list = current;

    // Update pool statistics
    pool->used_size += current->size + sizeof(MemoryBlock);
    pool->free_size -= current->size + sizeof(MemoryBlock);
    pool->allocation_count++;

    void* user_ptr = get_ptr_from_block(current);
    memset(user_ptr, 0, aligned_size); // Zero-initialize allocated memory

    logr(VERBOSE, "[MemoryPool] Allocated %zu bytes at %p", aligned_size, user_ptr);
    return user_ptr;
}

void memory_pool_free(MemoryPool* pool, void* ptr) {
    if (!pool || !pool->is_initialized || !ptr) {
        logr(WARN, "[MemoryPool] Invalid free request");
        return;
    }

    // Get block from user pointer
    MemoryBlock* block = get_block_from_ptr(ptr);

    // Validate block is within pool bounds
    if ((char*)block < (char*)pool->memory || 
        (char*)block >= (char*)pool->memory + pool->total_size) {
        logr(ERROR, "[MemoryPool] Attempting to free pointer outside pool bounds");
        return;
    }

    if (block->is_free) {
        logr(ERROR, "[MemoryPool] Double free detected at %p", ptr);
        return;
    }

    // Remove from used list
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        pool->used_list = block->next;
    }

    if (block->next) {
        block->next->prev = block->prev;
    }

    // Mark as free and add to free list
    block->is_free = true;
    block->next = pool->free_list;
    block->prev = NULL;
    
    if (pool->free_list) {
        pool->free_list->prev = block;
    }
    pool->free_list = block;

    // Update statistics
    pool->used_size -= block->size + sizeof(MemoryBlock);
    pool->free_size += block->size + sizeof(MemoryBlock);
    pool->allocation_count--;

    logr(VERBOSE, "[MemoryPool] Freed %zu bytes at %p", block->size, ptr);

    // Attempt to coalesce adjacent free blocks
    memory_pool_defragment(pool);
}

void* memory_pool_realloc(MemoryPool* pool, void* ptr, size_t new_size) {
    if (!pool || new_size == 0) {
        memory_pool_free(pool, ptr);
        return NULL;
    }

    if (!ptr) {
        return memory_pool_alloc(pool, new_size);
    }

    MemoryBlock* block = get_block_from_ptr(ptr);
    size_t aligned_new_size = ALIGN_SIZE(new_size, POOL_ALIGNMENT);

    // If new size fits in current block, just return the same pointer
    if (aligned_new_size <= block->size) {
        return ptr;
    }

    // Allocate new block and copy data
    void* new_ptr = memory_pool_alloc(pool, new_size);
    if (!new_ptr) {
        return NULL;
    }

    memcpy(new_ptr, ptr, block->size < aligned_new_size ? block->size : aligned_new_size);
    memory_pool_free(pool, ptr);

    return new_ptr;
}

void memory_pool_reset(MemoryPool* pool) {
    if (!pool || !pool->is_initialized) return;

    // Reset all blocks to a single free block
    MemoryBlock* initial_block = (MemoryBlock*)pool->memory;
    initial_block->size = pool->total_size - sizeof(MemoryBlock);
    initial_block->is_free = true;
    initial_block->next = NULL;
    initial_block->prev = NULL;

    pool->free_list = initial_block;
    pool->used_list = NULL;
    pool->used_size = 0;
    pool->free_size = pool->total_size;
    pool->block_count = 1;
    pool->allocation_count = 0;
    pool->fragmentation_count = 0;

    logr(DEBUG, "[MemoryPool] Pool reset complete");
}

bool memory_pool_defragment(MemoryPool* pool) {
    if (!pool || !pool->is_initialized) return false;

    bool coalesced = false;
    MemoryBlock* current = pool->free_list;

    while (current) {
        MemoryBlock* next = current->next;
        
        // Check if current block is adjacent to next block
        char* current_end = (char*)current + sizeof(MemoryBlock) + current->size;
        
        if (next && (char*)next == current_end && next->is_free) {
            // Coalesce blocks
            current->size += sizeof(MemoryBlock) + next->size;
            current->next = next->next;
            
            if (next->next) {
                next->next->prev = current;
            }
            
            pool->block_count--;
            coalesced = true;
            
            logr(VERBOSE, "[MemoryPool] Coalesced blocks, new size: %zu", current->size);
        } else {
            current = next;
        }
    }

    if (coalesced) {
        pool->fragmentation_count++;
    }

    return coalesced;
}

PoolStats memory_pool_stats(const MemoryPool* pool) {
    PoolStats stats = {0};
    
    if (!pool || !pool->is_initialized) {
        return stats;
    }

    stats.total_allocations = pool->allocation_count;
    stats.current_usage = pool->used_size;
    stats.peak_usage = pool->used_size; // Could track peak separately
    stats.fragmentation_ratio = (pool->block_count * 100) / (pool->total_size / POOL_MIN_BLOCK_SIZE);
    stats.block_count = pool->block_count;

    return stats;
}

bool memory_pool_validate(const MemoryPool* pool) {
    if (!pool || !pool->is_initialized || !pool->memory) {
        return false;
    }

    // Check if used_size + free_size equals total_size
    if (pool->used_size + pool->free_size != pool->total_size) {
        logr(ERROR, "[MemoryPool] Size mismatch: used=%zu + free=%zu != total=%zu",
             pool->used_size, pool->free_size, pool->total_size);
        return false;
    }

    return true;
}

void memory_pool_debug_print(const MemoryPool* pool) {
    if (!pool) return;

    PoolStats stats = memory_pool_stats(pool);
    
    logr(INFO, "[MemoryPool] Debug Info:");
    logr(INFO, "  Total Size: %zu bytes", pool->total_size);
    logr(INFO, "  Used Size: %zu bytes", pool->used_size);
    logr(INFO, "  Free Size: %zu bytes", pool->free_size);
    logr(INFO, "  Active Allocations: %zu", pool->allocation_count);
    logr(INFO, "  Block Count: %zu", pool->block_count);
    logr(INFO, "  Fragmentation: %zu%%", stats.fragmentation_ratio);
}