#include "language_analyzers.h"
#include "pattern_cache.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>

// Create language analysis context
LanguageContext* language_context_create(LanguageType type, const char* content, const char* file_path, const LanguageGrammar* grammar) {
    if (!content || !file_path || !grammar) return NULL;

    LanguageContext* context = malloc(sizeof(LanguageContext));
    if (!context) return NULL;

    memset(context, 0, sizeof(LanguageContext));
    context->type = type;
    context->file_path = file_path;
    context->content = content;
    context->content_length = strlen(content);
    context->grammar = grammar;
    context->current_line = 1;
    context->current_column = 1;

    return context;
}

void language_context_destroy(LanguageContext* context) {
    if (!context) return;

    free(context->current_namespace);
    free(context->current_class);
    free(context);
}

// Rust analyzer implementation
ExtractedDependency* analyze_rust(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    logr(DEBUG, "[RustAnalyzer] Analyzing file: %s", file_path);

    LanguageContext* context = language_context_create(LANG_RUST, content, file_path, grammar);
    if (!context) return NULL;

    ExtractedDependency* dependencies = NULL;
    ExtractedDependency* imports = extract_imports(context, LAYER_MODULE);
    ExtractedDependency* structures = extract_structures(context);
    ExtractedDependency* methods = extract_methods(context);

    // Chain dependencies together
    dependencies = imports;
    if (dependencies) {
        ExtractedDependency* current = dependencies;
        while (current->next) current = current->next;
        current->next = structures;
    } else {
        dependencies = structures;
    }

    if (dependencies) {
        ExtractedDependency* current = dependencies;
        while (current->next) current = current->next;
        current->next = methods;
    } else if (!imports) {
        dependencies = methods;
    }

    language_context_destroy(context);
    logr(DEBUG, "[RustAnalyzer] Analysis complete");
    return dependencies;
}

// JavaScript/TypeScript analyzer implementation
ExtractedDependency* analyze_javascript(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    logr(DEBUG, "[JSAnalyzer] Analyzing file: %s", file_path);

    LanguageContext* context = language_context_create(LANG_JAVASCRIPT, content, file_path, grammar);
    if (!context) return NULL;

    ExtractedDependency* dependencies = NULL;

    // Analyze ES6 imports and CommonJS requires
    const CompiledPatterns* patterns = compiledPatterns(LANG_JAVASCRIPT, LAYER_MODULE);
    if (patterns) {
        const char* pos = content;
        
        for (size_t i = 0; i < patterns->pattern_count; i++) {
            regex_t* regex = &patterns->compiled_patterns[i];
            regmatch_t matches[3];  // full match, import path, optional name

            pos = content; // Reset position for each pattern
            while (regexec(regex, pos, 3, matches, 0) == 0) {
                size_t path_len = matches[1].rm_eo - matches[1].rm_so;
                char* import_path = malloc(path_len + 1);
                strncpy(import_path, pos + matches[1].rm_so, path_len);
                import_path[path_len] = '\0';

                // Skip relative paths starting with . or /
                if (import_path[0] != '.' && import_path[0] != '/') {
                    ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                    memset(dep, 0, sizeof(ExtractedDependency));
                    
                    dep->module_name = import_path;
                    dep->target = strdup(import_path);
                    dep->file_path = strdup(file_path);
                    dep->layer = LAYER_MODULE;
                    dep->next = dependencies;
                    dependencies = dep;

                    logr(VERBOSE, "[JSAnalyzer] Found import: %s", import_path);
                } else {
                    free(import_path);
                }

                pos += matches[0].rm_eo;
            }
        }
    }

    // Analyze class definitions
    const CompiledPatterns* struct_patterns = compiledPatterns(LANG_JAVASCRIPT, LAYER_STRUCT);
    if (struct_patterns) {
        const char* pos = content;
        
        for (size_t i = 0; i < struct_patterns->pattern_count; i++) {
            regex_t* regex = &struct_patterns->compiled_patterns[i];
            regmatch_t matches[2];  // full match, class name

            pos = content;
            while (regexec(regex, pos, 2, matches, 0) == 0) {
                size_t name_len = matches[1].rm_eo - matches[1].rm_so;
                char* class_name = malloc(name_len + 1);
                strncpy(class_name, pos + matches[1].rm_so, name_len);
                class_name[name_len] = '\0';

                ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                memset(dep, 0, sizeof(ExtractedDependency));
                
                dep->module_name = class_name;
                dep->target = strdup(class_name);
                dep->file_path = strdup(file_path);
                dep->layer = LAYER_STRUCT;
                dep->next = dependencies;
                dependencies = dep;

                logr(VERBOSE, "[JSAnalyzer] Found class: %s", class_name);
                pos += matches[0].rm_eo;
            }
        }
    }

    language_context_destroy(context);
    logr(DEBUG, "[JSAnalyzer] Analysis complete");
    return dependencies;
}

