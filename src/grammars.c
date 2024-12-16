#include "grammars.h"
#include "patterns.h"

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
        .method_pattern_count = sizeof(RUST_METHOD_PATTERNS) / sizeof(char*)
    },
    // C
    {
        .type = LANG_C,
        .module_patterns = C_MODULE_PATTERNS,
        .module_pattern_count = sizeof(C_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = C_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(C_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = C_METHOD_PATTERNS,
        .method_pattern_count = sizeof(C_METHOD_PATTERNS) / sizeof(char*)
    },
    // JavaScript
    {
        .type = LANG_JAVASCRIPT,
        .module_patterns = JS_MODULE_PATTERNS,
        .module_pattern_count = sizeof(JS_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = JS_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(JS_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = JS_METHOD_PATTERNS,
        .method_pattern_count = sizeof(JS_METHOD_PATTERNS) / sizeof(char*)
    },
    // Go
    {
        .type = LANG_GO,
        .module_patterns = GO_MODULE_PATTERNS,
        .module_pattern_count = sizeof(GO_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = GO_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(GO_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = GO_METHOD_PATTERNS,
        .method_pattern_count = sizeof(GO_METHOD_PATTERNS) / sizeof(char*)
    },
    // Python
    {
        .type = LANG_PYTHON,
        .module_patterns = PYTHON_MODULE_PATTERNS,
        .module_pattern_count = sizeof(PYTHON_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = PYTHON_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(PYTHON_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = PYTHON_METHOD_PATTERNS,
        .method_pattern_count = sizeof(PYTHON_METHOD_PATTERNS) / sizeof(char*)
    },
    // Java
    {
        .type = LANG_JAVA,
        .module_patterns = JAVA_MODULE_PATTERNS,
        .module_pattern_count = sizeof(JAVA_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = JAVA_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(JAVA_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = JAVA_METHOD_PATTERNS,
        .method_pattern_count = sizeof(JAVA_METHOD_PATTERNS) / sizeof(char*)
    },
    // PHP
    {
        .type = LANG_PHP,
        .module_patterns = PHP_MODULE_PATTERNS,
        .module_pattern_count = sizeof(PHP_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = PHP_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(PHP_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = PHP_METHOD_PATTERNS,
        .method_pattern_count = sizeof(PHP_METHOD_PATTERNS) / sizeof(char*)
    },
    // Ruby
    {
        .type = LANG_RUBY,
        .module_patterns = RUBY_MODULE_PATTERNS,
        .module_pattern_count = sizeof(RUBY_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = RUBY_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(RUBY_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = RUBY_METHOD_PATTERNS,
        .method_pattern_count = sizeof(RUBY_METHOD_PATTERNS) / sizeof(char*)
    },
    // Svelte
    {
        .type = LANG_SVELTE,
        .module_patterns = SVELTE_MODULE_PATTERNS,
        .module_pattern_count = sizeof(SVELTE_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = SVELTE_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(SVELTE_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = SVELTE_METHOD_PATTERNS,
        .method_pattern_count = sizeof(SVELTE_METHOD_PATTERNS) / sizeof(char*)
    }
};

// Define the count
const size_t LANGUAGE_GRAMMAR_COUNT = sizeof(LANGUAGE_GRAMMARS) / sizeof(LanguageGrammar);

// Implementation of languageGrammars function
const LanguageGrammar* languageGrammars(LanguageType type) {
    if (type < 0 || type >= LANGUAGE_GRAMMAR_COUNT) {
        return NULL;
    }
    return &LANGUAGE_GRAMMARS[type];
}
