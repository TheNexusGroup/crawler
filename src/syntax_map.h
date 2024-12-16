#ifndef SYNTAX_MAP_H
#define SYNTAX_MAP_H

#include "syntaxes.h"
#include <stddef.h>

// Language grammar structure
typedef struct {
    LanguageType type;
    const char** module_patterns;
    size_t module_pattern_count;
    const char** struct_patterns;
    size_t struct_pattern_count;
    const char** method_patterns;
    size_t method_pattern_count;
} LanguageGrammar;

// Grammar definitions for each language
static const LanguageGrammar LANGUAGE_GRAMMARS[] = {
    {
        .type = LANG_RUST,
        .module_patterns = RUST_MODULE_PATTERNS,
        .module_pattern_count = sizeof(RUST_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = RUST_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(RUST_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = RUST_METHOD_PATTERNS,
        .method_pattern_count = sizeof(RUST_METHOD_PATTERNS) / sizeof(char*)
    },
    {
        .type = LANG_C,
        .module_patterns = C_MODULE_PATTERNS,
        .module_pattern_count = sizeof(C_MODULE_PATTERNS) / sizeof(char*),
        .struct_patterns = C_STRUCT_PATTERNS,
        .struct_pattern_count = sizeof(C_STRUCT_PATTERNS) / sizeof(char*),
        .method_patterns = C_METHOD_PATTERNS,
        .method_pattern_count = sizeof(C_METHOD_PATTERNS) / sizeof(char*)
    },
    // Add other languages...
};

// Unified analyzer functions
ExtractedDependency* analyze_dependencies(const char* content, 
                                        LanguageType lang,
                                        AnalysisLayer layer);

// Helper functions
static inline const LanguageGrammar* get_language_grammar(LanguageType type);
static inline const CompiledPatterns* get_compiled_patterns(LanguageType type, 
                                                          AnalysisLayer layer);

#endif // SYNTAX_MAP_H