// Python analyzer implementation
ExtractedDependency* analyze_python(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    logr(DEBUG, "[PythonAnalyzer] Analyzing file: %s", file_path);

    LanguageContext* context = language_context_create(LANG_PYTHON, content, file_path, grammar);
    if (!context) return NULL;

    ExtractedDependency* dependencies = NULL;

    // Analyze imports
    const CompiledPatterns* patterns = compiledPatterns(LANG_PYTHON, LAYER_MODULE);
    if (patterns) {
        const char* pos = content;
        
        for (size_t i = 0; i < patterns->pattern_count; i++) {
            regex_t* regex = &patterns->compiled_patterns[i];
            regmatch_t matches[2];

            pos = content;
            while (regexec(regex, pos, 2, matches, 0) == 0) {
                size_t name_len = matches[1].rm_eo - matches[1].rm_so;
                char* module_name = malloc(name_len + 1);
                strncpy(module_name, pos + matches[1].rm_so, name_len);
                module_name[name_len] = '\0';

                // Skip built-in modules
                if (!is_python_builtin(module_name)) {
                    ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                    memset(dep, 0, sizeof(ExtractedDependency));
                    
                    dep->module_name = module_name;
                    dep->target = strdup(module_name);
                    dep->file_path = strdup(file_path);
                    dep->layer = LAYER_MODULE;
                    dep->next = dependencies;
                    dependencies = dep;

                    logr(VERBOSE, "[PythonAnalyzer] Found import: %s", module_name);
                } else {
                    free(module_name);
                }

                pos += matches[0].rm_eo;
            }
        }
    }

    // Analyze class definitions
    const CompiledPatterns* struct_patterns = compiledPatterns(LANG_PYTHON, LAYER_STRUCT);
    if (struct_patterns) {
        const char* pos = content;
        
        for (size_t i = 0; i < struct_patterns->pattern_count; i++) {
            regex_t* regex = &struct_patterns->compiled_patterns[i];
            regmatch_t matches[2];

            pos = content;
            while (regexec(regex, pos, 2, matches, 0) == 0) {
                size_t name_len = matches[1].rm_eo - matches[1].rm_so;
                char* class_name = malloc(name_len + 1);
                strncpy(class_name, pos + matches[1].rm_so, name_len);
                class_name[name_len] = '\0';

                ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                memset(dep, 0, sizeof(ExtractedDependency));
                
                dep->module_name = class_name;
                dep->target = strdup(class_name);
                dep->file_path = strdup(file_path);
                dep->layer = LAYER_STRUCT;
                dep->next = dependencies;
                dependencies = dep;

                logr(VERBOSE, "[PythonAnalyzer] Found class: %s", class_name);
                pos += matches[0].rm_eo;
            }
        }
    }

    language_context_destroy(context);
    logr(DEBUG, "[PythonAnalyzer] Analysis complete");
    return dependencies;
}

