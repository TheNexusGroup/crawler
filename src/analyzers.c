#include "analyzers.h"
#include "pattern_cache.h"
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "logger.h"
#include <stdbool.h>
#include <ctype.h>


#define MAX_STRUCTURE_DEFS 1024
StructureDefinition* structure_definitions = NULL;
size_t structure_def_count = 0;

#define MAX_METHOD_DEFS 1024
MethodDefinition* method_definitions = NULL;
size_t method_def_count = 0;

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
        regmatch_t matches[4];  // We need up to 4 slots now: full match, type, name1, name2

        while (regexec(regex, pos, 4, matches, 0) == 0) {
            // Extract the type (typedef struct, struct, enum, etc.)
            size_t type_len = matches[1].rm_eo - matches[1].rm_so;
            char* type = malloc(type_len + 1);
            strncpy(type, pos + matches[1].rm_so, type_len);
            type[type_len] = '\0';

            // For typedef struct cases, use the third capture group if it exists
            size_t name_group = (matches[3].rm_so != -1) ? 3 : 2;
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
                    free(type);
                    break;
                }
            }

            if (!found && structure_def_count < MAX_STRUCTURE_DEFS) {
                structure_definitions[structure_def_count].name = struct_name;
                structure_definitions[structure_def_count].type = type;
                structure_definitions[structure_def_count].defined_in = strdup(file_path);
                structure_definitions[structure_def_count].max_references = 32;
                structure_definitions[structure_def_count].referenced_in = 
                    calloc(32, sizeof(char*));
                structure_def_count++;
                logr(DEBUG, "[Analyzer] Found %s definition: %s in %s", 
                     type, struct_name, file_path);
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

// Add this helper function to parse parameters from a parameter string
Parameter* parse_parameters(const char* params_str) {
    if (!params_str || !*params_str) return NULL;

    // Count parameters first (by counting commas + 1)
    int param_count = 1;
    const char* p = params_str;
    while (*p) {
        if (*p == ',') param_count++;
        p++;
    }

    // Allocate parameter array
    Parameter* params = calloc(param_count, sizeof(Parameter));
    if (!params) return NULL;

    // Parse each parameter
    char* params_copy = strdup(params_str);
    char* token = strtok(params_copy, ",");
    int idx = 0;

    while (token && idx < param_count) {
        // Skip leading whitespace
        while (*token && isspace(*token)) token++;

        // Parse parameter parts (type and name)
        char* type_end = token;
        while (*type_end && !isspace(*type_end)) type_end++;
        
        if (*type_end) {
            // We found a space, split into type and name
            *type_end = '\0';
            char* name_start = type_end + 1;
            while (*name_start && isspace(*name_start)) name_start++;

            // Check for default value
            char* default_start = strchr(name_start, '=');
            if (default_start) {
                *default_start = '\0';
                default_start++;
                while (*default_start && isspace(*default_start)) default_start++;
                params[idx].default_value = strdup(default_start);
            }

            params[idx].type = strdup(token);
            params[idx].name = strdup(name_start);
            idx++;
        }

        token = strtok(NULL, ",");
    }

    return params;
}

Method* analyze_method(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    logr(DEBUG, "[Analyzer] Starting method analysis for file: %s", file_path);
    
    // Get compiled patterns
    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_METHOD);
    if (!patterns) {
        logr(ERROR, "[Analyzer] Failed to get method patterns");
        return NULL;
    }

    // First collect method definitions if not already done
    collect_method_definitions(file_path, content, grammar);
    logr(DEBUG, "[Analyzer] Found %zu method definitions", method_def_count);

    Method* head = NULL;
    Method* current = NULL;

    // Now scan for references to known methods
    for (size_t i = 0; i < method_def_count; i++) {
        const char* method_name = method_definitions[i].name;
        if (!method_name) continue;

        // Create pattern to look for method calls
        char pattern[256];
        // Look for method name followed by opening parenthesis, not part of a definition
        snprintf(pattern, sizeof(pattern), "[^a-zA-Z0-9_]%s\\s*\\(", method_name);
        
        regex_t regex;
        if (regcomp(&regex, pattern, REG_EXTENDED) == 0) {
            regmatch_t matches[1];
            const char* pos = content;
            
            while (regexec(&regex, pos, 1, matches, 0) == 0) {
                // Only count as a call if not part of a definition
                char before_char = (matches[0].rm_so > 0) ? pos[matches[0].rm_so - 1] : '\n';
                if (!isalpha(before_char) && !isdigit(before_char) && before_char != '_') {
                    // Found a valid method call
                    Method* method = malloc(sizeof(Method));
                    if (!method) continue;
                    
                    memset(method, 0, sizeof(Method));
                    method->name = strdup(method_name);
                    method->dependencies = strdup(method_definitions[i].defined_in);
                    
                    // Add to list
                    if (!head) {
                        head = method;
                        current = method;
                    } else {
                        current->next = method;
                        current = method;
                    }
                }
                pos += matches[0].rm_eo;
            }
            regfree(&regex);
        }
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

void collect_method_definitions(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    if (!method_definitions) {
        method_definitions = calloc(MAX_METHOD_DEFS, sizeof(MethodDefinition));
        if (!method_definitions) {
            logr(ERROR, "[Analyzer] Failed to allocate method definitions array");
            return;
        }
    }

    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_METHOD);
    if (!patterns) {
        logr(ERROR, "[Analyzer] No compiled patterns found for methods");
        return;
    }

    logr(DEBUG, "[Analyzer] Searching for methods in file: %s", file_path);
    logr(DEBUG, "[Analyzer] Content length: %zu bytes", strlen(content));

    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        const char* pos = content;
        regmatch_t matches[5];

        logr(DEBUG, "[Analyzer] Trying pattern %zu: %s", i, grammar->method_patterns[i]);

        while (pos && *pos && regexec(regex, pos, 5, matches, 0) == 0) {
            // Extract the full match for debugging
            int match_len = matches[0].rm_eo - matches[0].rm_so;
            char* match_text = malloc(match_len + 1);
            if (!match_text) continue;

            strncpy(match_text, pos + matches[0].rm_so, match_len);
            match_text[match_len] = '\0';

            logr(DEBUG, "[Analyzer] Found match: '%s'", match_text);

            // Determine the correct group for the method name
            int name_group = 1;  // Adjust based on pattern index

            if (matches[name_group].rm_so != -1) {
                int name_len = matches[name_group].rm_eo - matches[name_group].rm_so;
                char* method_name = malloc(name_len + 1);
                if (method_name) {
                    strncpy(method_name, pos + matches[name_group].rm_so, name_len);
                    method_name[name_len] = '\0';

                    // Add to definitions if not already present
                    bool found = false;
                    for (size_t j = 0; j < method_def_count; j++) {
                        if (method_definitions[j].name && 
                            strcmp(method_definitions[j].name, method_name) == 0) {
                            found = true;
                            free(method_name);
                            break;
                        }
                    }

                    if (!found && method_def_count < MAX_METHOD_DEFS) {
                        method_definitions[method_def_count].name = method_name;
                        method_definitions[method_def_count].defined_in = strdup(file_path);
                        method_def_count++;
                        logr(INFO, "[Analyzer] Found method definition: %s in %s", method_name, file_path);
                    }
                }
            }

            free(match_text);
            pos += matches[0].rm_eo;
        }
    }
}
