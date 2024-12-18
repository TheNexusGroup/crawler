#ifndef GRAMMARS_H
#define GRAMMARS_H

#include "syntaxes.h"


typedef struct {
    LanguageType type;
    const char** module_patterns;
    size_t module_pattern_count;
    const char** struct_patterns;
    size_t struct_pattern_count;
    const char** method_patterns;
    size_t method_pattern_count;
    int method_name_group;
    const char* scope_separator;
} LanguageGrammar;

extern const LanguageGrammar LANGUAGE_GRAMMARS[];
extern const size_t LANGUAGE_GRAMMAR_COUNT;
const LanguageGrammar* languageGrammars(LanguageType type);

#endif // GRAMMARS_H