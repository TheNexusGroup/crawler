#include "crawler.h"
#include "syntax_map.h"
#include "syntaxes.h"
#include <dirent.h>
#include <sys/stat.h>
#include "logger.h"

// Initialize pattern cache at startup
static void init_crawler_patterns(void) {
    logr(DEBUG, "[Crawler] Initializing crawler patterns");
    if (initPatternCache() != 0) {
        logr(ERROR, "[Crawler] Failed to initialize pattern cache");
        exit(1);
    }
    logr(DEBUG, "[Crawler] Pattern cache initialized successfully");
}

// Helper function to check if a file has a specific extension
static int has_extension(const char* filename, const char* ext) {
    const char* dot = strrchr(filename, '.');
    return dot && !strcmp(dot + 1, ext);
}

// Helper function to determine language type from file extension
static LanguageType languageType(const char* filename) {
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
                                   AnalysisLayer level, LanguageType lang) {
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
    logr(INFO, "[Crawler] Creating new crawler instance with %d directories", dir_count);
    init_crawler_patterns();
    
    DependencyCrawler* crawler = (DependencyCrawler*)malloc(sizeof(DependencyCrawler));
    if (!crawler) {
        logr(ERROR, "[Crawler] Failed to allocate memory for crawler");
        return NULL;
    }
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
    
    logr(DEBUG, "[Crawler] instance created successfully");
    return crawler;
}

// Register a language-specific parser
void register_language_parser(DependencyCrawler* crawler, LanguageType type,
                            const LanguageParser* parser) {
    logr(DEBUG, "[Crawler] Registering parser for language type %d", type);
    crawler->parser_count++;
    crawler->parsers = (LanguageParser*)realloc(crawler->parsers, 
                                               sizeof(LanguageParser) * crawler->parser_count);
    
    crawler->parsers[crawler->parser_count - 1].type = type;
    crawler->parsers[crawler->parser_count - 1].analyze_module = parser->analyze_module;
    crawler->parsers[crawler->parser_count - 1].analyze_structure = parser->analyze_structure;
    crawler->parsers[crawler->parser_count - 1].analyze_method = parser->analyze_method;
}

// Process file content based on analysis layer
static void processLayer(DependencyCrawler* crawler, const char* filepath,
                         const char* content, AnalysisLayer layer) {
    LanguageType lang = languageType(filepath);
    if (lang < 0) {
        logr(DEBUG, "[Crawler] Unsupported file type: %s", filepath);
        return;
    }
    
    logr(DEBUG, "Processing %s layer for file: %s", 
                layer == LAYER_MODULE ? "module" :
                layer == LAYER_STRUCT ? "structure" : "method",
                filepath);
    
    // Find the appropriate parser
    LanguageParser* parser = NULL;
    for (int i = 0; i < crawler->parser_count; i++) {
        if (crawler->parsers[i].type == lang) {
            parser = &crawler->parsers[i];
            break;
        }
    }

    if (!parser) return;

    switch (layer) {
        case LAYER_MODULE:
            if (parser->analyze_module) {
                ExtractedDependency* deps = parser->analyze_module(content);
                if (deps) {
                    ExtractedDependency* current = deps;
                    while (current) {
                        if (current->target) {
                            Dependency* new_dep = create_dependency(
                                filepath,
                                current->target,
                                LAYER_MODULE,
                                lang
                            );
                            new_dep->next = crawler->dependency_graph;
                            crawler->dependency_graph = new_dep;
                            logr(DEBUG, "[Crawler] Added module dependency: %s -> %s", 
                                   filepath, current->target);
                        }
                        ExtractedDependency* next = current->next;
                        current = next;
                    }
                    free_dependency(deps);
                }
            }
            break;

        case LAYER_STRUCT:
            if (parser->analyze_structure) {
                Structure* structs = parser->analyze_structure(content);
                if (structs) {
                    Structure* current = structs;
                    while (current) {
                        if (current->dependencies) {
                            Dependency* new_dep = create_dependency(
                                filepath,
                                current->dependencies,
                                LAYER_STRUCT,
                                lang
                            );
                            new_dep->next = crawler->dependency_graph;
                            crawler->dependency_graph = new_dep;
                            logr(DEBUG, "[Crawler] Added structure dependency: %s -> %s", 
                                   filepath, current->dependencies);
                        }
                        Structure* next = current->next;
                        current = next;
                    }
                    free_structures(structs);
                }
            }
            break;

        case LAYER_METHOD:
            if (parser->analyze_method) {
                Method* methods = parser->analyze_method(content);
                if (methods) {
                    Method* current = methods;
                    while (current) {
                        if (current->dependencies) {
                            Dependency* new_dep = create_dependency(
                                filepath,
                                current->dependencies,
                                LAYER_METHOD,
                                lang
                            );
                            new_dep->next = crawler->dependency_graph;
                            crawler->dependency_graph = new_dep;
                            logr(DEBUG, "[Crawler] Added method dependency: %s -> %s", 
                                   filepath, current->dependencies);
                        }
                        Method* next = current->next;
                        current = next;
                    }
                    free_methods(methods);
                }
            }
            break;
    }
}

