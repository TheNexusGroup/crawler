#include "crawler.h"
#include "syntax_map.h"
#include "syntaxes.h"
#include "pattern_cache.h"
#include "analyzers.h"
#include <dirent.h>
#include <sys/stat.h>
#include "logger.h"
#include <ctype.h>

// Add these after includes
static void processFile(DependencyCrawler* crawler, const char* file_path);
static void crawlDir(DependencyCrawler* crawler, const char* dir_path);
static void processLayer(DependencyCrawler* crawler, const char* filepath, 
                        const char* content, const LanguageGrammar* grammar);
static void graphMethods(DependencyCrawler* crawler, const char* file_path, 
                                Method* methods);
static void printMethods(Method* method, const char* source_file);

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
        crawler->analysis_config.analyzeMethods = 1;
        crawler->analysis_config.max_depth = -1;
        crawler->analysis_config.follow_external = 0;
    }
    
    for (int i = 0; i < directory_count; i++) {
        if (!directories[i]) {
            logr(ERROR, "[Crawler] NULL directory at index %d", i);
            freeCrawler(crawler);
            return NULL;
        }
        crawler->root_directories[i] = strdup(directories[i]);
        if (!crawler->root_directories[i]) {
            logr(ERROR, "[Crawler] Failed to duplicate directory path at index %d", i);
            freeCrawler(crawler);
            return NULL;
        }
        logr(VERBOSE, "[Crawler] Added directory: %s", crawler->root_directories[i]);
    }
    
    crawler->parsers = NULL;
    crawler->parser_count = 0;
    crawler->dependency_graph = NULL;
    crawler->result_graph = NULL;
    
    logr(VERBOSE, "[Crawler] Crawler instance created successfully");
    return crawler;
}

// Register a language-specific parser
void registerParser(DependencyCrawler* crawler, LanguageType type,
                            const LanguageParser* parser) {
    logr(VERBOSE, "[Crawler] Registering parser for language %s", languageName(type));
    crawler->parser_count++;
    crawler->parsers = (LanguageParser*)realloc(crawler->parsers, 
                                               sizeof(LanguageParser) * crawler->parser_count);
    
    crawler->parsers[crawler->parser_count - 1].type = type;
    crawler->parsers[crawler->parser_count - 1].analyze_module = parser->analyze_module;
    crawler->parsers[crawler->parser_count - 1].analyze_structure = parser->analyze_structure;
    crawler->parsers[crawler->parser_count - 1].analyzeMethod = parser->analyzeMethod;
}