// Java analyzer implementation
ExtractedDependency* analyze_java(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    logr(DEBUG, "[JavaAnalyzer] Analyzing file: %s", file_path);

    LanguageContext* context = language_context_create(LANG_JAVA, content, file_path, grammar);
    if (!context) return NULL;

    ExtractedDependency* dependencies = NULL;

    // Analyze imports
    const CompiledPatterns* patterns = compiledPatterns(LANG_JAVA, LAYER_MODULE);
    if (patterns) {
        const char* pos = content;
        
        for (size_t i = 0; i < patterns->pattern_count; i++) {
            regex_t* regex = &patterns->compiled_patterns[i];
            regmatch_t matches[2];

            pos = content;
            while (regexec(regex, pos, 2, matches, 0) == 0) {
                size_t name_len = matches[1].rm_eo - matches[1].rm_so;
                char* import_name = malloc(name_len + 1);
                strncpy(import_name, pos + matches[1].rm_so, name_len);
                import_name[name_len] = '\0';

                // Skip java.* standard library imports unless explicitly requested
                if (strncmp(import_name, "java.", 5) != 0 || !is_java_std_library(import_name)) {
                    ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                    memset(dep, 0, sizeof(ExtractedDependency));
                    
                    dep->module_name = import_name;
                    dep->target = strdup(import_name);
                    dep->file_path = strdup(file_path);
                    dep->layer = LAYER_MODULE;
                    dep->next = dependencies;
                    dependencies = dep;

                    logr(VERBOSE, "[JavaAnalyzer] Found import: %s", import_name);
                } else {
                    free(import_name);
                }

                pos += matches[0].rm_eo;
            }
        }
    }

    language_context_destroy(context);
    logr(DEBUG, "[JavaAnalyzer] Analysis complete");
    return dependencies;
}

// Go analyzer implementation
ExtractedDependency* analyze_go(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    logr(DEBUG, "[GoAnalyzer] Analyzing file: %s", file_path);

    LanguageContext* context = language_context_create(LANG_GO, content, file_path, grammar);
    if (!context) return NULL;

    ExtractedDependency* dependencies = NULL;

    // Analyze imports (Go has a unique import syntax)
    const CompiledPatterns* patterns = compiledPatterns(LANG_GO, LAYER_MODULE);
    if (patterns) {
        const char* pos = content;
        
        // Look for import blocks
        regex_t import_block_regex;
        if (regcomp(&import_block_regex, "import\\s*\\(([^)]+)\\)", REG_EXTENDED) == 0) {
            regmatch_t matches[2];
            
            while (regexec(&import_block_regex, pos, 2, matches, 0) == 0) {
                size_t block_len = matches[1].rm_eo - matches[1].rm_so;
                char* import_block = malloc(block_len + 1);
                strncpy(import_block, pos + matches[1].rm_so, block_len);
                import_block[block_len] = '\0';

                // Parse individual imports from the block
                char* line = strtok(import_block, "\n");
                while (line) {
                    // Remove quotes and whitespace
                    while (*line && (*line == ' ' || *line == '\t' || *line == '"')) line++;
                    char* end = line + strlen(line) - 1;
                    while (end > line && (*end == ' ' || *end == '\t' || *end == '"')) end--;
                    *(end + 1) = '\0';

                    if (strlen(line) > 0 && !is_go_std_library(line)) {
                        ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                        memset(dep, 0, sizeof(ExtractedDependency));
                        
                        dep->module_name = strdup(line);
                        dep->target = strdup(line);
                        dep->file_path = strdup(file_path);
                        dep->layer = LAYER_MODULE;
                        dep->next = dependencies;
                        dependencies = dep;

                        logr(VERBOSE, "[GoAnalyzer] Found import: %s", line);
                    }

                    line = strtok(NULL, "\n");
                }

                free(import_block);
                pos += matches[0].rm_eo;
            }
            
            regfree(&import_block_regex);
        }

        // Also check for single-line imports
        for (size_t i = 0; i < patterns->pattern_count; i++) {
            regex_t* regex = &patterns->compiled_patterns[i];
            regmatch_t matches[2];

            pos = content;
            while (regexec(regex, pos, 2, matches, 0) == 0) {
                size_t name_len = matches[1].rm_eo - matches[1].rm_so;
                char* import_name = malloc(name_len + 1);
                strncpy(import_name, pos + matches[1].rm_so, name_len);
                import_name[name_len] = '\0';

                // Remove quotes
                char* clean_name = import_name;
                if (*clean_name == '"') clean_name++;
                char* end = clean_name + strlen(clean_name) - 1;
                if (*end == '"') *end = '\0';

                if (!is_go_std_library(clean_name)) {
                    ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                    memset(dep, 0, sizeof(ExtractedDependency));
                    
                    dep->module_name = strdup(clean_name);
                    dep->target = strdup(clean_name);
                    dep->file_path = strdup(file_path);
                    dep->layer = LAYER_MODULE;
                    dep->next = dependencies;
                    dependencies = dep;

                    logr(VERBOSE, "[GoAnalyzer] Found import: %s", clean_name);
                }

                free(import_name);
                pos += matches[0].rm_eo;
            }
        }
    }

    language_context_destroy(context);
    logr(DEBUG, "[GoAnalyzer] Analysis complete");
    return dependencies;
}

