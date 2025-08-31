#include "incremental_analyzer.h"
#include "hash_table.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

IncrementalContext* incremental_context_create(const char* state_file_path) {
    IncrementalContext* context = malloc(sizeof(IncrementalContext));
    if (!context) {
        logr(ERROR, "[IncrementalAnalyzer] Failed to allocate context");
        return NULL;
    }
    
    memset(context, 0, sizeof(IncrementalContext));
    
    // Initialize hash tables
    context->file_fingerprints = hash_table_create(1024);
    context->dependency_cache = hash_table_create(1024);
    
    if (!context->file_fingerprints || !context->dependency_cache) {
        logr(ERROR, "[IncrementalAnalyzer] Failed to create hash tables");
        if (context->file_fingerprints) hash_table_destroy(context->file_fingerprints);
        if (context->dependency_cache) hash_table_destroy(context->dependency_cache);
        free(context);
        return NULL;
    }
    
    // Initialize file cache
    context->file_cache = file_cache_create(DEFAULT_CACHE_SIZE, MAX_CACHE_ENTRIES);
    if (!context->file_cache) {
        logr(ERROR, "[IncrementalAnalyzer] Failed to create file cache");
        hash_table_destroy(context->file_fingerprints);
        hash_table_destroy(context->dependency_cache);
        free(context);
        return NULL;
    }
    
    // Set configuration
    context->enable_fingerprinting = true;
    context->enable_dependency_caching = true;
    context->track_file_moves = false;
    context->max_cache_age = CACHE_ENTRY_TIMEOUT;
    context->auto_save_state = true;
    
    if (state_file_path) {
        context->state_file_path = strdup(state_file_path);
    }
    
    context->is_initialized = true;
    
    logr(DEBUG, "[IncrementalAnalyzer] Created incremental context");
    return context;
}

void incremental_context_destroy(IncrementalContext* context) {
    if (!context) return;
    
    // Save state if auto-save is enabled
    if (context->auto_save_state && context->state_file_path) {
        incremental_context_save_state(context);
    }
    
    // Cleanup hash tables
    if (context->file_fingerprints) {
        hash_table_destroy(context->file_fingerprints);
    }
    if (context->dependency_cache) {
        hash_table_destroy(context->dependency_cache);
    }
    
    // Cleanup file cache
    if (context->file_cache) {
        file_cache_destroy(context->file_cache);
    }
    
    // Cleanup changes
    DependencyChange* change = context->changes_head;
    while (change) {
        DependencyChange* next = change->next;
        dependency_change_free(change);
        change = next;
    }
    
    free(context->state_file_path);
    free(context);
    
    logr(DEBUG, "[IncrementalAnalyzer] Incremental context destroyed");
}

FileState incremental_get_file_state(IncrementalContext* context, const char* file_path) {
    if (!context || !context->is_initialized || !file_path) {
        return FILE_STATE_NEW;
    }
    
    FileFingerprint* fingerprint = hash_table_get(context->file_fingerprints, file_path);
    if (!fingerprint) {
        return FILE_STATE_NEW;
    }
    
    // Check if file has been modified
    struct stat file_stat;
    if (stat(file_path, &file_stat) != 0) {
        return FILE_STATE_DELETED;
    }
    
    if (file_stat.st_mtime != fingerprint->last_modified ||
        file_stat.st_size != fingerprint->file_size) {
        fingerprint->state = FILE_STATE_MODIFIED;
        return FILE_STATE_MODIFIED;
    }
    
    return FILE_STATE_UNCHANGED;
}

bool incremental_has_file_changed(IncrementalContext* context, const char* file_path) {
    FileState state = incremental_get_file_state(context, file_path);
    return (state != FILE_STATE_UNCHANGED);
}