// Process file content based on analysis layer
static void processLayer(DependencyCrawler* crawler, const char* filepath,
                        const char* content, const LanguageGrammar* grammar) {
    if (!crawler || !filepath || !content || !grammar) {
        logr(ERROR, "[Crawler] Invalid parameters passed to processLayer");
        return;
    }
    
    logr(VERBOSE, "[Crawler] Processing layer for file: %s", filepath);
    
    // Analyze based on configuration
    if (crawler->analysis_config.analyze_modules) {
        logr(VERBOSE, "[Crawler] Analyzing modules for file: %s", filepath);
        ExtractedDependency* deps = analyze_module(content, grammar);
        ExtractedDependency* current_dep = deps;
        
        while (current_dep) {
            logr(VERBOSE, "[Crawler] Processing module dependency: %s", 
                 current_dep->target ? current_dep->target : "NULL");
            
            ExtractedDependency* temp_dep = malloc(sizeof(ExtractedDependency));
            if (!temp_dep) {
                logr(ERROR, "[Crawler] Failed to allocate memory for temporary dependency");
                freeExtractedDep(deps);
                return;
            }
            memset(temp_dep, 0, sizeof(ExtractedDependency));
            temp_dep->file_path = strdup(filepath);
            temp_dep->target = current_dep->target ? strdup(current_dep->target) : NULL;
            temp_dep->layer = LAYER_MODULE;
            
            logr(VERBOSE, "[Crawler] Adding module dependency to graph: %s -> %s", 
                 temp_dep->file_path, temp_dep->target ? temp_dep->target : "NULL");
            
            if (!crawler->dependency_graph) {
                crawler->dependency_graph = create_dependency_from_extracted(temp_dep);
                logr(VERBOSE, "[Crawler] Created new dependency graph");
            } else {
                graphDependency(crawler->dependency_graph, temp_dep);
                logr(VERBOSE, "[Crawler] Added to existing dependency graph");
            }
            
            freeExtractedDep(temp_dep);
            current_dep = current_dep->next;
        }
        freeExtractedDep(deps);
    }

    if (crawler->analysis_config.analyze_structures) {
        logr(VERBOSE, "[Crawler] Analyzing structures in %s", filepath);
        Structure* structs = analyze_structure(content, filepath, grammar);
        if (structs) {
            logr(VERBOSE, "[Crawler] Found structure dependencies");
            ExtractedDependency* struct_dep = malloc(sizeof(ExtractedDependency));
            if (!struct_dep) {
                logr(ERROR, "[Crawler] Failed to allocate memory for structure dependency");
                freeStructures(structs);
                return;
            }
            memset(struct_dep, 0, sizeof(ExtractedDependency));
            struct_dep->file_path = strdup(filepath);
            struct_dep->structures = structs;
            struct_dep->layer = LAYER_STRUCT;
            
            // Track type source locations
            Structure* curr_struct = structs;
            while (curr_struct) {
                logr(VERBOSE, "[Crawler] Processing structure: %s", 
                     curr_struct->name ? curr_struct->name : "NULL");
                
                // Check for type dependencies in includes/imports
                ExtractedDependency* type_deps = analyze_module(content, grammar);
                ExtractedDependency* curr_dep = type_deps;
                
                while (curr_dep) {
                    logr(VERBOSE, "[Crawler] Checking type dependency: %s", 
                         curr_dep->target ? curr_dep->target : "NULL");
                    
                    // If this include/import contains our dependent type
                    if (curr_dep->target && curr_struct->dependencies && 
                        strstr(curr_struct->dependencies, curr_dep->target)) {
                        logr(VERBOSE, "[Crawler] Found type source: %s in %s", 
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
                            logr(VERBOSE, "[Crawler] Updated dependency info: %s", full_dep);
                        } else {
                            logr(ERROR, "[Crawler] Failed to allocate memory for full dependency");
                        }
                    }
                    curr_dep = curr_dep->next;
                }
                freeExtractedDep(type_deps);
                curr_struct = curr_struct->next;
            }
            
            logr(VERBOSE, "[Crawler] Adding structure dependencies to graph");
            if (!crawler->dependency_graph) {
                crawler->dependency_graph = create_dependency_from_extracted(struct_dep);
                logr(VERBOSE, "[Crawler] Created new dependency graph for structures");
            } else {
                graphDependency(crawler->dependency_graph, struct_dep);
                logr(VERBOSE, "[Crawler] Added structures to existing dependency graph");
            }
            
            freeExtractedDep(struct_dep);
        }
    }

    // Analyze methods
    if (crawler->analysis_config.analyzeMethods) {
        logr(DEBUG, "[Crawler] Analyzing methods for file: %s", filepath);
        Method* methods = analyzeMethod(filepath, content, grammar);
        
        if (methods) {
            graphMethods(crawler, filepath, methods);
        }
    }
    
    logr(VERBOSE, "[Crawler] Finished processing layer for file: %s", filepath);
}

// Process a single file
static void processFile(DependencyCrawler* crawler, const char* file_path) {
    if (!crawler || !file_path) return;
    
    logr(VERBOSE, "[Crawler] Processing file: %s", file_path);
    
    // Read file content
    FILE* file = fopen(file_path, "r");
    if (!file) {
        logr(ERROR, "[Crawler] Failed to open file: %s", file_path);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate memory and read file
    char* content = malloc(file_size + 1);
    if (!content) {
        logr(ERROR, "[Crawler] Failed to allocate memory for file content");
        fclose(file);
        return;
    }

    size_t read_size = fread(content, 1, file_size, file);
    content[read_size] = '\0';
    fclose(file);

    // Get language type and grammar
    LanguageType lang_type = languageType(file_path);
    const LanguageGrammar* grammar = languageGrammars(lang_type);
    if (!grammar) {
        free(content);
        return;
    }

    // Use processLayer instead of direct analysis
    processLayer(crawler, file_path, content, grammar);

    free(content);
}

// Recursively crawl directories
static void crawlDir(DependencyCrawler* crawler, const char* dir_path) {
    if (!crawler || !dir_path) {
        logr(ERROR, "[Crawler] Invalid parameters in crawlDir");
        return;
    }

    logr(VERBOSE, "[Crawler] Opening directory: %s", dir_path);
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
        logr(VERBOSE, "[Crawler] Processing entry: %s", path);
        
        struct stat statbuf;
        if (stat(path, &statbuf) != 0) {
            logr(ERROR, "[Crawler] Failed to stat file: %s", path);
            continue;
        }
        
        if (S_ISDIR(statbuf.st_mode)) {
            logr(VERBOSE, "[Crawler] Found directory: %s", path);
            crawlDir(crawler, path);
        } else {
            logr(VERBOSE, "[Crawler] Found file: %s", path);
            processFile(crawler, path);
        }
    }
    
    closedir(dir);
    logr(VERBOSE, "[Crawler] Finished processing directory: %s", dir_path);
}

