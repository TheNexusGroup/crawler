#include "crawler.h"
#include "syntax_map.h"
#include "syntaxes.h"
#include "pattern_cache.h"
#include "analyzers.h"
#include <dirent.h>
#include <sys/stat.h>
#include "logger.h"

DependencyCrawler* create_crawler(char** directories, int directory_count, AnalysisConfig* config) {
    logr(INFO, "[Crawler] Creating new crawler instance with %d directories", directory_count);
    
    DependencyCrawler* crawler = malloc(sizeof(DependencyCrawler));
    if (!crawler) {
        logr(ERROR, "[Crawler] Failed to allocate memory for crawler");
        return NULL;
    }
    
    // Initialize patterns
    logr(DEBUG, "[Crawler] Initializing crawler patterns");
    if (!initPatternCache()) {
        logr(ERROR, "[Crawler] Failed to initialize pattern cache");
        free(crawler);
        return NULL;
    }
    
    crawler->root_directories = malloc(sizeof(char*) * directory_count);
    if (!crawler->root_directories) {
        logr(ERROR, "[Crawler] Failed to allocate memory for root directories");
        free(crawler);
        return NULL;
    }
    crawler->directory_count = directory_count;
    
    // Copy configuration if provided
    if (config) {
        crawler->analysis_config = *config;
    } else {
        // Default configuration
        crawler->analysis_config.analyze_modules = 1;
        crawler->analysis_config.analyze_structures = 1;
        crawler->analysis_config.analyze_methods = 1;
        crawler->analysis_config.max_depth = -1;
        crawler->analysis_config.follow_external = 0;
    }
    
    for (int i = 0; i < directory_count; i++) {
        if (!directories[i]) {
            logr(ERROR, "[Crawler] NULL directory at index %d", i);
            free_crawler(crawler);
            return NULL;
        }
        crawler->root_directories[i] = strdup(directories[i]);
        if (!crawler->root_directories[i]) {
            logr(ERROR, "[Crawler] Failed to duplicate directory path at index %d", i);
            free_crawler(crawler);
            return NULL;
        }
        logr(DEBUG, "[Crawler] Added directory: %s", crawler->root_directories[i]);
    }
    
    crawler->parsers = NULL;
    crawler->parser_count = 0;
    crawler->dependency_graph = NULL;
    crawler->result_graph = NULL;
    
    logr(DEBUG, "[Crawler] Crawler instance created successfully");
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
                        const char* content, const LanguageGrammar* grammar) {
    if (!crawler || !filepath || !content || !grammar) {
        logr(ERROR, "[Crawler] Invalid parameters passed to processLayer");
        return;
    }
    
    logr(DEBUG, "[Crawler] Processing layer for file: %s", filepath);
    
    // Analyze based on configuration
    if (crawler->analysis_config.analyze_modules) {
        logr(DEBUG, "[Crawler] Analyzing modules for file: %s", filepath);
        ExtractedDependency* deps = analyze_module(content, grammar);
        ExtractedDependency* current_dep = deps;
        
        while (current_dep) {
            logr(DEBUG, "[Crawler] Processing module dependency: %s", 
                 current_dep->target ? current_dep->target : "NULL");
            
            ExtractedDependency* temp_dep = malloc(sizeof(ExtractedDependency));
            if (!temp_dep) {
                logr(ERROR, "[Crawler] Failed to allocate memory for temporary dependency");
                free_extracted_dependency(deps);
                return;
            }
            memset(temp_dep, 0, sizeof(ExtractedDependency));
            temp_dep->file_path = strdup(filepath);
            temp_dep->target = current_dep->target ? strdup(current_dep->target) : NULL;
            temp_dep->layer = LAYER_MODULE;
            
            logr(DEBUG, "[Crawler] Adding module dependency to graph: %s -> %s", 
                 temp_dep->file_path, temp_dep->target ? temp_dep->target : "NULL");
            
            if (!crawler->dependency_graph) {
                crawler->dependency_graph = create_dependency_from_extracted(temp_dep);
                logr(DEBUG, "[Crawler] Created new dependency graph");
            } else {
                add_to_dependency_graph(crawler->dependency_graph, temp_dep);
                logr(DEBUG, "[Crawler] Added to existing dependency graph");
            }
            
            free_extracted_dependency(temp_dep);
            current_dep = current_dep->next;
        }
        free_extracted_dependency(deps);
    }

    if (crawler->analysis_config.analyze_structures) {
        logr(DEBUG, "[Crawler] Analyzing structures in %s", filepath);
        Structure* structs = analyze_structure(content, filepath, grammar);
        if (structs) {
            logr(DEBUG, "[Crawler] Found structure dependencies");
            ExtractedDependency* struct_dep = malloc(sizeof(ExtractedDependency));
            if (!struct_dep) {
                logr(ERROR, "[Crawler] Failed to allocate memory for structure dependency");
                free_structures(structs);
                return;
            }
            memset(struct_dep, 0, sizeof(ExtractedDependency));
            struct_dep->file_path = strdup(filepath);
            struct_dep->structures = structs;
            struct_dep->layer = LAYER_STRUCT;
            
            // Track type source locations
            Structure* curr_struct = structs;
            while (curr_struct) {
                logr(DEBUG, "[Crawler] Processing structure: %s", 
                     curr_struct->name ? curr_struct->name : "NULL");
                
                // Check for type dependencies in includes/imports
                ExtractedDependency* type_deps = analyze_module(content, grammar);
                ExtractedDependency* curr_dep = type_deps;
                
                while (curr_dep) {
                    logr(DEBUG, "[Crawler] Checking type dependency: %s", 
                         curr_dep->target ? curr_dep->target : "NULL");
                    
                    // If this include/import contains our dependent type
                    if (curr_dep->target && curr_struct->dependencies && 
                        strstr(curr_struct->dependencies, curr_dep->target)) {
                        logr(DEBUG, "[Crawler] Found type source: %s in %s", 
                             curr_struct->dependencies, curr_dep->file_path);
                        
                        // Add source file information to the dependency
                        size_t new_len = (curr_dep->file_path ? strlen(curr_dep->file_path) : 0) + 
                                       (curr_struct->dependencies ? strlen(curr_struct->dependencies) : 0) + 2;
                        char* full_dep = malloc(new_len);
                        
                        if (full_dep) {
                            snprintf(full_dep, new_len, "%s:%s", 
                                   curr_dep->file_path ? curr_dep->file_path : "", 
                                   curr_struct->dependencies ? curr_struct->dependencies : "");
                            free(curr_struct->dependencies);
                            curr_struct->dependencies = full_dep;
                            logr(DEBUG, "[Crawler] Updated dependency info: %s", full_dep);
                        } else {
                            logr(ERROR, "[Crawler] Failed to allocate memory for full dependency");
                        }
                    }
                    curr_dep = curr_dep->next;
                }
                free_extracted_dependency(type_deps);
                curr_struct = curr_struct->next;
            }
            
            logr(DEBUG, "[Crawler] Adding structure dependencies to graph");
            if (!crawler->dependency_graph) {
                crawler->dependency_graph = create_dependency_from_extracted(struct_dep);
                logr(DEBUG, "[Crawler] Created new dependency graph for structures");
            } else {
                add_to_dependency_graph(crawler->dependency_graph, struct_dep);
                logr(DEBUG, "[Crawler] Added structures to existing dependency graph");
            }
            
            free_extracted_dependency(struct_dep);
        }
    }

    if (crawler->analysis_config.analyze_methods) {
        Method* methods = analyze_method(content, grammar);
        if (methods) {
            ExtractedDependency* method_dep = malloc(sizeof(ExtractedDependency));
            memset(method_dep, 0, sizeof(ExtractedDependency));
            method_dep->file_path = strdup(filepath);
            method_dep->methods = methods;
            method_dep->layer = LAYER_METHOD;
            
            if (!crawler->dependency_graph) {
                crawler->dependency_graph = create_dependency_from_extracted(method_dep);
            } else {
                add_to_dependency_graph(crawler->dependency_graph, method_dep);
            }
            free_extracted_dependency(method_dep);
        }
    }
    
    logr(DEBUG, "[Crawler] Finished processing layer for file: %s", filepath);
}