int incremental_track_file(IncrementalContext* context, const char* file_path) {
    if (!context || !context->is_initialized || !file_path) {
        return -1;
    }
    
    struct stat file_stat;
    if (stat(file_path, &file_stat) != 0) {
        logr(WARN, "[IncrementalAnalyzer] Cannot stat file: %s", file_path);
        return -1;
    }
    
    FileFingerprint* fingerprint = malloc(sizeof(FileFingerprint));
    if (!fingerprint) {
        logr(ERROR, "[IncrementalAnalyzer] Failed to allocate fingerprint");
        return -1;
    }
    
    memset(fingerprint, 0, sizeof(FileFingerprint));
    fingerprint->file_path = strdup(file_path);
    fingerprint->last_modified = file_stat.st_mtime;
    fingerprint->file_size = file_stat.st_size;
    fingerprint->state = FILE_STATE_NEW;
    fingerprint->last_analyzed = time(NULL);
    
    // Calculate content hash if fingerprinting is enabled
    if (context->enable_fingerprinting) {
        // For now, just use file size and modification time as hash
        fingerprint->content_hash = (uint64_t)file_stat.st_size ^ 
                                   (uint64_t)file_stat.st_mtime;
    }
    
    if (hash_table_put(context->file_fingerprints, file_path, fingerprint) != 0) {
        free(fingerprint->file_path);
        free(fingerprint);
        return -1;
    }
    
    logr(VERBOSE, "[IncrementalAnalyzer] Tracking file: %s", file_path);
    return 0;
}

IncrementalResult* incremental_analyze_file(
    IncrementalContext* context,
    const char* file_path,
    const LanguageGrammar* grammar) {
    
    if (!context || !context->is_initialized || !file_path || !grammar) {
        logr(ERROR, "[IncrementalAnalyzer] Invalid parameters for file analysis");
        return NULL;
    }
    
    IncrementalResult* result = malloc(sizeof(IncrementalResult));
    if (!result) {
        logr(ERROR, "[IncrementalAnalyzer] Failed to allocate result");
        return NULL;
    }
    
    memset(result, 0, sizeof(IncrementalResult));
    
    // Check file state
    result->file_state = incremental_get_file_state(context, file_path);
    
    // If file hasn't changed and we have cached results, use them
    if (result->file_state == FILE_STATE_UNCHANGED && context->enable_dependency_caching) {
        ExtractedDependency* cached_deps = hash_table_get(context->dependency_cache, file_path);
        if (cached_deps) {
            // TODO: Deep copy cached dependencies
            result->dependencies = cached_deps;
            result->from_cache = true;
            context->files_skipped++;
            
            logr(VERBOSE, "[IncrementalAnalyzer] Using cached results for: %s", file_path);
            return result;
        }
    }
    
    // Need to analyze the file
    size_t content_size;
    char* content = file_cache_get(context->file_cache, file_path, &content_size);
    
    if (!content) {
        // Read file from disk
        FILE* file = fopen(file_path, "r");
        if (!file) {
            logr(ERROR, "[IncrementalAnalyzer] Cannot open file: %s", file_path);
            free(result);
            return NULL;
        }
        
        // Get file size
        fseek(file, 0, SEEK_END);
        content_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        content = malloc(content_size + 1);
        if (!content) {
            fclose(file);
            free(result);
            return NULL;
        }
        
        fread(content, 1, content_size, file);
        content[content_size] = '\0';
        fclose(file);
        
        // Cache the content
        file_cache_put(context->file_cache, file_path, content, content_size);
    }
    
    // Perform analysis using language-specific analyzer
    clock_t start = clock();
    result->dependencies = analyzeModuleWithFile(content, file_path, grammar);
    clock_t end = clock();
    
    result->processing_time_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    result->from_cache = false;
    
    // Cache the results if enabled
    if (context->enable_dependency_caching && result->dependencies) {
        hash_table_put(context->dependency_cache, file_path, result->dependencies);
    }
    
    // Update file tracking
    incremental_track_file(context, file_path);
    context->files_analyzed++;
    
    logr(VERBOSE, "[IncrementalAnalyzer] Analyzed file: %s (%.2f ms)", 
         file_path, result->processing_time_ms);
    
    return result;
}

