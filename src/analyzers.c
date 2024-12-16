#ifndef ANALYZERS_H
#define ANALYZERS_H

#include "syntax_map.h"
#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define MAX_MATCHES 10

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

// Unified analyzer function
ExtractedDependency* analyze_dependencies(const char* content, 
                                        LanguageType lang,
                                        AnalysisLayer layer) {
    ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
    memset(dep, 0, sizeof(ExtractedDependency));
    
    const LanguageGrammar* grammar = get_language_grammar(lang);
    const CompiledPatterns* patterns = get_compiled_patterns(lang, layer);
    
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
