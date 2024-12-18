#include "analyzers.h"
#include "pattern_cache.h"
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "logger.h"
#include <stdbool.h>


#define MAX_STRUCTURE_DEFS 1024
StructureDefinition* structure_definitions = NULL;
size_t structure_def_count = 0;

// Add this helper function
StructureDefinition* get_structure_definitions(size_t* count) {
    if (count) *count = structure_def_count;
    return structure_definitions;
}

// New function to collect structure definitions
void collect_structure_definitions(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    if (!structure_definitions) {
        structure_definitions = calloc(MAX_STRUCTURE_DEFS, sizeof(StructureDefinition));
        if (!structure_definitions) return;
    }

    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_STRUCT);
    if (!patterns) return;

    // Only match against the actual structure patterns for this language
    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        
        const char* pos = content;
        regmatch_t matches[3];  // We need up to 3 slots for some patterns

        while (regexec(regex, pos, 3, matches, 0) == 0) {
            // For typedef struct cases, use the second capture group if it exists
            size_t name_group = (matches[2].rm_so != -1) ? 2 : 1;
            size_t len = matches[name_group].rm_eo - matches[name_group].rm_so;
            char* struct_name = malloc(len + 1);
            strncpy(struct_name, pos + matches[name_group].rm_so, len);
            struct_name[len] = '\0';

            // Add to definitions if not already present
            bool found = false;
            for (size_t j = 0; j < structure_def_count; j++) {
                if (structure_definitions[j].name && 
                    strcmp(structure_definitions[j].name, struct_name) == 0) {
                    found = true;
                    free(struct_name);
                    break;
                }
            }

            if (!found && structure_def_count < MAX_STRUCTURE_DEFS) {
                structure_definitions[structure_def_count].name = struct_name;
                structure_definitions[structure_def_count].defined_in = strdup(file_path);
                structure_definitions[structure_def_count].max_references = 32;
                structure_definitions[structure_def_count].referenced_in = 
                    calloc(32, sizeof(char*));
                structure_def_count++;
                logr(DEBUG, "[Analyzer] Found structure definition: %s in %s", struct_name, file_path);
            }

            pos += matches[0].rm_eo;
        }
    }
}

// Modify analyze_structure to also look for references
Structure* analyze_structure(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    if (!content || !grammar) {
        logr(ERROR, "[Analyzer] Invalid parameters for structure analysis");
        return NULL;
    }

    logr(DEBUG, "[Analyzer] Starting structure analysis");
    Structure* head = NULL;
    
    // Get compiled patterns
    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_STRUCT);
    if (!patterns) {
        logr(ERROR, "[Analyzer] Failed to get compiled patterns");
        return NULL;
    }

    // First collect structure definitions if not already done
    collect_structure_definitions(file_path, content, grammar);

    // Now scan for references to known structures
    for (size_t i = 0; i < structure_def_count; i++) {
        const char* struct_name = structure_definitions[i].name;
        if (!struct_name) continue;

        // Create pattern to look for structure name
        char pattern[256];
        snprintf(pattern, sizeof(pattern), "\\b%s\\b", struct_name);
        
        regex_t regex;
        if (regcomp(&regex, pattern, REG_EXTENDED) == 0) {
            if (regexec(&regex, content, 0, NULL, 0) == 0) {
                // Found a reference
                StructureDefinition* def = &structure_definitions[i];
                if (strcmp(def->defined_in, file_path) != 0) {  // Don't count self-references
                    for (size_t j = 0; j < def->max_references; j++) {
                        if (!def->referenced_in[j]) {
                            def->referenced_in[j] = strdup(file_path);
                            def->reference_count++;
                            break;
                        }
                    }
                }
            }
            regfree(&regex);
        }
    }

    return head;
}

Method* analyze_method(const char* content, const LanguageGrammar* grammar) {
    Method* head = NULL;
    Method* current = NULL;

    // Use the method patterns from the grammar
    for (size_t i = 0; i < grammar->method_pattern_count; i++) {
        regex_t regex;
        if (regcomp(&regex, grammar->method_patterns[i], REG_EXTENDED) != 0) {
            continue;
        }

        regmatch_t match;
        const char* pos = content;
        
        while (regexec(&regex, pos, 1, &match, 0) == 0) {
            // Extract the matched method
            size_t len = match.rm_eo - match.rm_so;
            char* method_str = malloc(len + 1);
            strncpy(method_str, pos + match.rm_so, len);
            method_str[len] = '\0';

            // Create new method node
            Method* method = malloc(sizeof(Method));
            method->name = method_str;
            method->dependencies = NULL;  // Dependencies will be analyzed separately
            method->next = NULL;

            // Add to list
            if (!head) {
                head = method;
                current = method;
            } else {
                current->next = method;
                current = method;
            }

            pos += match.rm_eo;
        }

        regfree(&regex);
    }

    return head;
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
void add_to_dependency_graph(Dependency* graph, ExtractedDependency* extracted) {
    Dependency* new_dep = create_dependency_from_extracted(extracted);
    Dependency* curr = graph;
    while (curr->next) {
        curr = curr->next;
    }
    curr->next = new_dep;
}

void free_extracted_dependency(ExtractedDependency* dep) {
    if (!dep) return;

    free(dep->file_path);
    free(dep->module_name);
    
    // Free modules
    ExtractedDependency* curr_mod = dep->modules;
    while (curr_mod) {
        ExtractedDependency* next = curr_mod->next;
        free_extracted_dependency(curr_mod);
        curr_mod = next;
    }

    // Free structures
    if (dep->structures) {
        free_structures(dep->structures);
    }

    // Free methods
    if (dep->methods) {
        free_methods(dep->methods);
    }

    free(dep);
}

// Add this implementation after the existing functions
ExtractedDependency* analyze_module(const char* content, const LanguageGrammar* grammar) {
    if (!content || !grammar) {
        logr(ERROR, "[Analyzer] Invalid parameters for module analysis");
        return NULL;
    }

    ExtractedDependency* head = NULL;
    ExtractedDependency* current = NULL;

    // Get compiled patterns
    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_MODULE);
    if (!patterns) {
        logr(ERROR, "[Analyzer] Failed to get compiled patterns for module analysis");
        return NULL;
    }

    // Process each pattern
    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t regex;
        if (regcomp(&regex, grammar->module_patterns[i], REG_EXTENDED) != 0) {
            logr(ERROR, "[Analyzer] Failed to compile module pattern: %s", grammar->module_patterns[i]);
            continue;
        }

        regmatch_t matches[2];  // [0] full match, [1] module name
        const char* pos = content;

        while (regexec(&regex, pos, 2, matches, 0) == 0) {
            // Extract the matched module name
            size_t len = matches[1].rm_eo - matches[1].rm_so;
            char* module_name = malloc(len + 1);
            strncpy(module_name, pos + matches[1].rm_so, len);
            module_name[len] = '\0';

            // Create new dependency node
            ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
            memset(dep, 0, sizeof(ExtractedDependency));
            dep->module_name = module_name;
            dep->target = strdup(module_name);  // For module dependencies, target is the module name
            dep->layer = LAYER_MODULE;
            dep->next = NULL;

            // Add to list
            if (!head) {
                head = dep;
                current = dep;
            } else {
                current->next = dep;
                current = dep;
            }

            pos += matches[0].rm_eo;
        }
        regfree(&regex);
    }

    return head;
}