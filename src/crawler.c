#include "crawler.h"
#include "syntax_map.h"
#include <dirent.h>
#include <sys/stat.h>

// Initialize pattern cache at startup
static void init_crawler_patterns(void) {
    if (initPatternCache() != 0) {
        fprintf(stderr, "Failed to initialize pattern cache\n");
        exit(1);
    }
}

// Helper function to check if a file has a specific extension
static int has_extension(const char* filename, const char* ext) {
    const char* dot = strrchr(filename, '.');
    return dot && !strcmp(dot + 1, ext);
}

// Helper function to determine language type from file extension
static LanguageType get_language_type(const char* filename) {
    static const struct {
        const char* ext;
        LanguageType type;
    } extension_map[] = {
        {"rs", LANG_RUST},
        {"c", LANG_C},
        {"h", LANG_C}, 
        {"cpp", LANG_C},
        {"hpp", LANG_C},
        {"hxx", LANG_C},
        {"cxx", LANG_C},
        {"ts", LANG_JAVASCRIPT},
        {"js", LANG_JAVASCRIPT},
        {"go", LANG_GO},
        {"py", LANG_PYTHON}, 
        {"java", LANG_JAVA},
        {"php", LANG_PHP},
        {"rb", LANG_RUBY},
        {"svelte", LANG_SVELTE}
    };
    
    // Loop through the lookup table once
    for (size_t i = 0; i < sizeof(extension_map) / sizeof(extension_map[0]); i++) {
        if (has_extension(filename, extension_map[i].ext)) {
            return extension_map[i].type;
        }
    }

    return (LanguageType)-1;
}

// Create a new dependency node
static Dependency* create_dependency(const char* source, const char* target, 
                                   DependencyLevel level, LanguageType lang) {
    Dependency* dep = (Dependency*)malloc(sizeof(Dependency));
    dep->source = strdup(source);
    dep->target = strdup(target);
    dep->level = level;
    dep->language = lang;
    dep->next = NULL;
    return dep;
}

// Create a new crawler instance with analysis configuration
DependencyCrawler* create_crawler(char** dirs, int dir_count, AnalysisConfig* config) {
    init_crawler_patterns();
    
    DependencyCrawler* crawler = (DependencyCrawler*)malloc(sizeof(DependencyCrawler));
    crawler->root_directories = (char**)malloc(sizeof(char*) * dir_count);
    crawler->directory_count = dir_count;
    
    for (int i = 0; i < dir_count; i++) {
        crawler->root_directories[i] = strdup(dirs[i]);
    }
    
    crawler->parsers = NULL;
    crawler->parser_count = 0;
    crawler->dependency_graph = NULL;
    crawler->result_graph = NULL;
    
    if (config) {
        memcpy(&crawler->analysis_config, config, sizeof(AnalysisConfig));
    } else {
        // Default configuration
        crawler->analysis_config.analyze_modules = 1;
        crawler->analysis_config.analyze_structures = 1;
        crawler->analysis_config.analyze_methods = 1;
        crawler->analysis_config.max_depth = -1;
        crawler->analysis_config.follow_external = 0;
    }
    
    return crawler;
}

// Register a language-specific parser
void register_language_parser(DependencyCrawler* crawler, LanguageType type,
                            const LanguageParser* parser) {
    crawler->parser_count++;
    crawler->parsers = (LanguageParser*)realloc(crawler->parsers, 
                                               sizeof(LanguageParser) * crawler->parser_count);
    
    crawler->parsers[crawler->parser_count - 1].type = type;
    crawler->parsers[crawler->parser_count - 1].analyze_module = parser->analyze_module;
    crawler->parsers[crawler->parser_count - 1].analyze_structure = parser->analyze_structure;
    crawler->parsers[crawler->parser_count - 1].analyze_method = parser->analyze_method;
}

