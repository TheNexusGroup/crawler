#include "analyzers.h"
#include "pattern_cache.h"
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "logger.h"
#include <stdbool.h>
#include <ctype.h>

// Helper function declarations
static void add_method_reference(MethodDefinition* def, const char* file_path);

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

// Helper function to check if we're within a valid scope
static bool is_within_scope(const char* content, size_t position, ScopeContext* context) {
    (void)context; // Explicitly mark as unused
    
    // Count braces from start of scope definition to current position
    const char* pos = content;
    int brace_count = 0;
    
    while (pos < content + position) {
        if (*pos == '{') brace_count++;
        else if (*pos == '}') brace_count--;
        pos++;
    }
    
    return brace_count > 0;
}

// Add this helper function before analyze_method
static Method* extract_method_from_match(const char* content, regmatch_t* matches, const LanguageGrammar* grammar) {
    if (!content || !matches || !grammar) return NULL;

    Method* method = malloc(sizeof(Method));
    if (!method) return NULL;
    memset(method, 0, sizeof(Method));

    // Extract return type (group 1)
    if (matches[1].rm_so != -1) {
        size_t type_len = matches[1].rm_eo - matches[1].rm_so;
        method->return_type = malloc(type_len + 1);
        if (method->return_type) {
            strncpy(method->return_type, content + matches[1].rm_so, type_len);
            method->return_type[type_len] = '\0';
            // Clean up whitespace
            char* end = method->return_type + strlen(method->return_type) - 1;
            while (end > method->return_type && isspace(*end)) *end-- = '\0';
        }
    }

    // Extract method name (group 2 by default in all grammars)
    size_t name_len = matches[grammar->method_name_group].rm_eo - matches[grammar->method_name_group].rm_so;
    method->name = malloc(name_len + 1);
    if (method->name) {
        strncpy(method->name, content + matches[grammar->method_name_group].rm_so, name_len);
        method->name[name_len] = '\0';
    }

    // Extract parameters (group after method name)
    int param_group = grammar->method_name_group + 1;
    if (matches[param_group].rm_so != -1) {
        size_t param_len = matches[param_group].rm_eo - matches[param_group].rm_so;
        char* param_str = malloc(param_len + 1);
        if (param_str) {
            strncpy(param_str, content + matches[param_group].rm_so, param_len);
            param_str[param_len] = '\0';
            method->parameters = parse_parameters(param_str);
            free(param_str);
        }
    }

    return method;
}

// Update the analyze_method function to separate definitions from calls
Method* analyze_method(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    if (!content || !grammar) {
        logr(ERROR, "[Analyzer] Invalid parameters for method analysis");
        return NULL;
    }

    Method* head = NULL;
    Method* current = NULL;

    // First collect method definitions
    collect_method_definitions(file_path, content, grammar);

    // Convert method_definitions to Method list
    for (size_t i = 0; i < method_def_count; i++) {
        MethodDefinition* def = &method_definitions[i];
        if (!def->name || !def->defined_in || strcmp(def->defined_in, file_path) != 0) continue;

        Method* method = malloc(sizeof(Method));
        if (!method) continue;

        memset(method, 0, sizeof(Method));
        method->name = strdup(def->name);
        method->defined_in = strdup(def->defined_in);
        method->is_definition = true;
        
        // Convert references
        MethodReference* ref = def->references;
        while (ref) {
            MethodReference* new_ref = malloc(sizeof(MethodReference));
            if (new_ref) {
                new_ref->called_in = strdup(ref->called_in);
                new_ref->next = method->references;
                method->references = new_ref;
                method->reference_count++;
            }
            ref = ref->next;
        }

        if (!head) {
            head = method;
            current = method;
        } else {
            current->next = method;
            current = method;
        }
    }

    return head;
}

