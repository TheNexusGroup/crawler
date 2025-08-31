#ifndef INCREMENTAL_ANALYZER_H
#define INCREMENTAL_ANALYZER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "dependency.h"
#include "hash_table.h"
#include "file_cache.h"

// File state tracking
typedef enum {
    FILE_STATE_NEW,
    FILE_STATE_MODIFIED,
    FILE_STATE_UNCHANGED,
    FILE_STATE_DELETED,
    FILE_STATE_MOVED
} FileState;

// File fingerprint for change detection
typedef struct FileFingerprint {
    char* file_path;
    time_t last_modified;
    off_t file_size;
    uint64_t content_hash;
    uint64_t dependency_hash;  // Hash of extracted dependencies
    FileState state;
    time_t last_analyzed;
} FileFingerprint;

// Dependency change tracking
typedef struct DependencyChange {
    char* file_path;
    ExtractedDependency* old_dependencies;
    ExtractedDependency* new_dependencies;
    AnalysisLayer affected_layers;
    time_t change_time;
    struct DependencyChange* next;
} DependencyChange;

// Incremental analysis context
typedef struct IncrementalContext {
    HashTable* file_fingerprints;  // file_path -> FileFingerprint
    HashTable* dependency_cache;   // file_path -> ExtractedDependency
    FileCache* file_cache;
    
    // Change tracking
    DependencyChange* changes_head;
    size_t total_changes;
    size_t files_analyzed;
    size_t files_skipped;
    
    // Configuration
    bool enable_fingerprinting;
    bool enable_dependency_caching;
    bool track_file_moves;
    time_t max_cache_age;
    
    // Persistence
    char* state_file_path;
    bool auto_save_state;
    time_t last_state_save;
    
    bool is_initialized;
} IncrementalContext;

// Analysis result with change information
typedef struct IncrementalResult {
    ExtractedDependency* dependencies;
    FileState file_state;
    bool from_cache;
    size_t processing_time_ms;
    DependencyChange* changes;
} IncrementalResult;

// Analysis statistics
typedef struct IncrementalStats {
    size_t total_files_tracked;
    size_t files_analyzed;
    size_t files_skipped_unchanged;
    size_t files_from_cache;
    size_t dependency_changes_detected;
    size_t cache_hit_ratio_percent;
    size_t time_saved_ms;
} IncrementalStats;

// Incremental analyzer functions
IncrementalContext* incremental_context_create(const char* state_file_path);
void incremental_context_destroy(IncrementalContext* context);

// State management
int incremental_context_load_state(IncrementalContext* context);
int incremental_context_save_state(IncrementalContext* context);
int incremental_context_reset_state(IncrementalContext* context);

// File tracking
FileState incremental_get_file_state(IncrementalContext* context, const char* file_path);
int incremental_track_file(IncrementalContext* context, const char* file_path);
int incremental_update_fingerprint(IncrementalContext* context, const char* file_path);
bool incremental_has_file_changed(IncrementalContext* context, const char* file_path);

// Dependency analysis
IncrementalResult* incremental_analyze_file(
    IncrementalContext* context,
    const char* file_path,
    const LanguageGrammar* grammar
);
int incremental_analyze_directory(
    IncrementalContext* context,
    const char* directory,
    ExtractedDependency** results
);
int incremental_analyze_batch(
    IncrementalContext* context,
    char** file_paths,
    size_t count,
    IncrementalResult** results
);

// Change detection and tracking
DependencyChange* incremental_detect_changes(
    IncrementalContext* context,
    const char* file_path,
    const ExtractedDependency* new_deps
);
int incremental_apply_changes(IncrementalContext* context, DependencyChange* changes);
DependencyChange* incremental_get_all_changes(const IncrementalContext* context);

// Cache management
int incremental_invalidate_cache(IncrementalContext* context, const char* file_path);
int incremental_clear_cache(IncrementalContext* context);
int incremental_cleanup_stale_entries(IncrementalContext* context);

// Configuration
void incremental_set_fingerprinting(IncrementalContext* context, bool enabled);
void incremental_set_dependency_caching(IncrementalContext* context, bool enabled);
void incremental_set_track_moves(IncrementalContext* context, bool enabled);
void incremental_set_max_cache_age(IncrementalContext* context, time_t max_age);
void incremental_set_auto_save(IncrementalContext* context, bool enabled);

// Statistics and monitoring
IncrementalStats incremental_get_stats(const IncrementalContext* context);
void incremental_debug_print(const IncrementalContext* context);
float incremental_get_efficiency_ratio(const IncrementalContext* context);

// Utility functions
void incremental_result_free(IncrementalResult* result);
void dependency_change_free(DependencyChange* change);
uint64_t incremental_hash_dependencies(const ExtractedDependency* deps);
bool incremental_dependencies_equal(const ExtractedDependency* deps1, const ExtractedDependency* deps2);

#endif // INCREMENTAL_ANALYZER_H