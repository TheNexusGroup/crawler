#include "src/crawler.h"
#include "src/syntaxes.h"
#include "src/logger.h"
#include "src/config.h"
#include "src/memory_pool.h"
#include "src/parallel_processor.h"
#include "src/incremental_analyzer.h"
#include "src/output_formatter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>

// Performance timing utilities
struct timeval start_time, end_time;

void start_timer(void) {
    gettimeofday(&start_time, NULL);
}

double end_timer(void) {
    gettimeofday(&end_time, NULL);
    return (end_time.tv_sec - start_time.tv_sec) + 
           (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
}

// Enhanced command line options structure
typedef struct {
    char** directories;
    int dir_count;
    char** library_dirs;
    int lib_count;
    int depth;
    char* output_format;
    char* output_file;
    int verbose;
    int thread_count;
    bool enable_parallel;
    bool enable_incremental;
    bool enable_caching;
    size_t memory_pool_size;
    LayerFilter layer_filter;
    char* config_file;
} EnhancedOptions;

// Function to print enhanced usage information
static void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS] [ENTRY_POINT]\n\n", program_name);
    printf("Analysis Options:\n");
    printf("  -d, --depth NUM           Set maximum crawl depth (default: unlimited)\n");
    printf("  -l, --library DIR         Specify additional library directory\n");
    printf("  --layer LAYER             Filter by layer (module|struct|method|all)\n");
    printf("  --incremental             Enable incremental analysis\n");
    printf("\n");
    printf("Performance Options:\n");
    printf("  -j, --threads NUM         Number of worker threads (default: %d)\n", DEFAULT_THREAD_COUNT);
    printf("  --parallel                Enable parallel processing\n");
    printf("  --cache                   Enable file caching\n");
    printf("  --memory-pool SIZE        Memory pool size in MB (default: %d)\n", 
           DEFAULT_MEMORY_POOL_SIZE / (1024 * 1024));
    printf("\n");
    printf("Output Options:\n");
    printf("  -o, --output FORMAT       Output format (terminal|json|graphviz|html|mermaid)\n");
    printf("  -f, --file FILE           Output to file instead of stdout\n");
    printf("  --stats                   Show performance statistics\n");
    printf("\n");
    printf("General Options:\n");
    printf("  -v, --verbose             Enable verbose output\n");
    printf("  -c, --config FILE         Load configuration from file\n");
    printf("  --help                    Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --parallel -j 8 ./src                    # Parallel analysis with 8 threads\n", program_name);
    printf("  %s --incremental --cache ./project          # Incremental analysis with caching\n", program_name);
    printf("  %s -o graphviz -f deps.dot ./src           # Generate GraphViz output\n", program_name);
    printf("  %s --layer module --stats ./large_project  # Module-level analysis with stats\n", program_name);
}

// Function to parse layer filter from string
static LayerFilter parse_layer_filter(const char* layer_str) {
    if (strcmp(layer_str, "module") == 0) return FILTER_MODULE;
    if (strcmp(layer_str, "struct") == 0) return FILTER_STRUCT;
    if (strcmp(layer_str, "method") == 0) return FILTER_METHOD;
    if (strcmp(layer_str, "all") == 0) return FILTER_ALL;
    
    logr(WARN, "Unknown layer filter '%s', using default", layer_str);
    return FILTER_ALL;
}