// Process file content based on analysis layer
static void process_layer(DependencyCrawler* crawler, const char* filepath,
                         const char* content, AnalysisLayer layer) {
    LanguageType lang = get_language_type(filepath);
    if (lang < 0) return;

    // Find the appropriate parser
    LanguageParser* parser = NULL;
    for (int i = 0; i < crawler->parser_count; i++) {
        if (crawler->parsers[i].type == lang) {
            parser = &crawler->parsers[i];
            break;
        }
    }

    if (!parser) return;

    ExtractedDependency* deps = NULL;
    switch (layer) {
        case LAYER_MODULE:
            if (parser->analyze_module) {
                deps = parser->analyze_module(content);
                if (deps) {
                    // Process module dependencies
                    // TODO: Add to dependency graph
                    free_dependency(deps);
                }
            }
            break;

        case LAYER_STRUCT:
            if (parser->analyze_structure) {
                Structure* structs = parser->analyze_structure(content);
                if (structs) {
                    // Process structure dependencies
                    // TODO: Add to dependency graph
                    // TODO: Implement structure cleanup
                    free(structs);
                }
            }
            break;

        case LAYER_METHOD:
            if (parser->analyze_method) {
                Method* methods = parser->analyze_method(content);
                if (methods) {
                    // Process method dependencies
                    // TODO: Add to dependency graph
                    // TODO: Implement method cleanup
                    free(methods);
                }
            }
            break;
    }
}

// Process a single file
static void process_file(DependencyCrawler* crawler, const char* filepath) {
    // Read file content
    FILE* file = fopen(filepath, "r");
    if (!file) return;

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    // Read content
    char* content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return;
    }

    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    fclose(file);

    // Process each layer according to configuration
    if (crawler->analysis_config.analyze_modules) {
        process_layer(crawler, filepath, content, LAYER_MODULE);
    }
    if (crawler->analysis_config.analyze_structures) {
        process_layer(crawler, filepath, content, LAYER_STRUCT);
    }
    if (crawler->analysis_config.analyze_methods) {
        process_layer(crawler, filepath, content, LAYER_METHOD);
    }

    free(content);
}

// Recursively crawl directories
static void crawl_directory(DependencyCrawler* crawler, const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;  // Skip hidden files/directories
        
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
        
        struct stat statbuf;
        if (stat(path, &statbuf) != 0) continue;
        
        if (S_ISDIR(statbuf.st_mode)) {
            crawl_directory(crawler, path);
        } else {
            process_file(crawler, path);
        }
    }
    
    closedir(dir);
}

// Main crawling function
void crawl_dependencies(DependencyCrawler* crawler) {
    for (int i = 0; i < crawler->directory_count; i++) {
        crawl_directory(crawler, crawler->root_directories[i]);
    }
}

// Print dependency information
void print_dependencies(DependencyCrawler* crawler, int verbosity) {
    Dependency* current = crawler->dependency_graph;
    while (current) {
        if (verbosity > 0) {
            printf("%s -> %s [%d, %d]\n", 
                   current->source, current->target, 
                   current->level, current->language);
        } else {
            printf("%s -> %s\n", current->source, current->target);
        }
        current = current->next;
    }
}

// Export dependencies in various formats
void export_dependencies(DependencyCrawler* crawler, const char* output_format) {
    if (strcmp(output_format, "json") == 0) {
        // TODO: Implement JSON export
        printf("JSON export not yet implemented\n");
    } else if (strcmp(output_format, "graphviz") == 0) {
        // TODO: Implement GraphViz export
        printf("GraphViz export not yet implemented\n");
    } else {
        print_dependencies(crawler, 0);  // Default to terminal output
    }
}

// Clean up resources
void free_crawler(DependencyCrawler* crawler) {
    cleanPatternCache();
    
    for (int i = 0; i < crawler->directory_count; i++) {
        free(crawler->root_directories[i]);
    }
    free(crawler->root_directories);
    free(crawler->parsers);
    
    // Free dependency graph
    Dependency* current = crawler->dependency_graph;
    while (current) {
        Dependency* next = current->next;
        free(current->source);
        free(current->target);
        free(current);
        current = next;
    }
    
    if (crawler->result_graph) {
        // Free relationship graph
        for (int i = 0; i < crawler->result_graph->relationship_count; i++) {
            free(crawler->result_graph->relationships[i]);
        }
        free(crawler->result_graph->relationships);
        free(crawler->result_graph);
    }
    
    free(crawler);
}
