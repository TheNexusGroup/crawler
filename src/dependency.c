#include "dependency.h"
#include "logger.h"
#include "structure.h"

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
    if (crawler->analysis_config.analyzeModules) {
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
                    printMethods(current->methods);//, current->source);
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


// Create a dependency graph from extracted dependencies
DependencyGraph* createDependencyGraph(ExtractedDependency** deps, int dep_count) {
    if (!deps || dep_count <= 0) return NULL;

    DependencyGraph* graph = malloc(sizeof(DependencyGraph));
    graph->relationships = malloc(sizeof(Relationship*) * dep_count * MAX_DEPENDENCIES);
    graph->relationship_count = 0;
    graph->current_layer = LAYER_MODULE;

    for (int i = 0; i < dep_count; i++) {
        ExtractedDependency* dep = deps[i];
        if (!dep) continue;

        // Process module-level relationships
        if (dep->module_name) {
            Relationship* rel = malloc(sizeof(Relationship));
            rel->from = strdup(dep->file_path);
            rel->to = strdup(dep->module_name);
            rel->relationship_type = strdup("imports");
            rel->layer = LAYER_MODULE;
            graph->relationships[graph->relationship_count++] = rel;
        }

        // Process structure-level relationships
        if (dep->structures) {
            Structure* curr_struct = dep->structures;
            while (curr_struct) {
                if (curr_struct->dependencies) {
                    Relationship* rel = malloc(sizeof(Relationship));
                    rel->from = strdup(curr_struct->name);
                    rel->to = strdup(curr_struct->dependencies);
                    rel->relationship_type = strdup("inherits");
                    rel->layer = LAYER_STRUCT;
                    graph->relationships[graph->relationship_count++] = rel;
                }
                curr_struct = curr_struct->next;
            }
        }

        // Process method-level relationships
        if (dep->methods) {
            Method* curr_method = dep->methods;
            while (curr_method) {
                if (curr_method->dependencies) {
                    Relationship* rel = malloc(sizeof(Relationship));
                    rel->from = strdup(curr_method->name);
                    rel->to = strdup(curr_method->dependencies);
                    rel->relationship_type = strdup("calls");
                    rel->layer = LAYER_METHOD;
                    graph->relationships[graph->relationship_count++] = rel;
                }
                curr_method = curr_method->next;
            }
        }
    }

    return graph;
}

// Helper function to convert ExtractedDependency to Dependency
Dependency* create_dependency_from_extracted(ExtractedDependency* extracted) {
    if (!extracted) return NULL;

    Dependency* dep = malloc(sizeof(Dependency));
    if (!dep) return NULL;
    
    memset(dep, 0, sizeof(Dependency));
    dep->source = extracted->file_path ? strdup(extracted->file_path) : NULL;
    dep->target = extracted->target ? strdup(extracted->target) : NULL;
    dep->language = extracted->language;
    dep->level = extracted->layer;
    dep->next = NULL;
    
    // Copy methods if they exist
    if (extracted->methods && extracted->method_count > 0) {
        dep->methods = malloc(sizeof(Method) * extracted->method_count);
        if (dep->methods) {
            memcpy(dep->methods, extracted->methods, sizeof(Method) * extracted->method_count);
            dep->method_count = extracted->method_count;
        }
    }

    return dep;
}

// Helper function to add to existing dependency graph
void graphDependency(Dependency* graph, ExtractedDependency* extracted) {
    Dependency* new_dep = create_dependency_from_extracted(extracted);
    Dependency* curr = graph;
    while (curr->next) {
        curr = curr->next;
    }
    curr->next = new_dep;
}

void freeExtractedDep(ExtractedDependency* dep) {
    if (!dep) return;

    free(dep->file_path);
    free(dep->module_name);
    
    // Free modules
    ExtractedDependency* curr_mod = dep->modules;
    while (curr_mod) {
        ExtractedDependency* next = curr_mod->next;
        freeExtractedDep(curr_mod);
        curr_mod = next;
    }

    // Free structures
    if (dep->structures) {
        freeStructures(dep->structures);
    }

    // Free methods
    if (dep->methods) {
        freeMethods(dep->methods);
    }

    free(dep);
}