// Function to parse command line arguments with enhanced options
static EnhancedOptions parse_enhanced_arguments(int argc, char* argv[]) {
    EnhancedOptions options = {
        .directories = malloc(sizeof(char*)),
        .dir_count = 0,
        .library_dirs = malloc(sizeof(char*)),
        .lib_count = 0,
        .depth = -1,
        .output_format = strdup("terminal"),
        .output_file = NULL,
        .verbose = 0,
        .thread_count = DEFAULT_THREAD_COUNT,
        .enable_parallel = false,
        .enable_incremental = false,
        .enable_caching = false,
        .memory_pool_size = DEFAULT_MEMORY_POOL_SIZE,
        .layer_filter = FILTER_ALL,
        .config_file = NULL
    };

    static struct option long_options[] = {
        {"library", required_argument, 0, 'l'},
        {"depth", required_argument, 0, 'd'},
        {"output", required_argument, 0, 'o'},
        {"file", required_argument, 0, 'f'},
        {"verbose", no_argument, 0, 'v'},
        {"threads", required_argument, 0, 'j'},
        {"config", required_argument, 0, 'c'},
        {"parallel", no_argument, 0, 1000},
        {"incremental", no_argument, 0, 1001},
        {"cache", no_argument, 0, 1002},
        {"memory-pool", required_argument, 0, 1003},
        {"layer", required_argument, 0, 1004},
        {"stats", no_argument, 0, 1005},
        {"help", no_argument, 0, 1006},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "l:d:o:f:vj:c:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'l':
                options.library_dirs = realloc(options.library_dirs, 
                                             (options.lib_count + 1) * sizeof(char*));
                options.library_dirs[options.lib_count++] = strdup(optarg);
                break;
                
            case 'd':
                options.depth = atoi(optarg);
                if (options.depth < 0) {
                    fprintf(stderr, "Error: Invalid depth value\n");
                    exit(1);
                }
                break;
                
            case 'o':
                free(options.output_format);
                options.output_format = strdup(optarg);
                break;
                
            case 'f':
                options.output_file = strdup(optarg);
                break;
                
            case 'v':
                options.verbose = 1;
                break;
                
            case 'j':
                options.thread_count = atoi(optarg);
                if (options.thread_count < 1 || options.thread_count > MAX_THREAD_COUNT) {
                    fprintf(stderr, "Error: Thread count must be between 1 and %d\n", MAX_THREAD_COUNT);
                    exit(1);
                }
                break;
                
            case 'c':
                options.config_file = strdup(optarg);
                break;
                
            case 1000: // --parallel
                options.enable_parallel = true;
                break;
                
            case 1001: // --incremental
                options.enable_incremental = true;
                break;
                
            case 1002: // --cache
                options.enable_caching = true;
                break;
                
            case 1003: // --memory-pool
                options.memory_pool_size = atoi(optarg) * 1024 * 1024; // Convert MB to bytes
                break;
                
            case 1004: // --layer
                options.layer_filter = parse_layer_filter(optarg);
                break;
                
            case 1005: // --stats
                // This will be handled in the main function
                break;
                
            case 1006: // --help
                print_usage(argv[0]);
                exit(0);
                
            case '?':
                fprintf(stderr, "Use --help for usage information\n");
                exit(1);
                
            default:
                break;
        }
    }

    // Process remaining arguments as entry points
    for (int i = optind; i < argc; i++) {
        options.directories = realloc(options.directories, 
                                    (options.dir_count + 1) * sizeof(char*));
        options.directories[options.dir_count++] = strdup(argv[i]);
    }

    // If no directories specified, use current directory
    if (options.dir_count == 0) {
        options.directories[0] = strdup(".");
        options.dir_count = 1;
    }

    return options;
}

// Enhanced crawler creation with optimizations
static DependencyCrawler* create_enhanced_crawler(const EnhancedOptions* options, GlobalConfig* config) {
    logr(INFO, "Creating enhanced crawler with optimizations");
    
    // Configure analysis settings
    AnalysisConfig analysis_config = {
        .analyzeModules = (options->layer_filter & FILTER_MODULE) ? 1 : 0,
        .analyze_structures = (options->layer_filter & FILTER_STRUCT) ? 1 : 0,
        .analyzeMethods = (options->layer_filter & FILTER_METHOD) ? 1 : 0,
        .max_depth = options->depth,
        .follow_external = 1
    };

    DependencyCrawler* crawler = createCrawler(options->directories, options->dir_count, &analysis_config);
    if (!crawler) {
        logr(ERROR, "Failed to create crawler");
        return NULL;
    }

    logr(INFO, "Enhanced crawler created successfully");
    return crawler;
}

