#include "analyzers.h"
#include "pattern_cache.h"
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "logger.h"
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

// Forward declarations at the top of the file
static bool definitionFound(const char* method_name);
static bool methodDefinition(const char* method_start, size_t len);
static MethodDefinition* findMethodDefinition(const char* method_name);

static bool isKeyword(const char* name, const LanguageGrammar* grammar) {
    if (!name || !grammar || !grammar->keywords) {
        logr(VERBOSE, "[Analyzer] Invalid parameters for keyword check: name=%s, grammar=%p", 
             name ? name : "NULL", (void*)grammar);
        return false;
    }
    
    logr(VERBOSE, "[Analyzer] Checking if '%s' is a keyword (total keywords: %zu)", 
         name, grammar->keyword_count);
    
    for (size_t i = 0; i < grammar->keyword_count; i++) {
        logr(VERBOSE, "[Analyzer] Comparing '%s' with keyword '%s'", 
             name, grammar->keywords[i]);
        if (strcmp(name, grammar->keywords[i]) == 0) {
            logr(VERBOSE, "[Analyzer] Found keyword match: %s", name);
            return true;
        }
    }
    logr(VERBOSE, "[Analyzer] '%s' is not a keyword", name);
    return false;
}

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
void collectStructures(const char* file_path, const char* content, const LanguageGrammar* grammar) {
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
    collectStructures(file_path, content, grammar);

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

Parameter* parse_parameters(const char* params_str) {
    if (!params_str || !*params_str) return NULL;

    int param_count = 1;
    const char* p = params_str;
    while (*p) {
        if (*p == ',') param_count++;
        p++;
    }

    Parameter* params = calloc(param_count + 1, sizeof(Parameter));
    if (!params) return NULL;

    char* params_copy = strdup(params_str);
    char* token = strtok(params_copy, ",");
    int idx = 0;

    while (token && idx < param_count) {
        while (*token && isspace(*token)) token++;

        char* last_space = strrchr(token, ' ');
        if (last_space) {
            *last_space = '\0';
            char* name_start = last_space + 1;
            
            char* end = last_space - 1;
            while (end > token && isspace(*end)) *end-- = '\0';
            
            params[idx].type = strdup(token);
            params[idx].name = strdup(name_start);
        } else {
            params[idx].type = strdup(token);
            params[idx].name = strdup("");
        }

        idx++;
        token = strtok(NULL, ",");
    }

    params[idx].type = NULL;
    params[idx].name = NULL;
    params[idx].default_value = NULL;

    free(params_copy);
    return params;
}


static void addMethodReference(MethodDefinition* method, const char* called_in) {
    if (!method || !called_in) return;
    
    // Create new reference
    MethodReference* ref = malloc(sizeof(MethodReference));
    if (!ref) return;
    
    ref->called_in = strdup(called_in);
    ref->next = NULL;
    
    // Add to list, checking for duplicates
    if (!method->references) {
        method->references = ref;
    } else {
        // Check for duplicate
        MethodReference* curr = method->references;
        while (curr) {
            if (strcmp(curr->called_in, called_in) == 0) {
                free(ref->called_in);
                free(ref);
                return;
            }
            if (!curr->next) break;
            curr = curr->next;
        }
        curr->next = ref;
    }
    method->reference_count++;
}


// Add the implementation
static bool isType(const char* word, const LanguageGrammar* grammar) {
    if (!word || !grammar || !grammar->types) return false;
    
    logr(DEBUG, "[Analyzer] Checking if '%s' is a type (total types: %zu)", 
         word, grammar->type_count);
    
    for (size_t i = 0; i < grammar->type_count; i++) {
        logr(VERBOSE, "[Analyzer] Comparing '%s' with type '%s'", 
             word, grammar->types[i]);
        if (strcmp(word, grammar->types[i]) == 0) {
            return true;
        }
    }
    return false;
}

static Method* extractMatchingMethod(const char* content, regmatch_t* matches, const LanguageGrammar* grammar) {
    if (!content || !matches || !grammar) return NULL;

    logr(DEBUG, "[Analyzer] Extracting method from match");
    
    // Log the matched content
    size_t decl_len = matches[1].rm_eo - matches[1].rm_so;
    char* declaration = malloc(decl_len + 1);
    strncpy(declaration, content + matches[1].rm_so, decl_len);
    declaration[decl_len] = '\0';
    logr(DEBUG, "[Analyzer] Found potential method declaration: '%s'", declaration);

    Method* method = malloc(sizeof(Method));
    if (!method) {
        logr(ERROR, "[Analyzer] Failed to allocate method structure");
        free(declaration);
        return NULL;
    }
    
    // Initialize method structure
    memset(method, 0, sizeof(Method));
    method->name = NULL;
    method->return_type = NULL;
    method->defined_in = NULL;
    method->dependencies = NULL;
    method->references = NULL;
    method->parameters = NULL;
    method->param_count = 0;
    method->next = NULL;

    // Parse the declaration
    char* word = strtok(declaration, " \t\n(");
    while (word) {
        logr(DEBUG, "[Analyzer] Parsing word: '%s'", word);
        
        // Check if it's a return type
        if (isType(word, grammar)) {
            method->return_type = strdup(word);
            logr(DEBUG, "[Analyzer] Set return type: '%s'", word);
        } 
        // Check if it's the method name
        else if (!method->name) {
            method->name = strdup(word);
            logr(DEBUG, "[Analyzer] Set method name: '%s'", word);
        }
        
        word = strtok(NULL, " \t\n(");
    }

    free(declaration);
    
    // Only return the method if we found at least a name
    if (method->name) {
        logr(DEBUG, "[Analyzer] Successfully created method: %s", method->name);
        return method;
    }

    // Clean up if we didn't find a valid method
    free(method->name);
    free(method->return_type);
    free(method);
    return NULL;
}

// Modify addMethod to check for keywords
static void addMethod(const char* method_name, const char* file_path, const char* return_type, const char* params) {
    if (method_def_count >= MAX_METHOD_DEFS || !method_name || !file_path) return;
    
    // Skip empty method names and keywords
    if (strlen(method_name) == 0) return;
    
    MethodDefinition* def = &method_definitions[method_def_count];
    def->name = strdup(method_name);
    def->defined_in = strdup(file_path);
    def->return_type = return_type ? strdup(return_type) : NULL;
    
    // Parse and store parameters
    if (params) {
        def->parameters = parse_parameters(params);
        // Count parameters
        Parameter* param = def->parameters;
        def->param_count = 0;
        while (param && param->type) {
            def->param_count++;
            param++;
        }
    } else {
        def->parameters = NULL;
        def->param_count = 0;
    }
    
    def->dependencies = NULL;
    def->references = NULL;
    def->reference_count = 0;
    
    method_def_count++;
}

static bool definitionFound(const char* method_name) {
    logr(DEBUG, "[Analyzer] Checking if method '%s' is already defined", method_name);
    
    for (size_t i = 0; i < method_def_count; i++) {
        logr(VERBOSE, "[Analyzer] Comparing with existing method: '%s'", 
             method_definitions[i].name);
        if (strcmp(method_definitions[i].name, method_name) == 0) {
            logr(DEBUG, "[Analyzer] Method '%s' already exists", method_name);
            return true;
        }
    }
    logr(DEBUG, "[Analyzer] Method '%s' is new", method_name);
    return false;
}

// Add this cleanup function
void freeMethod(Method* method) {
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
    freeMethods(method->children);
    free(method);
}

static void updateMethodReferences(const char* file_path, Method* method) {
    if (!method || !method->dependencies) return;
    
    char* deps_copy = strdup(method->dependencies);
    char* dep = strtok(deps_copy, ",");
    
    while (dep) {
        // Trim whitespace
        while (*dep && isspace(*dep)) dep++;
        char* end = dep + strlen(dep) - 1;
        while (end > dep && isspace(*end)) *end-- = '\0';
        
        // Find the called method and update its references
        for (size_t i = 0; i < method_def_count; i++) {
            if (strcmp(method_definitions[i].name, dep) == 0) {
                // Add this file as a reference
                addMethodReference(&method_definitions[i], file_path);
                break;
            }
        }
        dep = strtok(NULL, ",");
    }
    
    free(deps_copy);
}

// Modify collectDefinitions to handle method calls and definitions
void collectDefinitions(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    if (!method_definitions) {
        method_definitions = calloc(MAX_METHOD_DEFS, sizeof(MethodDefinition));
        if (!method_definitions) return;
    }

    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_METHOD);
    if (!patterns) {
        logr(ERROR, "[Analyzer] Failed to get compiled patterns");
        return;
    }

    logr(DEBUG, "[Analyzer] Starting method collection for file: %s", file_path);
    logr(DEBUG, "[Analyzer] Content length: %zu", strlen(content));

    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        const char* pos = content;
        regmatch_t matches[5];

        logr(DEBUG, "[Analyzer] Trying pattern %zu", i);

        while (regexec(regex, pos, 5, matches, 0) == 0) {
            logr(DEBUG, "[Analyzer] Found regex match at position %ld", pos - content);
            
            // Log the matched text
            char match_text[256] = {0};
            size_t match_len = matches[0].rm_eo - matches[0].rm_so;
            strncpy(match_text, pos + matches[0].rm_so, match_len);
            match_text[match_len] = '\0';
            logr(DEBUG, "[Analyzer] Found regex match: '%s'", match_text);

            Method* method = extractMatchingMethod(pos, matches, grammar);
            if (method) {
                if (methodDefinition(pos + matches[0].rm_so, matches[0].rm_eo - matches[0].rm_so)) {
                    // It's a method definition
                    if (!isKeyword(method->name, grammar) && !definitionFound(method->name)) {
                        // Convert parameters to string representation
                        char params_str[1024] = {0};
                        if (method->parameters) {
                            size_t offset = 0;
                            Parameter* param = method->parameters;
                            while (param && param->type) {
                                if (offset > 0) {
                                    offset += snprintf(params_str + offset, sizeof(params_str) - offset, ", ");
                                }
                                offset += snprintf(params_str + offset, sizeof(params_str) - offset, 
                                                "%s %s", 
                                                param->type ? param->type : "",
                                                param->name ? param->name : "");
                                param++;
                            }
                        }
                        
                        addMethod(method->name, file_path, method->return_type, params_str);
                        // Update references for this method
                        updateMethodReferences(file_path, method);
                    }
                } else {
                    // It's a method call
                    MethodDefinition* def = findMethodDefinition(method->name);
                    if (def) {
                        addMethodReference(def, file_path);
                    }
                }
                freeMethod(method);
            }
            pos += matches[0].rm_eo;
        }
    }
}