// C/C++ analyzer implementation
ExtractedDependency* analyze_c_cpp(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    logr(DEBUG, "[C/C++Analyzer] Analyzing file: %s", file_path);

    LanguageContext* context = language_context_create(LANG_C, content, file_path, grammar);
    if (!context) return NULL;

    ExtractedDependency* dependencies = NULL;

    // Analyze includes
    const CompiledPatterns* patterns = compiledPatterns(LANG_C, LAYER_MODULE);
    if (patterns) {
        const char* pos = content;
        
        for (size_t i = 0; i < patterns->pattern_count; i++) {
            regex_t* regex = &patterns->compiled_patterns[i];
            regmatch_t matches[2];

            pos = content;
            while (regexec(regex, pos, 2, matches, 0) == 0) {
                size_t name_len = matches[1].rm_eo - matches[1].rm_so;
                char* header_name = malloc(name_len + 1);
                strncpy(header_name, pos + matches[1].rm_so, name_len);
                header_name[name_len] = '\0';

                // Skip standard library headers unless explicitly requested
                if (!is_c_std_library(header_name)) {
                    ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                    memset(dep, 0, sizeof(ExtractedDependency));
                    
                    dep->module_name = header_name;
                    dep->target = strdup(header_name);
                    dep->file_path = strdup(file_path);
                    dep->layer = LAYER_MODULE;
                    dep->next = dependencies;
                    dependencies = dep;

                    logr(VERBOSE, "[C/C++Analyzer] Found include: %s", header_name);
                } else {
                    free(header_name);
                }

                pos += matches[0].rm_eo;
            }
        }
    }

    language_context_destroy(context);
    logr(DEBUG, "[C/C++Analyzer] Analysis complete");
    return dependencies;
}

// PHP analyzer implementation
ExtractedDependency* analyze_php(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    logr(DEBUG, "[PHPAnalyzer] Analyzing file: %s", file_path);

    LanguageContext* context = language_context_create(LANG_PHP, content, file_path, grammar);
    if (!context) return NULL;

    ExtractedDependency* dependencies = NULL;

    // Analyze includes/requires and use statements
    const CompiledPatterns* patterns = compiledPatterns(LANG_PHP, LAYER_MODULE);
    if (patterns) {
        const char* pos = content;
        
        for (size_t i = 0; i < patterns->pattern_count; i++) {
            regex_t* regex = &patterns->compiled_patterns[i];
            regmatch_t matches[2];

            pos = content;
            while (regexec(regex, pos, 2, matches, 0) == 0) {
                size_t name_len = matches[1].rm_eo - matches[1].rm_so;
                char* dependency_name = malloc(name_len + 1);
                strncpy(dependency_name, pos + matches[1].rm_so, name_len);
                dependency_name[name_len] = '\0';

                ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                memset(dep, 0, sizeof(ExtractedDependency));
                
                dep->module_name = dependency_name;
                dep->target = strdup(dependency_name);
                dep->file_path = strdup(file_path);
                dep->layer = LAYER_MODULE;
                dep->next = dependencies;
                dependencies = dep;

                logr(VERBOSE, "[PHPAnalyzer] Found dependency: %s", dependency_name);
                pos += matches[0].rm_eo;
            }
        }
    }

    language_context_destroy(context);
    logr(DEBUG, "[PHPAnalyzer] Analysis complete");
    return dependencies;
}

