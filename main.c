#include "src/crawler.h"
#include "src/syntaxes.h"
#include "src/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

// Default configuration values
#define DEFAULT_DEPTH -1
#define DEFAULT_OUTPUT_FORMAT "terminal"

// Command line options structure
typedef struct {
    char** directories;
    int dir_count;
    char** library_dirs;
    int lib_count;
    int depth;
    char* output_format;
    int verbose;
} CrawlerOptions;

// Function to print usage information
static void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS] [ENTRY_POINT]\n\n", program_name);
    printf("Options:\n");
    printf("  -l, --library DIR     Specify additional library directory to search for dependencies\n");
    printf("  -d, --depth NUM       Set maximum crawl depth (default: unlimited)\n");
    printf("  -o, --output FORMAT   Output format (terminal, json, graphviz)\n");
    printf("  -v, --verbose         Enable verbose output\n");
    printf("  --help                Show this help message\n");
}

// Function to parse command line arguments
static CrawlerOptions parse_arguments(int argc, char* argv[]) {
    CrawlerOptions options = {
        .directories = malloc(sizeof(char*)),
        .dir_count = 0,
        .library_dirs = malloc(sizeof(char*)),
        .lib_count = 0,
        .depth = DEFAULT_DEPTH,
        .output_format = strdup(DEFAULT_OUTPUT_FORMAT),
        .verbose = 0
    };

    static struct option long_options[] = {
        {"library", required_argument, 0, 'l'},
        {"depth", required_argument, 0, 'd'},
        {"output", required_argument, 0, 'o'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "l:d:o:vh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'l':
                options.library_dirs = realloc(options.library_dirs, 
                                            (options.lib_count + 1) * sizeof(char*));
                options.library_dirs[options.lib_count++] = strdup(optarg);
                break;

            case 'd':
                options.depth = atoi(optarg);
                break;

            case 'o':
                free(options.output_format);
                options.output_format = strdup(optarg);
                break;

            case 'v':
                options.verbose = 1;
                break;

            case 'h':
                print_usage(argv[0]);
                exit(0);

            default:
                print_usage(argv[0]);
                exit(1);
        }
    }

    // Handle non-option arguments (entry points)
    for (int i = optind; i < argc; i++) {
        options.directories = realloc(options.directories, 
                                    (options.dir_count + 1) * sizeof(char*));
        options.directories[options.dir_count++] = strdup(argv[i]);
    }

    // If no directory specified, use current directory
    if (options.dir_count == 0) {
        options.directories[0] = strdup(".");
        options.dir_count = 1;
    }

    return options;
}

// Function to free options resources
static void free_options(CrawlerOptions* options) {
    for (int i = 0; i < options->dir_count; i++) {
        free(options->directories[i]);
    }
    free(options->directories);

    for (int i = 0; i < options->lib_count; i++) {
        free(options->library_dirs[i]);
    }
    free(options->library_dirs);
    
    free(options->output_format);
}

int main(int argc, char* argv[]) {
    // setLogLevel(INFO);
    printf("Starting dependency crawler...\n");
    
    // Parse command line arguments
    CrawlerOptions options = parse_arguments(argc, argv);
    printf("Parsed command line arguments\n");
    printf("Analyzing directories:\n");
    for (int i = 0; i < options.dir_count; i++) {
        printf("  - %s\n", options.directories[i]);
    }

    // Create analysis configuration
    AnalysisConfig config = {
        .analyzeModules = 0,
        .analyze_structures = 0,
        .analyzeMethods = 1,
        .max_depth = options.depth,
        .follow_external = (options.lib_count > 0)
    };
    printf("Created analysis configuration\n");

    // Create crawler instance
    printf("Creating crawler instance...\n");
    DependencyCrawler* crawler = createCrawler(options.directories, 
                                              options.dir_count, 
                                              &config);
    if (!crawler) {
        fprintf(stderr, "Failed to create crawler\n");
        free_options(&options);
        return 1;
    }
    printf("Crawler instance created successfully\n");

    // Add library directories if specified
    for (int i = 0; i < options.lib_count; i++) {
        printf("Adding library directory: %s\n", options.library_dirs[i]);
    }

    // Perform dependency analysis
    printf("Starting dependency analysis...\n");
    crawlDeps(crawler);
    printf("Dependency analysis complete\n");

    // Export results
    printf("Exporting results...\n");
    if (options.verbose) {
        printDependencies(crawler);
    } else {
        exportDeps(crawler, options.output_format);
    }
    printf("Results exported\n");

    // Cleanup
    printf("Cleaning up...\n");
    freeCrawler(crawler);
    free_options(&options);
    printf("Done!\n");

    return 0;
}