// Process a single file
static int process_file(DependencyCrawler* crawler, const char* filepath) {
    if (!crawler || !filepath) {
        logr(ERROR, "[Crawler] Invalid parameters passed to process_file");
        return 0;
    }

    logr(DEBUG, "[Crawler] Starting to process file: %s", filepath);
    
    // Get language type from syntaxes.h
    LanguageType lang = languageType(filepath);
    if (lang == (LanguageType)-1) {
        logr(DEBUG, "[Crawler] Unsupported file type: %s", filepath);
        return 0;
    }
    logr(DEBUG, "[Crawler] Detected language type: %d for file: %s", lang, filepath);
    
    // Get grammar for language
    const LanguageGrammar* grammar = languageGrammars(lang);
    if (!grammar) {
        logr(ERROR, "[Crawler] Failed to get grammar for language type %d", lang);
        return 0;
    }
    logr(DEBUG, "[Crawler] Found grammar for language type %d", lang);
    
    // Read file content
    FILE* file = fopen(filepath, "r");
    if (!file) {
        logr(ERROR, "[Crawler] Failed to open file: %s", filepath);
        return 0;
    }
    logr(DEBUG, "[Crawler] Successfully opened file: %s", filepath);
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    
    // Allocate memory for content
    char* content = malloc(size + 1);
    if (!content) {
        logr(ERROR, "[Crawler] Failed to allocate memory for file content");
        fclose(file);
        return 0;
    }
    
    // Read content
    logr(DEBUG, "[Crawler] Reading file content for: %s", filepath);
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    fclose(file);
    
    logr(DEBUG, "[Crawler] Successfully read file content, processing with grammar");
    
    // Process with grammar and set language type
    ExtractedDependency* deps = analyze_module(content, grammar);
    if (deps) {
        deps->language = lang;  // Set the language type
    }
    
    processLayer(crawler, filepath, content, grammar);
    
    free(content);
    logr(DEBUG, "[Crawler] Finished processing file: %s", filepath);
    return 1;
}