// Modify analyzeMethod to properly transfer return types and parameters
Method* analyzeMethod(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    if (!content || !grammar) {
        logr(ERROR, "[Analyzer] Invalid parameters for method analysis");
        return NULL;
    }

    Method* head = NULL;
    Method* current = NULL;

    // Get compiled patterns
    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_METHOD);
    if (!patterns) {
        logr(ERROR, "[Analyzer] Failed to get compiled patterns");
        return NULL;
    }
    
    logr(DEBUG, "[Analyzer] Got %zu method patterns for language type %d", 
         patterns->pattern_count, grammar->type);

    // First collect method definitions
    collectDefinitions(file_path, content, grammar);
    
    logr(DEBUG, "[Analyzer] Found %zu method definitions in %s", 
         method_def_count, file_path);

    // Convert method_definitions to Method list
    for (size_t i = 0; i < method_def_count; i++) {
        MethodDefinition* def = &method_definitions[i];
        if (!def->name || !def->defined_in || strcmp(def->defined_in, file_path) != 0) continue;

        Method* method = malloc(sizeof(Method));
        if (!method) continue;

        memset(method, 0, sizeof(Method));
        method->name = strdup(def->name);
        method->defined_in = strdup(def->defined_in);
        method->return_type = def->return_type ? strdup(def->return_type) : NULL;
        
        // Copy parameters
        if (def->parameters) {
            method->param_count = def->param_count;
            method->parameters = calloc(def->param_count + 1, sizeof(Parameter));
            for (int j = 0; j < def->param_count; j++) {
                method->parameters[j].type = def->parameters[j].type ? strdup(def->parameters[j].type) : NULL;
                method->parameters[j].name = def->parameters[j].name ? strdup(def->parameters[j].name) : NULL;
                method->parameters[j].default_value = def->parameters[j].default_value ? 
                    strdup(def->parameters[j].default_value) : NULL;
            }
        }

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

// Add this implementation after the existing functions
ExtractedDependency* analyzeModule(const char* content, const LanguageGrammar* grammar) {
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

    // Process each pattern using the pre-compiled patterns
    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        regmatch_t matches[2];  // [0] full match, [1] module name
        const char* pos = content;

        while (regexec(regex, pos, 2, matches, 0) == 0) {
            // Extract the matched module name
            size_t len = matches[1].rm_eo - matches[1].rm_so;
            char* module_name = malloc(len + 1);
            strncpy(module_name, pos + matches[1].rm_so, len);
            module_name[len] = '\0';

            logr(DEBUG, "[Analyzer] Found module dependency: %s", module_name);

            // Create new dependency node
            ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
            memset(dep, 0, sizeof(ExtractedDependency));
            dep->module_name = module_name;
            dep->target = strdup(module_name);
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
    }

    return head;
}

void freeMethod_references(MethodReference* refs) {
    while (refs) {
        MethodReference* next = refs->next;
        free(refs->called_in);
        free(refs);
        refs = next;
    }
}

// Update the existing cleanup code to use this
void freeMethod_definitions() {
    for (size_t i = 0; i < method_def_count; i++) {
        free(method_definitions[i].name);
        free(method_definitions[i].defined_in);
        free(method_definitions[i].dependencies);
        freeMethod_references(method_definitions[i].references);
    }
    free(method_definitions);
    method_definitions = NULL;
    method_def_count = 0;
}

// Add this implementation
static bool methodDefinition(const char* method_start, size_t len) {
    if (!method_start || len == 0) return false;
    
    // Look for typical method definition indicators
    // 1. Opening brace after the declaration
    const char* end = method_start + len;
    const char* pos = end - 1;
    
    // Skip trailing whitespace
    while (pos > method_start && isspace(*pos)) pos--;
    
    // Check if it ends with an opening brace
    return *pos == '{';
}

// Add this implementation
static MethodDefinition* findMethodDefinition(const char* method_name) {
    if (!method_name) return NULL;
    
    for (size_t i = 0; i < method_def_count; i++) {
        if (strcmp(method_definitions[i].name, method_name) == 0) {
            return &method_definitions[i];
        }
    }
    return NULL;
}
