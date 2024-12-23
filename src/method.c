#include "method.h"
#include "logger.h"
#include "pattern_cache.h"

#define MAX_METHOD_DEFS 1024
MethodDefinition* method_definitions = NULL;
size_t method_def_count = 0;

// In printDependencies function
void printMethods(Method* method) {
    while (method) {
        bool is_last_method = (method->next == NULL);
        const char* method_prefix = is_last_method ? "└──" : "├──";
        
        char* signature = formatMethodSignature(method);
        logr(INFO, "  %s %s -> %s", method_prefix, signature, method->return_type);
        free(signature);
        
        // Print methods this method calls
        MethodDefinition* def = findMethodDefinition(method->name);
        if (def && def->dependencies) {
            const char* dep_header_prefix = is_last_method ? "    " : "│   ";
            const char* calls_prefix = def->reference_count > 0 ? "├──" : "└──";
            logr(INFO, "  %s %s calls:", dep_header_prefix, calls_prefix);
            
            MethodDependency* dep = def->dependencies;
            
            while (dep) {
                const char* dep_prefix = (dep->next == NULL) ? "└──" : "├──";
                const char* ref_prefix = def->reference_count > 0 ? "│   " : "    ";
                logr(INFO, "  %s %s %s %s", dep_header_prefix, ref_prefix, dep_prefix, dep->name);
                dep = dep->next;
            }
        }
        
        // Print methods that call this method
        if (def && def->references) {
            const char* ref_header_prefix = is_last_method ? "    " : "│   ";
            logr(INFO, "  %s └── called by:", ref_header_prefix);
            
            MethodReference* ref = def->references;
            int ref_count = 0;
            MethodReference* count_ref = ref;
            
            // Count references first
            while (count_ref) {
                if (count_ref->called_in) ref_count++;
                count_ref = count_ref->next;
            }
            
            // Print references
            int current_ref = 0;
            while (ref) {
                if (ref->called_in) {
                    current_ref++;
                    const char* ref_prefix = (current_ref == ref_count) ? "└──" : "├──";
                    logr(INFO, "  %s       %s %s", ref_header_prefix, ref_prefix, ref->called_in);
                }
                ref = ref->next;
            }
        }
        
        method = method->next;
    }
}


// Modify the dependency graph creation
void graphMethods(DependencyCrawler* crawler, const char* file_path, Method* methods) {
    if (!crawler || !file_path || !methods) return;
    
    logr(VERBOSE, "[Crawler] Adding methods to dependency graph from %s", file_path);
    
    Method* current = methods;
    while (current) {
        // Create new dependency node for each method
        Dependency* dep = malloc(sizeof(Dependency));
        if (!dep) {
            logr(ERROR, "[Crawler] Failed to allocate memory for dependency");
            return;
        }
        
        memset(dep, 0, sizeof(Dependency));
        dep->source = strdup(file_path);
        dep->level = LAYER_METHOD;
        
        // Create a new Method structure just for this method
        Method* single_method = malloc(sizeof(Method));
        if (!single_method) {
            free(dep->source);
            free(dep);
            return;
        }
        
        // Copy just this method's data
        memset(single_method, 0, sizeof(Method));
        single_method->name = current->name ? strdup(current->name) : NULL;
        single_method->return_type = current->return_type ? strdup(current->return_type) : NULL;
        single_method->dependencies = current->dependencies ? strdup(current->dependencies) : NULL;
        single_method->defined_in = current->defined_in ? strdup(current->defined_in) : NULL;
        single_method->references = current->references; // Keep reference to original references
        single_method->next = NULL; // Important: break the chain
        
        dep->methods = single_method;
        dep->method_count = 1; // Each dependency now has exactly one method
        
        // Add to graph
        dep->next = crawler->dependency_graph;
        crawler->dependency_graph = dep;
        
        logr(DEBUG, "[Crawler] Added method %s from %s to dependency graph", 
             current->name ? current->name : "unknown", file_path);
             
        current = current->next;
    }
}