// Process a single file
static void process_file(DependencyCrawler* crawler, const char* filepath) {
    logr(VERBOSE, "[Crawler] Processing file: %s", filepath);
    
    FILE* file = fopen(filepath, "r");
    if (!file) {
        logr(WARN, "[Crawler] Failed to open file: %s", filepath);
        return;
    }

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
        processLayer(crawler, filepath, content, LAYER_MODULE);
    }
    if (crawler->analysis_config.analyze_structures) {
        processLayer(crawler, filepath, content, LAYER_STRUCT);
    }
    if (crawler->analysis_config.analyze_methods) {
        processLayer(crawler, filepath, content, LAYER_METHOD);
    }

    free(content);
    logr(INFO, "[Crawler] Finished processing file: %s", filepath);
}

// Recursively crawl directories
static void crawl_directory(DependencyCrawler* crawler, const char* dir_path) {
    logr(VERBOSE, "[Crawler] Crawling directory: %s", dir_path);
    DIR* dir = opendir(dir_path);
    if (!dir) {
        logr(ERROR, "[Crawler] Failed to open directory: %s", dir_path);
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        logr(VERBOSE, "[Crawler] Crawling entry: %s", entry->d_name);
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
    logr(INFO, "[Crawler] Finished crawling directory: %s", dir_path);
}

// Main crawling function
void crawl_dependencies(DependencyCrawler* crawler) {
    for (int i = 0; i < crawler->directory_count; i++) {
        crawl_directory(crawler, crawler->root_directories[i]);
    }
}

// Print dependency information
void print_dependencies(DependencyCrawler* crawler, int verbosity) {
    if (!crawler->dependency_graph) {
        logr(INFO, "[Crawler] No dependencies found.");
        return;
    }

    logr(INFO, "[Crawler] Dependencies found.");
    logr(INFO, "==================\n");
    
    Dependency* current = crawler->dependency_graph;
    int count = 0;
    while (current) {
        count++;
        if (verbosity > 0) {
            logr(INFO, "[Crawler] %s -> %s [Level: %s, Language: %s]", 
                   current->source, 
                   current->target,
                   current->level == LAYER_MODULE ? "Module" :
                   current->level == LAYER_STRUCT ? "Structure" : "Method",
                   languageName(current->language));
        } else {
            logr(INFO, "[Crawler] %s -> %s", current->source, current->target);
        }
        current = current->next;
    }
    logr(INFO, "[Crawler] Total dependencies found: %d", count);
}

// Export dependencies in various formats
void export_dependencies(DependencyCrawler* crawler, const char* output_format) {
    if (strcmp(output_format, "json") == 0) {
        // TODO: Implement JSON export
        logr(INFO, "[Crawler] JSON export not yet implemented");
    } else if (strcmp(output_format, "graphviz") == 0) {
        // TODO: Implement GraphViz export
        logr(INFO, "[Crawler] GraphViz export not yet implemented");
    } else {
        print_dependencies(crawler, 0);  // Default to terminal output
    }
}

// Clean up resources
void free_crawler(DependencyCrawler* crawler) {
    logr(VERBOSE, "[Crawler] Cleaning up crawler resources");
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
    logr(VERBOSE, "[Crawler] cleanup complete");
}