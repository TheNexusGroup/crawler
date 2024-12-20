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
    
    logr(DEBUG, "[Analyzer] Adding reference to %s from %s", method->name, called_in);
    
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
        bool duplicate = false;
        
        while (curr) {
            if (strcmp(curr->called_in, called_in) == 0) {
                duplicate = true;
                free(ref->called_in);
                free(ref);
                break;
            }
            if (!curr->next) break; // Stop at last node
            curr = curr->next;
        }
        
        // Add if not duplicate
        if (!duplicate) {
            curr->next = ref;
        }
    }
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
    if (!method_definitions) {
        method_definitions = calloc(MAX_METHOD_DEFS, sizeof(MethodDefinition));
        if (!method_definitions) {
            logr(ERROR, "[Analyzer] Failed to allocate method definitions array");
            return;
        }
    }

    if (method_def_count >= MAX_METHOD_DEFS || !method_name || !file_path) {
        logr(ERROR, "[Analyzer] Invalid method addition: %s in %s", method_name, file_path);
        return;
    }
    
    // Skip empty method names and keywords
    if (strlen(method_name) == 0) return;
    
    logr(DEBUG, "[Analyzer] Adding method: %s in %s", method_name, file_path);
    
    MethodDefinition* def = &method_definitions[method_def_count];
    def->name = strdup(method_name);
    def->defined_in = strdup(file_path);
    def->return_type = return_type ? strdup(return_type) : NULL;
    logr(DEBUG, "[Analyzer] Set return type: %s", return_type);

    // Parse and store parameters
    if (params) {
        logr(DEBUG, "[Analyzer] Parsing parameters: %s", params);
        def->parameters = parse_parameters(params);
        // Count parameters
        Parameter* param = def->parameters;
        def->param_count = 0;
        while (param && param->type) {
            def->param_count++;
            param++;
        }
    } else {
        logr(DEBUG, "[Analyzer] No parameters found");
        def->parameters = NULL;
        def->param_count = 0;
    }
    
    def->dependencies = NULL;
    def->references = NULL;
    def->reference_count = 0;
    
    method_def_count++;
    logr(DEBUG, "[Analyzer] Method added successfully: %s in %s", method_name, file_path);
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

// Update methodDefinition to be more accurate
static bool methodDefinition(const char* method_start, size_t len) {
    if (!method_start || len == 0) return false;
    
    logr(DEBUG, "[Analyzer] Checking if this is a method definition: %.20s...", method_start);
    
    // Skip whitespace at start
    while (len > 0 && isspace(*method_start)) {
        method_start++;
        len--;
    }
    
    const char* pos = method_start;
    const char* end = method_start + len;
    bool in_params = false;
    bool found_params = false;
    int brace_count = 0;
    
    while (pos < end) {
        if (*pos == '(') {
            in_params = true;
            found_params = true;
        }
        else if (*pos == ')') {
            in_params = false;
        }
        else if (*pos == '{' && !in_params) {
            brace_count++;
            logr(DEBUG, "[Analyzer] Found opening brace, count: %d", brace_count);
        }
        else if (*pos == '}') {
            brace_count--;
        }
        else if (*pos == ';' && !in_params && brace_count == 0) {
            logr(DEBUG, "[Analyzer] Found semicolon - this is a declaration");
            return false;  // Found semicolon - it's a declaration
        }
        pos++;
    }
    
    bool is_definition = found_params && brace_count > 0;
    logr(DEBUG, "[Analyzer] Method definition check result: %d (params: %d, braces: %d)", 
         is_definition, found_params, brace_count);
    return is_definition;
}

// Add this helper function to store method dependencies
static void addMethodDependency(MethodDefinition* def, const char* dependency) {
    if (!def || !dependency) return;
    
    logr(DEBUG, "[Analyzer] Adding dependency %s to method %s", dependency, def->name);
    
    // Initialize dependencies string if needed
    if (!def->dependencies) {
        def->dependencies = strdup(dependency);
        return;
    }
    
    // Check for existing dependency
    char* deps_copy = strdup(def->dependencies);
    char* dep = strtok(deps_copy, ",");
    while (dep) {
        // Trim whitespace
        while (*dep && isspace(*dep)) dep++;
        char* end = dep + strlen(dep) - 1;
        while (end > dep && isspace(*end)) *end-- = '\0';
        
        if (strcmp(dep, dependency) == 0) {
            free(deps_copy);
            return; // Already exists
        }
        dep = strtok(NULL, ",");
    }
    free(deps_copy);
    
    // Add new dependency
    size_t new_len = strlen(def->dependencies) + strlen(dependency) + 3;
    char* new_deps = malloc(new_len);
    if (new_deps) {
        snprintf(new_deps, new_len, "%s,%s", def->dependencies, dependency);
        free(def->dependencies);
        def->dependencies = new_deps;
    }
}