// Main crawling function
void crawlDeps(DependencyCrawler* crawler) {
    logr(INFO, "[Crawler] Starting dependency crawl for %d directories", crawler->directory_count);
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
        crawlDir(crawler, crawler->root_directories[i]);
    }
}


// In printDependencies function
static void printMethods(Method* method, const char* source_file) {
    while (method) {
        bool is_last_method = (method->next == NULL);
        const char* method_prefix = is_last_method ? "└──" : "├──";
        
        // Format parameters string
        char param_str[256] = "";
        if (method->parameters) {
            int offset = 0;
            for (int i = 0; i < method->param_count; i++) {
                int written = snprintf(param_str + offset, sizeof(param_str) - offset, 
                    "%s%s %s%s", 
                    i > 0 ? ", " : "",
                    method->parameters[i].type ? method->parameters[i].type : "void",
                    method->parameters[i].name ? method->parameters[i].name : "",
                    method->parameters[i].default_value ? 
                        method->parameters[i].default_value : "");
                if (written < 0) break;
                offset += written;
            }
        }

        // Print method definition with all available information
        logr(INFO, "  %s %s(%s) -> %s", 
            method_prefix,
            method->name ? method->name : "(unknown)",
            param_str,
            method->return_type ? method->return_type : "void");
        
        // Print methods this method calls
        if (method->dependencies) {
            const char* dep_header_prefix = is_last_method ? "    " : "│   ";
            logr(INFO, "  %s├── calls:", dep_header_prefix);
            
            char* dep_copy = strdup(method->dependencies);
            char* dep = strtok(dep_copy, ",");
            char* next_dep = NULL;
            
            while (dep) {
                while (*dep == ' ') dep++; // Skip leading spaces
                next_dep = strtok(NULL, ",");
                const char* dep_prefix = next_dep ? "├──" : "└──";
                logr(INFO, "  %s│   %s %s", dep_header_prefix, dep_prefix, dep);
                dep = next_dep;
            }
            free(dep_copy);
        }

        // Print methods that call this method
        MethodReference* ref = method->references;
        if (ref) {
            const char* ref_header_prefix = is_last_method ? "    " : "│   ";
            logr(INFO, "  %s├── called by:", ref_header_prefix);
            
            // Count valid references first
            int valid_refs = 0;
            MethodReference* count_ref = ref;
            while (count_ref) {
                if (count_ref->called_in && strcmp(count_ref->called_in, source_file) != 0) {
                    valid_refs++;
                }
                count_ref = count_ref->next;
            }
            
            // Print references with correct prefixes
            int current_ref = 0;
            while (ref) {
                if (ref->called_in && strcmp(ref->called_in, source_file) != 0) {
                    current_ref++;
                    const char* ref_prefix = (current_ref == valid_refs) ? "└──" : "├──";
                    logr(INFO, "  %s│   %s %s", ref_header_prefix, ref_prefix, ref->called_in);
                }
                ref = ref->next;
            }
        }

        method = method->next;
    }
}