void freeMethods(Method* methods) {
    while (methods) {
        Method* next = methods->next;
        
        // Free method name and return type
        free(methods->name);
        free(methods->return_type);
        
        // Free parameters
        for (int i = 0; i < methods->param_count; i++) {
            free(methods->parameters[i].name);
            free(methods->parameters[i].type);
            free(methods->parameters[i].default_value);
        }
        
        // Free dependencies
        free(methods->dependencies);
        
        // Free the method itself
        free(methods);
        methods = next;
    }
}

void freeMethodReferences(MethodReference* refs) {
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
        freeMethodDependencies(method_definitions[i].dependencies);
        freeMethodReferences(method_definitions[i].references);
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


// Add this helper function to format method signature
char* formatMethodSignature(Method* method) {
    char* signature = malloc(256);  // Adjust size as needed
    if (!signature) return NULL;
    
    if (method->return_type) {
        snprintf(signature, 256, "%s(%p) -> %s", method->name, method->parameters, method->return_type);
    } else {
        snprintf(signature, 256, "%s(%p)", method->name, method->parameters);
    }
    return signature;
}

// Modify analyzeMethod to properly track relationships
Method* analyzeMethod(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    Method* head = NULL;
    Method* current = NULL;
    
    // Get compiled patterns
    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_METHOD);
    if (!patterns) {
        logr(ERROR, "[Analyzer] Failed to get compiled patterns");
        return NULL;
    }

    // First pass: collect all method definitions
    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        const char* pos = content;
        regmatch_t matches[5];

        while (regexec(regex, pos, 5, matches, 0) == 0) {
            Method* method = extractMatchingMethod(pos, matches, grammar);
            if (!method) {
                pos += matches[0].rm_eo;
                continue;
            }

            if (!isKeyword(method->name, grammar)) {
                if (methodDefinition(pos + matches[0].rm_so, matches[0].rm_eo - matches[0].rm_so)) {
                    // Add to linked list
                    if (!head) {
                        head = method;
                        current = method;
                    } else {
                        current->next = method;
                        current = method;
                    }

                    // Register method definition
                    if (!definitionFound(method->name)) {
                        addMethod(method->name, file_path, method->return_type);
                    }
                    
                    // Get the method definition and clear its existing dependencies/references
                    MethodDefinition* current_def = findMethodDefinition(method->name);
                    if (current_def) {
                        // Clear existing dependencies and references
                        freeMethodDependencies(current_def->dependencies);
                        current_def->dependencies = NULL;
                        
                        // Find and process method body
                        const char* body_start = findMethodBody(pos + matches[0].rm_so);
                        if (body_start) {
                            scanMethodBodyForCalls(body_start, current_def, grammar);
                        }
                    }
                    
                    pos += matches[0].rm_eo;
                    continue;
                }
            }
            freeMethod(method);
            pos += matches[0].rm_eo;
        }
    }

    return head;
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
    
    logr(VERBOSE, "[Analyzer] Checking if this is a method definition: %.20s...", method_start);
    
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
static void addMethodDependency(MethodDefinition* method, const char* dep_name) {
    if (!method || !dep_name) return;
    
    // Create new dependency
    MethodDependency* dep = malloc(sizeof(MethodDependency));
    if (!dep) return;
    
    dep->name = strdup(dep_name);
    dep->next = NULL;
    
    // Add to list, checking for duplicates
    if (!method->dependencies) {
        method->dependencies = dep;
    } else {
        // Check for duplicate
        MethodDependency* curr = method->dependencies;
        bool duplicate = false;
        
        while (curr) {
            if (strcmp(curr->name, dep_name) == 0) {
                duplicate = true;
                free(dep->name);
                free(dep);
                break;
            }
            if (!curr->next) break;
            curr = curr->next;
        }
        
        // Add if not duplicate
        if (!duplicate) {
            curr->next = dep;
        }
    }
}