// Update collectDefinitions to track dependencies
void collectDefinitions(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    if (!method_definitions) {
        method_definitions = calloc(MAX_METHOD_DEFS, sizeof(MethodDefinition));
        if (!method_definitions) return;
    }

    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_METHOD);
    if (!patterns) return;

    MethodDefinition* current_method = NULL;
    int brace_count = 0;
    const char* method_start = NULL;

    // First pass: collect all method definitions
    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        const char* pos = content;
        regmatch_t matches[5];

        while (regexec(regex, pos, 5, matches, 0) == 0) {
            Method* method = extractMatchingMethod(pos, matches, grammar);
            if (method && !isKeyword(method->name, grammar)) {
                if (methodDefinition(pos + matches[0].rm_so, matches[0].rm_eo - matches[0].rm_so)) {
                    // Found a method definition
                    if (!definitionFound(method->name)) {
                        addMethod(method->name, file_path, method->return_type, NULL);
                    }
                    current_method = findMethodDefinition(method->name);
                    method_start = pos + matches[0].rm_so;
                }
            }
            freeMethod(method);
            pos += matches[0].rm_eo;
        }
    }

    // Second pass: collect method calls within each definition
    for (size_t i = 0; i < method_def_count; i++) {
        current_method = &method_definitions[i];
        if (strcmp(current_method->defined_in, file_path) != 0) continue;

        // Find the method's bounds in the source
        const char* method_start = strstr(content, current_method->name);
        if (!method_start) continue;

        // Find opening brace
        const char* pos = method_start;
        while (*pos && *pos != '{') pos++;
        if (!*pos) continue;

        // Find closing brace (matching)
        const char* end = pos + 1;
        brace_count = 1;
        while (*end && brace_count > 0) {
            if (*end == '{') brace_count++;
            if (*end == '}') brace_count--;
            end++;
        }

        // Now look for method calls within these bounds
        for (size_t j = 0; j < patterns->pattern_count; j++) {
            regex_t* regex = &patterns->compiled_patterns[j];
            const char* call_pos = pos;
            regmatch_t matches[5];

            while (call_pos < end && regexec(regex, call_pos, 5, matches, 0) == 0) {
                Method* called = extractMatchingMethod(call_pos, matches, grammar);
                if (called && !isKeyword(called->name, grammar) && 
                    !methodDefinition(call_pos + matches[0].rm_so, matches[0].rm_eo - matches[0].rm_so)) {
                    
                    // Add dependency to current method
                    addMethodDependency(current_method, called->name);
                    
                    // Add reference from called method back to current method
                    MethodDefinition* called_def = findMethodDefinition(called->name);
                    if (called_def) {
                        addMethodReference(called_def, current_method->name);
                        logr(DEBUG, "[Analyzer] Added reference: %s is called by %s", 
                             called->name, current_method->name);
                    }
                }
                freeMethod(called);
                call_pos += matches[0].rm_eo;
            }
        }
    }
}

// Add this helper function to format method signature
char* formatMethodSignature(Method* method) {
    char* signature = malloc(256);  // Adjust size as needed
    if (!signature) return NULL;
    
    if (method->return_type) {
        snprintf(signature, 256, "%s() -> %s", method->name, method->return_type);
    } else {
        snprintf(signature, 256, "%s()", method->name);
    }
    return signature;
}

// Modify analyzeMethod to properly track relationships
Method* analyzeMethod(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    Method* head = NULL;
    Method* current = NULL;
    MethodDefinition* current_method_def = NULL;

    // Get compiled patterns
    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_METHOD);
    if (!patterns) {
        logr(ERROR, "[Analyzer] Failed to get compiled patterns");
        return NULL;
    }

    // First pass: find all method definitions
    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        const char* pos = content;
        regmatch_t matches[5];

        while (regexec(regex, pos, 5, matches, 0) == 0) {
            Method* method = extractMatchingMethod(pos, matches, grammar);
            if (method && !isKeyword(method->name, grammar)) {
                if (methodDefinition(pos + matches[0].rm_so, matches[0].rm_eo - matches[0].rm_so)) {
                    // It's a method definition
                    method->is_definition = true;
                    method->defined_in = strdup(file_path);
                    
                    if (!head) {
                        head = method;
                        current = method;
                    } else {
                        current->next = method;
                        current = method;
                    }
                    
                    if (!definitionFound(method->name)) {
                        addMethod(method->name, file_path, method->return_type, NULL);
                    }
                    current_method_def = findMethodDefinition(method->name);
                    
                    // Update method references for the definition
                    updateMethodReferences(file_path, method);
                } else {
                    // It's a method call - add reference
                    MethodDefinition* called_method = findMethodDefinition(method->name);
                    if (called_method && current_method_def) {
                        addMethodReference(called_method, current_method_def->name);
                    }
                }
            } else {
                freeMethod(method);
            }
            pos += matches[0].rm_eo;
        }
    }

    // Second pass: find all method calls within each definition
    Method* def = head;
    while (def) {
        current_method_def = findMethodDefinition(def->name);
        if (!current_method_def) {
            def = def->next;
            continue;
        }

        // Reset position for scanning calls within this definition
        for (size_t i = 0; i < patterns->pattern_count; i++) {
            regex_t* regex = &patterns->compiled_patterns[i];
            const char* pos = content;
            regmatch_t matches[5];

            while (regexec(regex, pos, 5, matches, 0) == 0) {
                Method* called = extractMatchingMethod(pos, matches, grammar);
                if (called && !isKeyword(called->name, grammar) && 
                    !methodDefinition(pos + matches[0].rm_so, matches[0].rm_eo - matches[0].rm_so)) {
                    
                    // Add the call to current method's dependencies
                    addMethodDependency(current_method_def, called->name);
                    
                    // Add reference from called method back to current method
                    MethodDefinition* called_def = findMethodDefinition(called->name);
                    if (called_def) {
                        addMethodReference(called_def, current_method_def->name);
                    }
                    
                    freeMethod(called);
                } else {
                    freeMethod(called);
                }
                pos += matches[0].rm_eo;
            }
        }
        def = def->next;
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
MethodDefinition* findMethodDefinition(const char* method_name) {
    if (!method_name) return NULL;
    
    for (size_t i = 0; i < method_def_count; i++) {
        if (strcmp(method_definitions[i].name, method_name) == 0) {
            return &method_definitions[i];
        }
    }
    return NULL;
}

int countMethods(Method* methods) {
    int count = 0;
    Method* current = methods;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}