// Add this cleanup function
void free_method(Method* method) {
    if (!method) return;
    
    free(method->name);
    free(method->return_type);
    free(method->defined_in);
    free(method->dependencies);
    
    // Free parameters
    for (int i = 0; i < method->param_count; i++) {
        free(method->parameters[i].name);
        free(method->parameters[i].type);
        free(method->parameters[i].default_value);
    }
    free(method->parameters);
    
    // Free references
    MethodReference* ref = method->references;
    while (ref) {
        MethodReference* next = ref->next;
        free(ref->called_in);
        free(ref);
        ref = next;
    }
    
    // Free children methods
    free_methods(method->children);
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

static bool is_method_definition(const char* method_start, size_t len) {
    // Look for opening brace after the method declaration
    const char* pos = method_start;
    const char* end = method_start + len;
    
    // Skip past the closing parenthesis
    while (pos < end && *pos != ')') pos++;
    if (pos >= end) return false;
    
    // Look for opening brace, skipping whitespace
    pos++;  // Move past ')'
    while (pos < end && isspace(*pos)) pos++;
    
    // Should find an opening brace
    return (pos < end && *pos == '{');
}

static void add_method_definition(const char* method_name, const char* file_path) {
    if (method_def_count >= MAX_METHOD_DEFS || !method_name || !file_path) return;
    
    // Skip empty method names
    if (strlen(method_name) == 0) return;
    
    MethodDefinition* def = &method_definitions[method_def_count];
    def->name = strdup(method_name);
    def->defined_in = strdup(file_path);
    def->dependencies = NULL;
    def->references = NULL;
    def->reference_count = 0;
    
    logr(DEBUG, "[Analyzer] Added method definition: %s in %s", 
         def->name, file_path);
    
    method_def_count++;
}

static bool found_existing_definition(const char* method_name) {
    for (size_t i = 0; i < method_def_count; i++) {
        if (strcmp(method_definitions[i].name, method_name) == 0) {
            return true;
        }
    }
    return false;
}

void collect_method_definitions(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    if (!method_definitions) {
        method_definitions = calloc(MAX_METHOD_DEFS, sizeof(MethodDefinition));
        if (!method_definitions) return;
    }

    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_METHOD);
    if (!patterns) return;

    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        const char* pos = content;
        regmatch_t matches[4];  // We need up to 4 groups

        while (regexec(regex, pos, 4, matches, 0) == 0) {
            if (is_method_definition(pos + matches[0].rm_so, 
                                   matches[0].rm_eo - matches[0].rm_so)) {
                
                // Extract method name using the language-specific group
                size_t name_len = matches[grammar->method_name_group].rm_eo - 
                                matches[grammar->method_name_group].rm_so;
                
                char* method_name = malloc(name_len + 1);
                if (!method_name) continue;
                
                strncpy(method_name, 
                        pos + matches[grammar->method_name_group].rm_so, 
                        name_len);
                method_name[name_len] = '\0';

                if (!found_existing_definition(method_name)) {
                    add_method_definition(method_name, file_path);
                }
                free(method_name);
            }
            pos += matches[0].rm_eo;
        }
    }
}

void free_method_references(MethodReference* refs) {
    while (refs) {
        MethodReference* next = refs->next;
        free(refs->called_in);
        free(refs);
        refs = next;
    }
}

// Update the existing cleanup code to use this
void free_method_definitions() {
    for (size_t i = 0; i < method_def_count; i++) {
        free(method_definitions[i].name);
        free(method_definitions[i].defined_in);
        free(method_definitions[i].dependencies);
        free_method_references(method_definitions[i].references);
    }
    free(method_definitions);
    method_definitions = NULL;
    method_def_count = 0;
}

// Add these implementations before collect_method_definitions

static void add_method_reference(MethodDefinition* def, const char* file_path) {
    if (!def || !file_path) return;

    // Check if we already have this reference
    MethodReference* curr = def->references;
    while (curr) {
        if (strcmp(curr->called_in, file_path) == 0) {
            return; // Already recorded
        }
        curr = curr->next;
    }

    // Add new reference
    MethodReference* new_ref = malloc(sizeof(MethodReference));
    if (!new_ref) return;

    new_ref->called_in = strdup(file_path);
    new_ref->next = def->references;
    def->references = new_ref;
    def->reference_count++;
    
    logr(DEBUG, "[Analyzer] Added reference to method %s from %s", 
         def->name, file_path);
}