// Recursively crawl directories
static void crawl_directory(DependencyCrawler* crawler, const char* dir_path) {
    if (!crawler || !dir_path) {
        logr(ERROR, "[Crawler] Invalid parameters in crawl_directory");
        return;
    }

    logr(DEBUG, "[Crawler] Opening directory: %s", dir_path);
    DIR* dir = opendir(dir_path);
    if (!dir) {
        logr(ERROR, "[Crawler] Failed to open directory: %s", dir_path);
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;  // Skip hidden files/directories
        
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
        logr(DEBUG, "[Crawler] Processing entry: %s", path);
        
        struct stat statbuf;
        if (stat(path, &statbuf) != 0) {
            logr(ERROR, "[Crawler] Failed to stat file: %s", path);
            continue;
        }
        
        if (S_ISDIR(statbuf.st_mode)) {
            logr(DEBUG, "[Crawler] Found directory: %s", path);
            crawl_directory(crawler, path);
        } else {
            logr(DEBUG, "[Crawler] Found file: %s", path);
            if (!process_file(crawler, path)) {
                logr(ERROR, "[Crawler] Failed to process file: %s", path);
            }
        }
    }
    
    closedir(dir);
    logr(DEBUG, "[Crawler] Finished processing directory: %s", dir_path);
}

