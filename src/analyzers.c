#include "analyzers.h"
#include "pattern_cache.h"
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "logger.h"

ExtractedDependency* analyze_module(const char* content, const LanguageGrammar* grammar) {
    if (!content || !grammar) {
        return NULL;
    }

    ExtractedDependency* head = NULL;
    ExtractedDependency* current = NULL;

    // Process the content line by line
    const char* line_start = content;
    const char* line_end;
    
    while ((line_end = strchr(line_start, '\n')) != NULL) {
        // Create a temporary buffer for the current line
        size_t line_length = line_end - line_start;
        char* line = malloc(line_length + 1);
        if (!line) continue;
        
        strncpy(line, line_start, line_length);
        line[line_length] = '\0';

        // Check each module pattern against the current line
        for (size_t i = 0; i < grammar->module_pattern_count; i++) {
            regex_t regex;
            if (regcomp(&regex, grammar->module_patterns[i], REG_EXTENDED) != 0) {
                free(line);
                continue;
            }

            regmatch_t matches[2];  // [0] for full match, [1] for capture group
            
            if (regexec(&regex, line, 2, matches, 0) == 0) {
                // Extract the matched dependency
                size_t len = matches[1].rm_eo - matches[1].rm_so;
                char* dep_str = malloc(len + 1);
                if (dep_str) {
                    strncpy(dep_str, line + matches[1].rm_so, len);
                    dep_str[len] = '\0';

                    // Create new dependency node
                    ExtractedDependency* new_dep = malloc(sizeof(ExtractedDependency));
                    if (new_dep) {
                        memset(new_dep, 0, sizeof(ExtractedDependency));
                        new_dep->target = dep_str;
                        new_dep->layer = LAYER_MODULE;
                        new_dep->next = NULL;

                        // Add to linked list
                        if (!head) {
                            head = new_dep;
                            current = head;
                        } else {
                            current->next = new_dep;
                            current = new_dep;
                        }
                    } else {
                        free(dep_str);
                    }
                }
            }
            regfree(&regex);
        }

        free(line);
        line_start = line_end + 1;  // Move to the start of the next line
    }

    // Process the last line if it doesn't end with a newline
    if (*line_start != '\0') {
        char* line = strdup(line_start);
        if (line) {
            // Check each module pattern against the last line
            for (size_t i = 0; i < grammar->module_pattern_count; i++) {
                regex_t regex;
                if (regcomp(&regex, grammar->module_patterns[i], REG_EXTENDED) != 0) {
                    free(line);
                    continue;
                }

                regmatch_t matches[2];
                
                if (regexec(&regex, line, 2, matches, 0) == 0) {
                    size_t len = matches[1].rm_eo - matches[1].rm_so;
                    char* dep_str = malloc(len + 1);
                    if (dep_str) {
                        strncpy(dep_str, line + matches[1].rm_so, len);
                        dep_str[len] = '\0';

                        ExtractedDependency* new_dep = malloc(sizeof(ExtractedDependency));
                        if (new_dep) {
                            memset(new_dep, 0, sizeof(ExtractedDependency));
                            new_dep->target = dep_str;
                            new_dep->layer = LAYER_MODULE;
                            new_dep->next = NULL;

                            if (!head) {
                                head = new_dep;
                                current = head;
                            } else {
                                current->next = new_dep;
                                current = new_dep;
                            }
                        } else {
                            free(dep_str);
                        }
                    }
                }
                regfree(&regex);
            }
            free(line);
        }
    }

    return head;
}

Structure* analyze_structure(const char* content, const LanguageGrammar* grammar) {
    if (!content || !grammar) {
        logr(ERROR, "[Analyzer] Invalid parameters for structure analysis");
        return NULL;
    }

    logr(DEBUG, "[Analyzer] Starting structure analysis");
    Structure* head = NULL;
    Structure* current = NULL;

    // Get compiled patterns
    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_STRUCT);
    if (!patterns) {
        logr(ERROR, "[Analyzer] Failed to get compiled patterns");
        return NULL;
    }

    // Process the content line by line
    const char* line_start = content;
    const char* line_end;
    
    while ((line_end = strchr(line_start, '\n')) != NULL) {
        size_t line_length = line_end - line_start;
        char* line = malloc(line_length + 1);
        if (!line) {
            logr(ERROR, "[Analyzer] Failed to allocate memory for line");
            continue;
        }
        
        strncpy(line, line_start, line_length);
        line[line_length] = '\0';
        logr(DEBUG, "[Analyzer] Processing line for structure: %s", line);

        // Check each struct pattern
        for (size_t i = 0; i < patterns->pattern_count; i++) {
            regmatch_t matches[3];  // [0] full match, [1] name, [2] inheritance
            int result = regexec(&patterns->compiled_patterns[i], line, 3, matches, 0);
            
            if (result == 0) {
                logr(DEBUG, "[Analyzer] Found structure match");
                Structure* new_struct = malloc(sizeof(Structure));
                if (!new_struct) {
                    logr(ERROR, "[Analyzer] Failed to allocate memory for structure");
                    free(line);
                    continue;
                }
                memset(new_struct, 0, sizeof(Structure));

                // Extract structure name
                size_t name_len = matches[1].rm_eo - matches[1].rm_so;
                new_struct->name = malloc(name_len + 1);
                if (new_struct->name) {
                    strncpy(new_struct->name, line + matches[1].rm_so, name_len);
                    new_struct->name[name_len] = '\0';

                    // Look for inheritance
                    if (matches[2].rm_so != -1) {
                        new_struct->dependencies = strdup(line + matches[2].rm_so);
                    } else {
                        new_struct->dependencies = NULL;
                    }

                    new_struct->next = NULL;

                    if (!head) {
                        head = new_struct;
                        current = head;
                    } else {
                        current->next = new_struct;
                        current = new_struct;
                    }
                } else {
                    logr(ERROR, "[Analyzer] Failed to allocate memory for structure name");
                    free(new_struct);
                }
            }
        }
        free(line);
        line_start = line_end + 1;
    }

    logr(DEBUG, "[Analyzer] Completed structure analysis");
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