// Ruby analyzer implementation
ExtractedDependency* analyze_ruby(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    logr(DEBUG, "[RubyAnalyzer] Analyzing file: %s", file_path);

    LanguageContext* context = language_context_create(LANG_RUBY, content, file_path, grammar);
    if (!context) return NULL;

    ExtractedDependency* dependencies = NULL;

    // Analyze requires
    const CompiledPatterns* patterns = compiledPatterns(LANG_RUBY, LAYER_MODULE);
    if (patterns) {
        const char* pos = content;
        
        for (size_t i = 0; i < patterns->pattern_count; i++) {
            regex_t* regex = &patterns->compiled_patterns[i];
            regmatch_t matches[2];

            pos = content;
            while (regexec(regex, pos, 2, matches, 0) == 0) {
                size_t name_len = matches[1].rm_eo - matches[1].rm_so;
                char* require_name = malloc(name_len + 1);
                strncpy(require_name, pos + matches[1].rm_so, name_len);
                require_name[name_len] = '\0';

                if (!is_ruby_builtin(require_name)) {
                    ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                    memset(dep, 0, sizeof(ExtractedDependency));
                    
                    dep->module_name = require_name;
                    dep->target = strdup(require_name);
                    dep->file_path = strdup(file_path);
                    dep->layer = LAYER_MODULE;
                    dep->next = dependencies;
                    dependencies = dep;

                    logr(VERBOSE, "[RubyAnalyzer] Found require: %s", require_name);
                } else {
                    free(require_name);
                }

                pos += matches[0].rm_eo;
            }
        }
    }

    language_context_destroy(context);
    logr(DEBUG, "[RubyAnalyzer] Analysis complete");
    return dependencies;
}

// Common extraction utilities implementation
ExtractedDependency* extract_imports(const LanguageContext* context, AnalysisLayer layer) {
    if (!context || layer != LAYER_MODULE) return NULL;

    const CompiledPatterns* patterns = compiledPatterns(context->type, layer);
    if (!patterns) return NULL;

    ExtractedDependency* dependencies = NULL;
    const char* pos = context->content;

    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        regmatch_t matches[2];

        pos = context->content;
        while (regexec(regex, pos, 2, matches, 0) == 0) {
            size_t name_len = matches[1].rm_eo - matches[1].rm_so;
            char* import_name = malloc(name_len + 1);
            strncpy(import_name, pos + matches[1].rm_so, name_len);
            import_name[name_len] = '\0';

            ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
            memset(dep, 0, sizeof(ExtractedDependency));
            
            dep->module_name = import_name;
            dep->target = strdup(import_name);
            dep->file_path = strdup(context->file_path);
            dep->layer = layer;
            dep->next = dependencies;
            dependencies = dep;

            pos += matches[0].rm_eo;
        }
    }

    return dependencies;
}

ExtractedDependency* extract_structures(const LanguageContext* context) {
    if (!context) return NULL;

    const CompiledPatterns* patterns = compiledPatterns(context->type, LAYER_STRUCT);
    if (!patterns) return NULL;

    ExtractedDependency* dependencies = NULL;
    const char* pos = context->content;

    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        regmatch_t matches[4];  // Allow for more captures

        pos = context->content;
        while (regexec(regex, pos, 4, matches, 0) == 0) {
            // Try to get structure name from different capture groups
            int name_group = 1;
            if (matches[2].rm_so != -1 && matches[2].rm_eo > matches[2].rm_so) {
                name_group = 2;
            }
            if (matches[3].rm_so != -1 && matches[3].rm_eo > matches[3].rm_so) {
                name_group = 3;
            }

            size_t name_len = matches[name_group].rm_eo - matches[name_group].rm_so;
            char* struct_name = malloc(name_len + 1);
            strncpy(struct_name, pos + matches[name_group].rm_so, name_len);
            struct_name[name_len] = '\0';

            ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
            memset(dep, 0, sizeof(ExtractedDependency));
            
            dep->module_name = struct_name;
            dep->target = strdup(struct_name);
            dep->file_path = strdup(context->file_path);
            dep->layer = LAYER_STRUCT;
            dep->next = dependencies;
            dependencies = dep;

            pos += matches[0].rm_eo;
        }
    }

    return dependencies;
}