// Update the cleanup function
static void freeMethodDependencies(MethodDependency* deps) {
    while (deps) {
        MethodDependency* next = deps->next;
        free(deps->name);
        free(deps);
        deps = next;
    }
}

// Fix createMethodFromDefinition to handle the new MethodDependency structure
static Method* createMethodFromDefinition(MethodDefinition* def) {
    if (!def) return NULL;
    
    Method* method = malloc(sizeof(Method));
    if (!method) return NULL;
    
    memset(method, 0, sizeof(Method));
    method->name = def->name ? strdup(def->name) : NULL;
    method->return_type = def->return_type ? strdup(def->return_type) : NULL;
    method->defined_in = def->defined_in ? strdup(def->defined_in) : NULL;
    
    // Convert linked list of dependencies to comma-separated string
    if (def->dependencies) {
        size_t dep_len = 0;
        MethodDependency* curr = def->dependencies;
        
        // Calculate total length needed
        while (curr) {
            dep_len += strlen(curr->name) + 2; // +2 for ", "
            curr = curr->next;
        }
        
        method->dependencies = malloc(dep_len + 1);
        if (method->dependencies) {
            method->dependencies[0] = '\0';
            curr = def->dependencies;
            while (curr) {
                strcat(method->dependencies, curr->name);
                if (curr->next) strcat(method->dependencies, ", ");
                curr = curr->next;
            }
        }
    } else {
        method->dependencies = NULL;
    }
    
    method->param_count = def->param_count;
    return method;
}

static const char* findMethodBody(const char* start) {
    if (!start) return NULL;
    
    // Skip to opening brace
    while (*start && *start != '{') start++;
    return start;
}

static void scanMethodBodyForCalls(const char* body_start, MethodDefinition* method, 
                                 const LanguageGrammar* grammar) {
    if (!body_start || !method || !grammar) return;
    
    // Clear existing dependencies before scanning
    freeMethodDependencies(method->dependencies);
    method->dependencies = NULL;
    
    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_METHOD);
    if (!patterns) return;
    
    char** unique_deps = NULL;
    size_t unique_count = 0;
    
    const char* pos = body_start;
    
    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        regmatch_t matches[2];  // We need 2 slots: full match and the method name
        
        while (regexec(regex, pos, 2, matches, 0) == 0) {
            size_t len = matches[1].rm_eo - matches[1].rm_so;
            char* called_method = malloc(len + 1);
            if (!called_method) continue;
            
            strncpy(called_method, pos + matches[1].rm_so, len);
            called_method[len] = '\0';
            
            // Only add if it's not a keyword and not the method itself
            if (!isKeyword(called_method, grammar) && 
                strcmp(called_method, method->name) != 0) {
                
                // Check for duplicates
                bool already_added = false;
                for (size_t j = 0; j < unique_count; j++) {
                    if (strcmp(unique_deps[j], called_method) == 0) {
                        already_added = true;
                        break;
                    }
                }
                
                if (!already_added) {
                    char** new_deps = realloc(unique_deps, (unique_count + 1) * sizeof(char*));
                    if (new_deps) {
                        unique_deps = new_deps;
                        unique_deps[unique_count] = strdup(called_method);
                        unique_count++;
                        
                        addMethodDependency(method, called_method);
                    }
                }
            }
            free(called_method);
            pos += matches[0].rm_eo;
        }
    }
    
    // Clean up
    for (size_t i = 0; i < unique_count; i++) {
        free(unique_deps[i]);
    }
    free(unique_deps);
}


