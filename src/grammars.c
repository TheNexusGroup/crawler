#include "grammars.h"
#include "patterns.h"
#include "logger.h"
#include <stddef.h>

// Define the actual array
const LanguageGrammar LANGUAGE_GRAMMARS[] = {
    // Rust
    {
        .type = LANG_RUST,
        .module_patterns = RUST_MODULE_PATTERNS,
        .module_pattern_count = sizeof(RUST_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = RUST_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(RUST_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = RUST_METHOD_PATTERNS,
        .method_pattern_count = sizeof(RUST_METHOD_PATTERNS) / sizeof(char*),
        .method_name_group = 1,
        .return_type_group = 3,
        .params_group = 2,
        .scope_separator = "::",
        .storage_classes = (const char*[]){"pub", "async"},
        .storage_class_count = 2,
        .keywords = RUST_KEYWORDS,
        .keyword_count = sizeof(RUST_KEYWORDS) / sizeof(char*),
    },
    // C/C++
    {
        .type = LANG_C,
        .module_patterns = C_MODULE_PATTERNS,
        .module_pattern_count = sizeof(C_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = C_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(C_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = C_METHOD_PATTERNS,
        .method_pattern_count = sizeof(C_METHOD_PATTERNS) / sizeof(char*),
        .method_name_group = 2,
        .return_type_group = 1,
        .params_group = 3,
        .scope_separator = "->.::",
        .storage_classes = (const char*[]){"static", "extern", "inline"},
        .storage_class_count = 3,
        .keywords = C_KEYWORDS,
        .keyword_count = sizeof(C_KEYWORDS) / sizeof(char*),
    },
    // JavaScript
    {
        .type = LANG_JAVASCRIPT,
        .module_patterns = JS_MODULE_PATTERNS,
        .module_pattern_count = sizeof(JS_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = JS_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(JS_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = JS_METHOD_PATTERNS,
        .method_pattern_count = sizeof(JS_METHOD_PATTERNS) / sizeof(char*),
        .method_name_group = 2,
        .return_type_group = -1,
        .params_group = 3,
        .scope_separator = ".",
        .storage_classes = (const char*[]){"async"},
        .storage_class_count = 1,
        .keywords = JS_KEYWORDS,
        .keyword_count = sizeof(JS_KEYWORDS) / sizeof(char*),
    },
    // Go
    {
        .type = LANG_GO,
        .module_patterns = GO_MODULE_PATTERNS,
        .module_pattern_count = sizeof(GO_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = GO_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(GO_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = GO_METHOD_PATTERNS,
        .method_pattern_count = sizeof(GO_METHOD_PATTERNS) / sizeof(char*),
        .method_name_group = 2,
        .return_type_group = 1,
        .params_group = 3,
        .scope_separator = ".",
        .storage_classes = NULL,
        .storage_class_count = 0,
        .keywords = GO_KEYWORDS,
        .keyword_count = sizeof(GO_KEYWORDS) / sizeof(char*),
    },
    // Python
    {
        .type = LANG_PYTHON,
        .module_patterns = PYTHON_MODULE_PATTERNS,
        .module_pattern_count = sizeof(PYTHON_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = PYTHON_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(PYTHON_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = PYTHON_METHOD_PATTERNS,
        .method_pattern_count = sizeof(PYTHON_METHOD_PATTERNS) / sizeof(char*),
        .method_name_group = 2,
        .return_type_group = -1,
        .params_group = 3,
        .scope_separator = ".",
        .storage_classes = (const char*[]){"async"},
        .storage_class_count = 1,
        .keywords = PYTHON_KEYWORDS,
        .keyword_count = sizeof(PYTHON_KEYWORDS) / sizeof(char*),
    },
    // Java
    {
        .type = LANG_JAVA,
        .module_patterns = JAVA_MODULE_PATTERNS,
        .module_pattern_count = sizeof(JAVA_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = JAVA_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(JAVA_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = JAVA_METHOD_PATTERNS,
        .method_pattern_count = sizeof(JAVA_METHOD_PATTERNS) / sizeof(char*),
        .method_name_group = 2,
        .return_type_group = 1,
        .params_group = 3,
        .scope_separator = ".",
        .storage_classes = (const char*[]){"static", "final", "abstract", "synchronized"},
        .storage_class_count = 4,
        .keywords = JAVA_KEYWORDS,
        .keyword_count = sizeof(JAVA_KEYWORDS) / sizeof(char*),
    },
    // PHP
    {
        .type = LANG_PHP,
        .module_patterns = PHP_MODULE_PATTERNS,
        .module_pattern_count = sizeof(PHP_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = PHP_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(PHP_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = PHP_METHOD_PATTERNS,
        .method_pattern_count = sizeof(PHP_METHOD_PATTERNS) / sizeof(char*),
        .method_name_group = 2,
        .return_type_group = 1,
        .params_group = 3,
        .scope_separator = "->",
        .storage_classes = (const char*[]){"static", "final", "abstract", "private", "public", "protected"},
        .storage_class_count = 6,
        .keywords = PHP_KEYWORDS,
        .keyword_count = sizeof(PHP_KEYWORDS) / sizeof(char*),
    },
    // Ruby
    {
        .type = LANG_RUBY,
        .module_patterns = RUBY_MODULE_PATTERNS,
        .module_pattern_count = sizeof(RUBY_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = RUBY_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(RUBY_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = RUBY_METHOD_PATTERNS,
        .method_pattern_count = sizeof(RUBY_METHOD_PATTERNS) / sizeof(char*),
        .method_name_group = 2,
        .return_type_group = -1,
        .params_group = 3,
        .scope_separator = "::",
        .storage_classes = (const char*[]){"private", "public", "protected"},
        .storage_class_count = 3,
        .keywords = RUBY_KEYWORDS,
        .keyword_count = sizeof(RUBY_KEYWORDS) / sizeof(char*),
    },
    // Svelte
    {
        .type = LANG_SVELTE,
        .module_patterns = SVELTE_MODULE_PATTERNS,
        .module_pattern_count = sizeof(SVELTE_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = SVELTE_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(SVELTE_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = SVELTE_METHOD_PATTERNS,
        .method_pattern_count = sizeof(SVELTE_METHOD_PATTERNS) / sizeof(char*),
        .method_name_group = 2,
        .return_type_group = -1,
        .params_group = 3,
        .scope_separator = ".",
        .storage_classes = (const char*[]){"export", "async"},
        .storage_class_count = 2,
        .keywords = SVELTE_KEYWORDS,
        .keyword_count = sizeof(SVELTE_KEYWORDS) / sizeof(char*),
    }
};

// Define the count
const size_t LANGUAGE_GRAMMAR_COUNT = sizeof(LANGUAGE_GRAMMARS) / sizeof(LanguageGrammar);

// Implementation of languageGrammars function
const LanguageGrammar* languageGrammars(LanguageType type) {
    const char* lang_name = languageName(type);
    logr(VERBOSE, "[Grammars] Looking up grammar for language: %s", lang_name);
    
    if (type < 0) {
        logr(WARN, "[Grammars] Invalid language: %s", lang_name);
        return NULL;
    }
    
    if (type >= LANGUAGE_GRAMMAR_COUNT) {
        logr(ERROR, "[Grammars] Language: %s exceeds grammar count %zu", 
             lang_name, LANGUAGE_GRAMMAR_COUNT);
        return NULL;
    }
    
    const LanguageGrammar* grammar = &LANGUAGE_GRAMMARS[type];
    if (!grammar->module_patterns || grammar->module_pattern_count == 0) {
        logr(ERROR, "[Grammars] Invalid grammar configuration for language: %s", lang_name);
        return NULL;
    }
    
    logr(VERBOSE, "[Grammars] Successfully found grammar for language: %s", lang_name);
    return grammar;
}