// Print dependency information
void printDependencies(DependencyCrawler* crawler) {
    if (!crawler->dependency_graph) {
        logr(INFO, "[Crawler] No dependencies found.");
        return;
    }

    int module_count = 0;
    int struct_count = 0;
    int method_count = 0;

    logr(INFO, "[Crawler] Dependencies by Layer");
    logr(INFO, "==========================\n");
    if (crawler->analysis_config.analyze_modules) {
        // Module Dependencies
        logr(INFO, "Module Dependencies:");
        logr(INFO, "-----------------");
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
    }
    
    if (crawler->analysis_config.analyze_structures) {
        // Structure Dependencies
        logr(INFO, "Structure Dependencies:");
        logr(INFO, "--------------------");
        size_t def_count;
        size_t struct_count = 0;
        StructureDefinition* defs = get_structure_definitions(&def_count);
    
        for (size_t i = 0; i < def_count; i++) {
            StructureDefinition* def = &defs[i];
            logr(INFO, "  %s %s (defined in %s)", 
                def->type,
                def->name,
                def->defined_in);
            if (def->reference_count > 0) {
                logr(INFO, "    Referenced in:");
                for (int j = 0; j < def->reference_count; j++) {
                    // Use ├── for all but the last reference, └── for the last one
                    const char* prefix = (j == def->reference_count - 1) ? "└──" : "├──";
                    logr(INFO, "      %s %s", prefix, def->referenced_in[j]);
                }
                struct_count++;
            }
        }
        logr(INFO, "Total Structure Dependencies: %zu\n", struct_count);
    }

    if (crawler->analysis_config.analyzeMethods) {
        logr(INFO, "Method Dependencies:");
        logr(INFO, "-----------------");
        
        // Create a hash table or sorted list to track unique files
        char** processed_files = NULL;
        size_t file_count = 0;
        
        // First pass - collect unique files
        Dependency* current = crawler->dependency_graph;
        while (current) {
            if (current->level == LAYER_METHOD && current->source) {
                bool found = false;
                for (size_t i = 0; i < file_count; i++) {
                    if (strcmp(processed_files[i], current->source) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    processed_files = realloc(processed_files, 
                                           (file_count + 1) * sizeof(char*));
                    processed_files[file_count] = strdup(current->source);
                    file_count++;
                }
            }
            current = current->next;
        }
        
        // Second pass - print method dependencies for each unique file
        for (size_t i = 0; i < file_count; i++) {
            logr(INFO, "Method Dependencies for %s:", processed_files[i]);
            logr(INFO, "-----------------------------");
            
            Dependency* current = crawler->dependency_graph;
            while (current) {
                if (current->level == LAYER_METHOD && current->source && 
                    strcmp(current->source, processed_files[i]) == 0) {
                    printMethods(current->methods, current->source);
                    method_count++;
                }
                current = current->next;
            }
            logr(INFO, "\nTotal Method Dependencies for %s: %d\n", processed_files[i], method_count);
            free(processed_files[i]);
        }
        free(processed_files);
        logr(INFO, "\nTotal Method Dependencies: %d\n", method_count);
    }

    logr(INFO, "\nTotal Dependencies: %zu", module_count + struct_count + method_count);
}

// Export dependencies in various formats
void exportDeps(DependencyCrawler* crawler, const char* output_format) {
    if (strcmp(output_format, "json") == 0) {
        // TODO: Implement JSON export
        logr(INFO, "[Crawler] JSON export not yet implemented");
    } else if (strcmp(output_format, "graphviz") == 0) {
        // TODO: Implement GraphViz export
        logr(INFO, "[Crawler] GraphViz export not yet implemented");
    } else {
        printDependencies(crawler);  // Default to terminal output
    }
}

// Clean up resources
void freeCrawler(DependencyCrawler* crawler) {
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
            Method* method = current->methods;
            while (method) {
                Method* next_method = method->next;
                
                // Free basic method data
                free(method->name);
                free(method->return_type);
                free(method->dependencies);
                free(method->defined_in);
                
                // Free method references
                MethodReference* ref = method->references;
                while (ref) {
                    MethodReference* next_ref = ref->next;
                    free(ref->called_in);
                    free(ref);
                    ref = next_ref;
                }
                
                // Free parameters
                for (int j = 0; j < method->param_count; j++) {
                    free(method->parameters[j].name);
                    free(method->parameters[j].type);
                    free(method->parameters[j].default_value);
                }
                free(method->parameters);
                
                // Free children methods recursively
                freeMethods(method->children);
                
                free(method);
                method = next_method;
            }
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

// Modify the dependency graph creation
static void graphMethods(DependencyCrawler* crawler, const char* file_path, Method* methods) {
    if (!crawler || !file_path || !methods) return;
    
    logr(VERBOSE, "[Crawler] Adding methods to dependency graph from %s", file_path);
    
    // Create new dependency node
    Dependency* dep = malloc(sizeof(Dependency));
    if (!dep) {
        logr(ERROR, "[Crawler] Failed to allocate memory for dependency");
        return;
    }
    
    memset(dep, 0, sizeof(Dependency));
    dep->source = strdup(file_path);
    dep->level = LAYER_METHOD;
    dep->methods = methods;  // Take ownership of the methods list
    dep->next = NULL;
    
    // Add to graph
    if (!crawler->dependency_graph) {
        crawler->dependency_graph = dep;
        logr(VERBOSE, "[Crawler] Created new dependency graph with methods");
    } else {
        // Find end of list and append
        Dependency* current = crawler->dependency_graph;
        while (current->next) {
            current = current->next;
        }
        current->next = dep;
        logr(VERBOSE, "[Crawler] Added methods to existing dependency graph");
    }
}