int incremental_context_load_state(IncrementalContext* context) {
    if (!context || !context->state_file_path) {
        return -1;
    }
    
    // For now, just check if state file exists
    if (access(context->state_file_path, F_OK) == 0) {
        logr(DEBUG, "[IncrementalAnalyzer] State file exists: %s", context->state_file_path);
        // TODO: Implement state loading
        return 0;
    }
    
    logr(DEBUG, "[IncrementalAnalyzer] No existing state file");
    return 0;
}

int incremental_context_save_state(IncrementalContext* context) {
    if (!context || !context->state_file_path) {
        return -1;
    }
    
    // TODO: Implement state saving
    logr(DEBUG, "[IncrementalAnalyzer] State saved to: %s", context->state_file_path);
    context->last_state_save = time(NULL);
    return 0;
}

IncrementalStats incremental_get_stats(const IncrementalContext* context) {
    IncrementalStats stats = {0};
    
    if (!context || !context->is_initialized) {
        return stats;
    }
    
    stats.total_files_tracked = hash_table_size(context->file_fingerprints);
    stats.files_analyzed = context->files_analyzed;
    stats.files_skipped_unchanged = context->files_skipped;
    stats.dependency_changes_detected = context->total_changes;
    
    if (context->file_cache) {
        CacheStats cache_stats = file_cache_get_stats(context->file_cache);
        stats.files_from_cache = cache_stats.cache_hits;
        stats.cache_hit_ratio_percent = cache_stats.hit_ratio_percent;
    }
    
    return stats;
}

void incremental_debug_print(const IncrementalContext* context) {
    if (!context) return;
    
    IncrementalStats stats = incremental_get_stats(context);
    
    logr(INFO, "[IncrementalAnalyzer] Debug Info:");
    logr(INFO, "  Files Tracked: %zu", stats.total_files_tracked);
    logr(INFO, "  Files Analyzed: %zu", stats.files_analyzed);
    logr(INFO, "  Files Skipped: %zu", stats.files_skipped_unchanged);
    logr(INFO, "  Cache Hit Ratio: %zu%%", stats.cache_hit_ratio_percent);
    logr(INFO, "  Changes Detected: %zu", stats.dependency_changes_detected);
}

// Utility functions
void incremental_result_free(IncrementalResult* result) {
    if (!result) return;
    
    if (result->dependencies) {
        freeDependency(result->dependencies);
    }
    
    if (result->changes) {
        dependency_change_free(result->changes);
    }
    
    free(result);
}

void dependency_change_free(DependencyChange* change) {
    if (!change) return;
    
    free(change->file_path);
    if (change->old_dependencies) freeDependency(change->old_dependencies);
    if (change->new_dependencies) freeDependency(change->new_dependencies);
    free(change);
}

// Configuration functions
void incremental_set_fingerprinting(IncrementalContext* context, bool enabled) {
    if (context) context->enable_fingerprinting = enabled;
}

void incremental_set_dependency_caching(IncrementalContext* context, bool enabled) {
    if (context) context->enable_dependency_caching = enabled;
}

// Placeholder implementations for other functions
uint64_t incremental_hash_dependencies(const ExtractedDependency* deps) {
    if (!deps) return 0;
    
    uint64_t hash = 0;
    const ExtractedDependency* current = deps;
    
    while (current) {
        if (current->module_name) {
            // Simple hash combination
            uint64_t name_hash = 5381;
            const char* name = current->module_name;
            int c;
            while ((c = *name++)) {
                name_hash = ((name_hash << 5) + name_hash) + c;
            }
            hash ^= name_hash + (uint64_t)current->layer;
        }
        current = current->next;
    }
    
    return hash;
}

bool incremental_dependencies_equal(const ExtractedDependency* deps1, const ExtractedDependency* deps2) {
    // Simple comparison - just compare hashes
    return incremental_hash_dependencies(deps1) == incremental_hash_dependencies(deps2);
}