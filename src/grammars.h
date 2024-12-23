#ifndef GRAMMARS_H
#define GRAMMARS_H

#include "syntaxes.h"
#include "dependency.h"
#include "structure.h"
#include "method.h"

// Enhanced Language Parser interface
typedef struct {
    LanguageType type;
    ExtractedDependency* (*analyzeModule)(const char* content);
    Structure* (*analyze_structure)(const char* content);
    Method* (*analyzeMethod)(const char* content);
} LanguageParser;

typedef struct {
    LanguageType type;
    const char** module_patterns;
    size_t module_pattern_count;
    const char** struct_patterns;
    size_t struct_pattern_count;
    const char** method_patterns;
    size_t method_pattern_count;
    int method_name_group;
    int return_type_group;
    int params_group;
    const char* scope_separator;
    const char** storage_classes;
    size_t storage_class_count;
    const char** keywords;
    size_t keyword_count;
    const char** types;
    size_t type_count;
    const char** prefixes;
    size_t prefix_count;
} LanguageGrammar;

extern const LanguageGrammar LANGUAGE_GRAMMARS[];
extern const size_t LANGUAGE_GRAMMAR_COUNT;
const LanguageGrammar* languageGrammars(LanguageType type);

#endif // GRAMMARS_H