static Method* extractMatchingMethod(const char* content, regmatch_t* matches, const LanguageGrammar* grammar) {
    if (!content || !matches || !grammar) return NULL;

    logr(VERBOSE, "[Analyzer] Extracting method from match");
    
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
            logr(VERBOSE, "[Analyzer] Set method name: '%s'", word);
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
static void addMethod(const char* method_name, const char* file_path, const char* return_type) {
    if (!method_definitions) {
        method_definitions = calloc(MAX_METHOD_DEFS, sizeof(MethodDefinition));
        if (!method_definitions) {
            logr(ERROR, "[Analyzer] Failed to allocate method definitions array");
            return;
        }
    }

    if (method_def_count >= MAX_METHOD_DEFS || !method_name || !file_path) return;
    
    // Check if method already exists
    for (size_t i = 0; i < method_def_count; i++) {
        if (strcmp(method_definitions[i].name, method_name) == 0) {
            // Method already exists, don't add it again
            logr(WARN, "[Analyzer] Method '%s' already exists", method_name);
            return;
        }
    }
    
    logr(WARN, "[Analyzer] Creating method '%s' definition", method_name);

    MethodDefinition* def = &method_definitions[method_def_count];
    def->name = strdup(method_name);
    def->defined_in = strdup(file_path);
    def->return_type = return_type ? strdup(return_type) : NULL;
    def->dependencies = NULL;  // Initialize empty dependency list
    def->references = NULL;   // Initialize empty reference list
    def->param_count = 0;
    
    method_def_count++;
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


#define MIN(a,b) ((a) < (b) ? (a) : (b))

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

// Add the implementation
static bool isType(const char* word, const LanguageGrammar* grammar) {
    if (!word || !grammar || !grammar->types) return false;
    
    logr(VERBOSE, "[Analyzer] Checking if '%s' is a type (total types: %zu)", 
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

static bool definitionFound(const char* method_name) {
    logr(DEBUG, "[Analyzer] Checking if method '%s' is already defined", method_name);
    
    for (size_t i = 0; i < method_def_count; i++) {
        logr(VERBOSE, "[Analyzer] Comparing with existing method: '%s'", 
             method_definitions[i].name);
        if (strcmp(method_definitions[i].name, method_name) == 0) {
            logr(WARN, "[Analyzer] Method '%s' already exists", method_name);
            return true;
        }
    }
    logr(WARN, "[Analyzer] Method '%s' is new", method_name);
    return false;
}


// Update collectDefinitions to use method_body_start and method_body_end
void collectDefinitions(const char* file_path, const char* content, const LanguageGrammar* grammar) {
    if (!method_definitions) {
        method_definitions = calloc(MAX_METHOD_DEFS, sizeof(MethodDefinition));
        if (!method_definitions) return;
    }

    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_METHOD);
    if (!patterns) return;

    MethodDefinition* current_method = NULL;
    
    // First pass: collect all method definitions
    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        const char* pos = content;
        regmatch_t matches[5];

        while (regexec(regex, pos, 5, matches, 0) == 0) {
            Method* method = extractMatchingMethod(pos, matches, grammar);
            if (method && !isKeyword(method->name, grammar)) {
                if (methodDefinition(pos + matches[0].rm_so, matches[0].rm_eo - matches[0].rm_so)) {
                    // Add or update method definition
                    if (!definitionFound(method->name)) {
                        addMethod(method->name, file_path, method->return_type);
                    }
                    
                    // Get the method definition and clear its existing dependencies/references
                    current_method = findMethodDefinition(method->name);
                    if (current_method) {
                        // Clear existing dependencies and references
                        freeMethodDependencies(current_method->dependencies);
                        current_method->dependencies = NULL;
                        freeMethodReferences(current_method->references);
                        current_method->references = NULL;
                        
                        // Find and process method body
                        const char* body_start = findMethodBody(pos + matches[0].rm_so);
                        if (body_start) {
                            scanMethodBodyForCalls(body_start, current_method, grammar);
                        }
                    }
                }
            }
            freeMethod(method);
            pos += matches[0].rm_eo;
        }
    }

    // Second pass: update references based on dependencies
    for (size_t i = 0; i < method_def_count; i++) {
        current_method = &method_definitions[i];
        if (strcmp(current_method->defined_in, file_path) != 0) continue;
        
        Method* temp_method = createMethodFromDefinition(current_method);
        updateMethodReferences(file_path, temp_method);
        freeMethod(temp_method);
    }
}