ExtractedDependency* extract_methods(const LanguageContext* context) {
    if (!context) return NULL;

    const CompiledPatterns* patterns = compiledPatterns(context->type, LAYER_METHOD);
    if (!patterns) return NULL;

    ExtractedDependency* dependencies = NULL;
    const char* pos = context->content;

    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        regmatch_t matches[4];

        pos = context->content;
        while (regexec(regex, pos, 4, matches, 0) == 0) {
            // Extract method name (usually in group 1 or 2)
            int name_group = 1;
            if (matches[2].rm_so != -1 && matches[2].rm_eo > matches[2].rm_so) {
                name_group = 2;
            }

            size_t name_len = matches[name_group].rm_eo - matches[name_group].rm_so;
            char* method_name = malloc(name_len + 1);
            strncpy(method_name, pos + matches[name_group].rm_so, name_len);
            method_name[name_len] = '\0';

            // Skip if it's a language keyword
            if (!is_keyword(method_name, context->grammar->keywords, context->grammar->keyword_count)) {
                ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
                memset(dep, 0, sizeof(ExtractedDependency));
                
                dep->module_name = method_name;
                dep->target = strdup(method_name);
                dep->file_path = strdup(context->file_path);
                dep->layer = LAYER_METHOD;
                dep->next = dependencies;
                dependencies = dep;
            } else {
                free(method_name);
            }

            pos += matches[0].rm_eo;
        }
    }

    return dependencies;
}

// Utility functions
bool is_keyword(const char* word, const char** keywords, size_t keyword_count) {
    if (!word || !keywords) return false;

    for (size_t i = 0; i < keyword_count; i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool is_builtin_type(const char* word, const char** types, size_t type_count) {
    if (!word || !types) return false;

    for (size_t i = 0; i < type_count; i++) {
        if (strcmp(word, types[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Standard library detection functions
bool is_python_builtin(const char* name) {
    const char* builtins[] = {
        "os", "sys", "json", "re", "math", "datetime", "collections",
        "itertools", "functools", "operator", "typing", "pathlib",
        "urllib", "http", "socket", "threading", "multiprocessing",
        "asyncio", "sqlite3", "pickle", "csv", "xml", "html"
    };
    
    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (strcmp(name, builtins[i]) == 0) return true;
    }
    return false;
}

bool is_java_std_library(const char* name) {
    return (strncmp(name, "java.", 5) == 0 || 
            strncmp(name, "javax.", 6) == 0 ||
            strncmp(name, "org.w3c.", 8) == 0 ||
            strncmp(name, "org.xml.", 8) == 0);
}

bool is_go_std_library(const char* name) {
    const char* std_libs[] = {
        "fmt", "os", "io", "strings", "strconv", "time", "math",
        "net", "net/http", "encoding/json", "log", "bufio", "bytes",
        "context", "sync", "regexp", "sort", "errors", "flag"
    };
    
    for (size_t i = 0; i < sizeof(std_libs) / sizeof(std_libs[0]); i++) {
        if (strcmp(name, std_libs[i]) == 0) return true;
    }
    return false;
}

bool is_c_std_library(const char* name) {
    const char* std_headers[] = {
        "stdio.h", "stdlib.h", "string.h", "math.h", "time.h",
        "ctype.h", "assert.h", "errno.h", "limits.h", "float.h",
        "stdarg.h", "setjmp.h", "signal.h", "locale.h"
    };
    
    for (size_t i = 0; i < sizeof(std_headers) / sizeof(std_headers[0]); i++) {
        if (strcmp(name, std_headers[i]) == 0) return true;
    }
    return false;
}

bool is_ruby_builtin(const char* name) {
    const char* builtins[] = {
        "json", "yaml", "csv", "uri", "net/http", "openssl",
        "digest", "base64", "zlib", "fileutils", "pathname",
        "logger", "benchmark", "optparse", "ostruct"
    };
    
    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (strcmp(name, builtins[i]) == 0) return true;
    }
    return false;
}

// Placeholder implementations for other utility functions
bool is_rust_std_library(const char* name) {
    return (strncmp(name, "std::", 5) == 0 ||
            strncmp(name, "core::", 6) == 0 ||
            strncmp(name, "alloc::", 7) == 0);
}

bool is_js_builtin(const char* name) {
    const char* builtins[] = {
        "fs", "path", "os", "util", "events", "stream", "buffer",
        "crypto", "http", "https", "url", "querystring", "zlib"
    };
    
    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (strcmp(name, builtins[i]) == 0) return true;
    }
    return false;
}

bool is_php_builtin(const char* name) {
    // Most PHP functions are built-in and don't require includes
    return false;
}