// Main analysis function with all optimizations
static int run_enhanced_analysis(const EnhancedOptions* options, GlobalConfig* config) {
    logr(INFO, "Starting enhanced dependency analysis");
    start_timer();

    // Initialize optimized components
    MemoryPool* memory_pool = NULL;
    ParallelProcessor* parallel_processor = NULL;
    IncrementalContext* incremental_context = NULL;
    OutputFormatter* formatter = NULL;

    int result = 0;

    // Initialize memory pool if enabled
    if (options->enable_parallel || config->enable_parallel_processing) {
        memory_pool = memory_pool_create(options->memory_pool_size);
        if (!memory_pool) {
            logr(ERROR, "Failed to create memory pool");
            return 1;
        }
        logr(INFO, "Memory pool initialized with %zu bytes", options->memory_pool_size);
    }

    // Initialize parallel processor if enabled
    if (options->enable_parallel) {
        parallel_processor = parallel_processor_create(options->thread_count, options->memory_pool_size);
        if (!parallel_processor) {
            logr(ERROR, "Failed to create parallel processor");
            result = 1;
            goto cleanup;
        }
        
        parallel_processor_set_caching(parallel_processor, options->enable_caching);
        parallel_processor_set_memory_mapping(parallel_processor, true);
        
        logr(INFO, "Parallel processor initialized with %d threads", options->thread_count);
    }

    // Initialize incremental analyzer if enabled
    if (options->enable_incremental) {
        incremental_context = incremental_context_create(".crawler_state");
        if (!incremental_context) {
            logr(ERROR, "Failed to create incremental context");
            result = 1;
            goto cleanup;
        }
        
        incremental_set_dependency_caching(incremental_context, options->enable_caching);
        incremental_context_load_state(incremental_context);
        
        logr(INFO, "Incremental analysis initialized");
    }

    // Create output formatter
    OutputFormat output_format = OUTPUT_FORMAT_TERMINAL;
    if (strcmp(options->output_format, "json") == 0) {
        output_format = OUTPUT_FORMAT_JSON;
    } else if (strcmp(options->output_format, "graphviz") == 0) {
        output_format = OUTPUT_FORMAT_GRAPHVIZ;
    } else if (strcmp(options->output_format, "html") == 0) {
        output_format = OUTPUT_FORMAT_HTML;
    } else if (strcmp(options->output_format, "mermaid") == 0) {
        output_format = OUTPUT_FORMAT_MERMAID;
    }

    formatter = output_formatter_create(output_format, options->output_file);
    if (!formatter) {
        logr(ERROR, "Failed to create output formatter");
        result = 1;
        goto cleanup;
    }

    // Configure output filters
    FilterConfig filter_config = {
        .layer_filter = options->layer_filter,
        .connection_filter = CONNECTION_ALL,
        .include_patterns = NULL,
        .include_pattern_count = 0,
        .exclude_patterns = NULL,
        .exclude_pattern_count = 0
    };
    
    output_formatter_set_filters(formatter, &filter_config);

    // Create and configure crawler
    DependencyCrawler* crawler = create_enhanced_crawler(options, config);
    if (!crawler) {
        result = 1;
        goto cleanup;
    }

    // Run analysis based on configuration
    ExtractedDependency* dependencies = NULL;
    
    if (options->enable_parallel && parallel_processor) {
        logr(INFO, "Running parallel analysis");
        
        // Queue all directories for parallel processing
        for (int i = 0; i < options->dir_count; i++) {
            parallel_processor_queue_directory(parallel_processor, options->directories[i]);
        }
        
        parallel_processor_start(parallel_processor);
        parallel_processor_wait_completion(parallel_processor, -1); // Wait indefinitely
        
        // Collect results
        ProcessingResult* results = parallel_processor_get_all_results(parallel_processor);
        
        // Convert parallel results to dependencies (implementation needed)
        // dependencies = convert_parallel_results(results);
        
    } else if (options->enable_incremental && incremental_context) {
        logr(INFO, "Running incremental analysis");
        
        // Analyze each directory incrementally
        for (int i = 0; i < options->dir_count; i++) {
            ExtractedDependency* dir_deps = NULL;
            incremental_analyze_directory(incremental_context, options->directories[i], &dir_deps);
            
            // Merge dependencies (implementation needed)
            // merge_dependencies(&dependencies, dir_deps);
        }
        
        // Save incremental state
        incremental_context_save_state(incremental_context);
        
    } else {
        logr(INFO, "Running standard analysis");
        
        // Standard crawler analysis
        dependencies = crawlDependencies(crawler);
    }

    // Output results
    if (dependencies) {
        double analysis_time = end_timer();
        logr(INFO, "Analysis completed in %.2f seconds", analysis_time);
        
        output_formatter_write_dependencies(formatter, dependencies);
        
        if (options->verbose) {
            OutputStats stats = output_calculate_stats(dependencies);
            output_print_stats(&stats, output_format, stdout);
            
            if (memory_pool) {
                memory_pool_debug_print(memory_pool);
            }
            
            if (parallel_processor) {
                parallel_processor_debug_print(parallel_processor);
            }
            
            if (incremental_context) {
                incremental_debug_print(incremental_context);
            }
        }
        
        // Cleanup dependencies
        freeDependency(dependencies);
    } else {
        logr(ERROR, "Analysis failed to produce results");
        result = 1;
    }

    // Cleanup crawler
    freeCrawler(crawler);

cleanup:
    if (formatter) {
        output_formatter_destroy(formatter);
    }
    
    if (incremental_context) {
        incremental_context_destroy(incremental_context);
    }
    
    if (parallel_processor) {
        parallel_processor_destroy(parallel_processor);
    }
    
    if (memory_pool) {
        memory_pool_destroy(memory_pool);
    }

    return result;
}

