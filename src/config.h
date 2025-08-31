#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdbool.h>

// Performance configuration
#define ENABLE_PARALLEL_PROCESSING 1
#define ENABLE_MEMORY_POOL 1
#define ENABLE_FILE_CACHE 1
#define ENABLE_INCREMENTAL_ANALYSIS 1
#define ENABLE_HASH_TABLE_OPTIMIZATION 1

// Thread configuration
#define DEFAULT_THREAD_COUNT 4
#define MAX_THREAD_COUNT 16

// Memory configuration
#define DEFAULT_MEMORY_POOL_SIZE (16 * 1024 * 1024)  // 16MB
#define DEFAULT_FILE_CACHE_SIZE (64 * 1024 * 1024)   // 64MB
#define HASH_TABLE_INITIAL_SIZE 4096

// File processing limits
#define MAX_FILE_SIZE (100 * 1024 * 1024)  // 100MB
#define MAX_FILES_PER_BATCH 1000
#define MAX_DEPENDENCY_DEPTH 50

// Cache configuration
#define CACHE_ENTRY_TIMEOUT 3600  // 1 hour
#define ENABLE_CACHE_COMPRESSION 0
#define ENABLE_MEMORY_MAPPING 1

// Analysis configuration
#define DEFAULT_ANALYSIS_DEPTH 10
#define ENABLE_EXTERNAL_DEPENDENCIES 1
#define ENABLE_CIRCULAR_DEPENDENCY_DETECTION 1

// Output configuration
#define DEFAULT_OUTPUT_FORMAT OUTPUT_FORMAT_TERMINAL
#define ENABLE_COLOR_OUTPUT 1
#define MAX_OUTPUT_DEPTH 20
#define MAX_GRAPH_NODES 10000

// Debugging and logging
#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO
#define ENABLE_PERFORMANCE_METRICS 1
#define ENABLE_DEBUG_OUTPUT 0

// Global configuration structure
typedef struct GlobalConfig {
    // Performance settings
    bool enable_parallel_processing;
    int thread_count;
    size_t memory_pool_size;
    size_t file_cache_size;
    bool enable_incremental_analysis;
    
    // Analysis settings
    int max_analysis_depth;
    bool follow_external_dependencies;
    bool detect_circular_dependencies;
    size_t max_file_size;
    
    // Output settings
    int output_format;
    bool enable_colors;
    int max_output_depth;
    bool show_statistics;
    
    // Cache settings
    bool enable_file_cache;
    int cache_timeout_seconds;
    bool enable_cache_compression;
    bool enable_memory_mapping;
    
    // Debugging
    int log_level;
    bool enable_performance_metrics;
    bool enable_debug_output;
    
    // Paths
    char* state_file_path;
    char* cache_directory;
    char* output_file_path;
} GlobalConfig;

// Default configuration
extern const GlobalConfig DEFAULT_CONFIG;

// Configuration functions
GlobalConfig* config_create_default(void);
GlobalConfig* config_load_from_file(const char* config_file);
GlobalConfig* config_load_from_env(void);
int config_save_to_file(const GlobalConfig* config, const char* config_file);
void config_destroy(GlobalConfig* config);

// Configuration validation
bool config_validate(const GlobalConfig* config);
void config_print_debug(const GlobalConfig* config);

// Helper functions
int config_get_optimal_thread_count(void);
size_t config_get_system_memory_size(void);
size_t config_get_available_memory(void);

#endif // CONFIG_H