// Main crawling function
void crawl_dependencies(DependencyCrawler* crawler) {
    logr(DEBUG, "[Crawler] Starting dependency crawl for %d directories", crawler->directory_count);
    if (!crawler || !crawler->root_directories) {
        logr(ERROR, "[Crawler] Invalid crawler instance");
        return;
    }
    
    for (int i = 0; i < crawler->directory_count; i++) {
        if (!crawler->root_directories[i]) {
            logr(ERROR, "[Crawler] Invalid directory at index %d", i);
            continue;
        }
        logr(DEBUG, "[Crawler] Processing directory %d: %s", i, crawler->root_directories[i]);
        crawl_directory(crawler, crawler->root_directories[i]);
    }
}

// Print dependency information
void print_dependencies(DependencyCrawler* crawler) {
    if (!crawler->dependency_graph) {
        logr(INFO, "[Crawler] No dependencies found.");
        return;
    }

    logr(INFO, "[Crawler] Dependencies by Layer");
    logr(INFO, "==========================\n");
    
    // Module Dependencies
    logr(INFO, "Module Dependencies:");
    logr(INFO, "-----------------");
    Dependency* current = crawler->dependency_graph;
    int module_count = 0;
    char* current_file = NULL;
    Dependency* dep = crawler->dependency_graph;
    while (dep) {
        if (!current_file || strcmp(current_file, dep->source) != 0) {
            if (current_file) free(current_file);
            current_file = strdup(dep->source);
            logr(INFO, "  %s", current_file);
        }
        
        // Check if this is the last dependency for this source file
        Dependency* next_with_same_source = dep->next;
        while (next_with_same_source && strcmp(next_with_same_source->source, current_file) != 0) {
            next_with_same_source = next_with_same_source->next;
        }
        
        // Use └── for last item, ├── for others
        const char* prefix = next_with_same_source ? "├──" : "└──";
        logr(INFO, "    %s %s", prefix, dep->target);
        module_count++;
        
        dep = dep->next;
    }
    logr(INFO, "Total Module Dependencies: %d\n", module_count);
    free(current_file);
    
    // Structure Dependencies
    logr(INFO, "Structure Dependencies:");
    logr(INFO, "--------------------");
    size_t def_count;
    size_t struct_count = 0;
    StructureDefinition* defs = get_structure_definitions(&def_count);
    
    for (size_t i = 0; i < def_count; i++) {
        StructureDefinition* def = &defs[i];
        logr(INFO, "  %s (defined in %s)", def->name, def->defined_in);
        if (def->reference_count > 0) {
            logr(INFO, "    Referenced in:");
            for (int j = 0; j < def->reference_count; j++) {
                logr(INFO, "      └── %s", def->referenced_in[j]);
            }
            struct_count++;
        }
    }
    logr(INFO, "Total Structure Dependencies: %zu\n", struct_count);

    // Method Dependencies
    logr(INFO, "Method Dependencies:");
    logr(INFO, "-----------------");
    current = crawler->dependency_graph;
    int method_count = 0;
    while (current) {
        if (current->level == LAYER_METHOD) {
            method_count++;
            logr(INFO, "  %s", current->source);
            if (current->methods) {
                Method* method = current->methods;
                while (method) {
                    logr(INFO, "    └── %s()", method->name);
                    if (method->dependencies) {
                        logr(INFO, "        └── calls: %s", method->dependencies);
                    }
                    method = method->next;
                }
            }
        }
        current = current->next;
    }
    logr(INFO, "Total Method Dependencies: %d\n", method_count);

    logr(INFO, "\nTotal Dependencies: %zu", module_count + struct_count + method_count);
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
        print_dependencies(crawler);  // Default to terminal output
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
        
        // Free source and target
        free(current->source);
        free(current->target);
        
        // Free methods
        if (current->methods) {
            for (int i = 0; i < current->method_count; i++) {
                free(current->methods[i].name);
                free(current->methods[i].return_type);
                free(current->methods[i].dependencies);
                
                // Free parameters
                for (int j = 0; j < current->methods[i].param_count; j++) {
                    free(current->methods[i].parameters[j].name);
                    free(current->methods[i].parameters[j].type);
                    free(current->methods[i].parameters[j].default_value);
                }
            }
            free(current->methods);
        }
        
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