// Main function with enhanced features
int main(int argc, char* argv[]) {
    // Parse enhanced command line arguments
    EnhancedOptions options = parse_enhanced_arguments(argc, argv);
    
    // Initialize logging
    LogLevel log_level = options.verbose ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO;
    if (loggerInitialize(log_level, NULL) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    logr(INFO, "Starting Enhanced Dependency Crawler v2.0");
    logr(INFO, "Analyzing %d directories with optimizations", options.dir_count);

    // Load configuration
    GlobalConfig* config = NULL;
    if (options.config_file) {
        config = config_load_from_file(options.config_file);
    } else {
        config = config_create_default();
    }

    if (!config) {
        logr(ERROR, "Failed to load configuration");
        return 1;
    }

    // Override config with command line options
    config->thread_count = options.thread_count;
    config->enable_parallel_processing = options.enable_parallel;
    config->memory_pool_size = options.memory_pool_size;
    config->enable_file_cache = options.enable_caching;

    if (!config_validate(config)) {
        logr(ERROR, "Invalid configuration");
        config_destroy(config);
        return 1;
    }

    // Run enhanced analysis
    int result = run_enhanced_analysis(&options, config);

    // Cleanup
    config_destroy(config);
    
    // Free options
    for (int i = 0; i < options.dir_count; i++) {
        free(options.directories[i]);
    }
    free(options.directories);
    
    for (int i = 0; i < options.lib_count; i++) {
        free(options.library_dirs[i]);
    }
    free(options.library_dirs);
    
    free(options.output_format);
    free(options.output_file);
    free(options.config_file);

    logr(INFO, "Enhanced Dependency Crawler completed with exit code %d", result);
    loggerShutdown();
    
    return result;
}