#ifndef ANALYZERS_H
#define ANALYZERS_H

#include "syntax_map.h"
#include "grammars.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE_LENGTH 1024
#define MAX_MATCHES 10

// Process module-level matches
void process_module_matches(ExtractedDependency* dep, char** matches, size_t match_count) {
    if (!dep || !matches) return;

    // Allocate space for new dependencies if needed
    if (!dep->module_name) {
        dep->module_name = strdup(matches[0]);
    }

    // Process each match as a module dependency
    for (size_t i = 0; i < match_count; i++) {
        // TODO: Add module dependencies to the dependency structure
        // For now, just store the module name
        if (i == 0 && !dep->module_name) {
            dep->module_name = strdup(matches[i]);
        }
    }
}

// Process structure-level matches
void process_struct_matches(ExtractedDependency* dep, char** matches, size_t match_count) {
    if (!dep || !matches) return;

    // Allocate space for new structures if needed
    size_t new_count = dep->structure_count + 1;
    dep->structures = realloc(dep->structures, sizeof(Structure) * new_count);
    Structure* structure = &dep->structures[dep->structure_count];
    memset(structure, 0, sizeof(Structure));

    // Process the structure name and any dependencies
    if (match_count > 0) {
        structure->name = strdup(matches[0]);
        
        // Process additional matches as dependencies or traits
        for (size_t i = 1; i < match_count; i++) {
            // Add implemented traits
            if (structure->trait_count < MAX_TRAITS) {
                structure->implemented_traits = realloc(structure->implemented_traits, 
                                                     sizeof(char*) * (structure->trait_count + 1));
                structure->implemented_traits[structure->trait_count++] = strdup(matches[i]);
            }
        }
    }

    dep->structure_count = new_count;
}

// Process method-level matches
void process_method_matches(ExtractedDependency* dep, char** matches, size_t match_count) {
    if (!dep || !matches || match_count == 0) return;

    // Allocate space for new methods if needed
    size_t new_count = dep->method_count + 1;
    dep->methods = realloc(dep->methods, sizeof(Method) * new_count);
    Method* method = &dep->methods[dep->method_count];
    memset(method, 0, sizeof(Method));

    // Initialize parameters array
    method->param_count = 0;

    // Process method name and return type
    if (match_count > 0) {
        method->name = strdup(matches[0]);
        if (match_count > 1) {
            method->return_type = strdup(matches[1]);
        }

        // Process parameters if present
        for (size_t i = 2; i < match_count && method->param_count < MAX_PARAMETERS; i++) {
            Parameter* param = &method->parameters[method->param_count++];
            memset(param, 0, sizeof(Parameter));

            // Parse parameter string (format: "name:type=default")
            char* param_str = strdup(matches[i]);
            char* type_sep = strchr(param_str, ':');
            char* default_sep = strchr(param_str, '=');

            if (type_sep) {
                *type_sep = '\0';
                param->name = strdup(param_str);
                if (default_sep) {
                    *default_sep = '\0';
                    param->type = strdup(type_sep + 1);
                    param->default_value = strdup(default_sep + 1);
                } else {
                    param->type = strdup(type_sep + 1);
                }
            } else {
                param->name = strdup(param_str);
            }
            free(param_str);
        }
    }

    dep->method_count = new_count;
}

// Helper function to extract matches from regex
static char** extract_matches(const regex_t* pattern, const char* content, 
                            size_t* match_count) {
    regmatch_t matches[MAX_MATCHES];
    char** results = NULL;
    *match_count = 0;
    
    if (regexec(pattern, content, MAX_MATCHES, matches, 0) == 0) {
        results = malloc(sizeof(char*) * MAX_MATCHES);
        
        for (size_t i = 1; i < MAX_MATCHES && matches[i].rm_so != -1; i++) {
            size_t len = matches[i].rm_eo - matches[i].rm_so;
            results[*match_count] = malloc(len + 1);
            strncpy(results[*match_count], 
                   content + matches[i].rm_so, 
                   len);
            results[*match_count][len] = '\0';
            (*match_count)++;
        }
    }
    
    return results;
}

// Main analyzer function
ExtractedDependency* analyze_dependencies(const char* content, 
                                        LanguageType lang,
                                        AnalysisLayer layer) {
    ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
    memset(dep, 0, sizeof(ExtractedDependency));
    
    const LanguageGrammar* grammar = languageGrammars(lang);
    const CompiledPatterns* patterns = compiledPatterns(lang, layer);
    
    if (!patterns || !grammar) return dep;
    
    char line[MAX_LINE_LENGTH];
    const char* pos = content;
    
    while (*pos) {
        // Get next line
        size_t i = 0;
        while (*pos && *pos != '\n' && i < MAX_LINE_LENGTH - 1) {
            line[i++] = *pos++;
        }
        line[i] = '\0';
        if (*pos) pos++;
        
        // Check each pattern for the current layer
        for (size_t j = 0; j < patterns->pattern_count; j++) {
            size_t match_count;
            char** matches = extract_matches(&patterns->compiled_patterns[j], 
                                          line, &match_count);
            
            if (matches) {
                switch (layer) {
                    case LAYER_MODULE:
                        process_module_matches(dep, matches, match_count);
                        break;
                        
                    case LAYER_STRUCT:
                        process_struct_matches(dep, matches, match_count);
                        break;
                        
                    case LAYER_METHOD:
                        process_method_matches(dep, matches, match_count);
                        break;
                }
                
                // Cleanup matches
                for (size_t k = 0; k < match_count; k++) {
                    free(matches[k]);
                }
                free(matches);
            }
        }
    }
    
    dep->layer = layer;
    return dep;
}

#endif // ANALYZERS_H
