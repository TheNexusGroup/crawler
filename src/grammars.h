#ifndef GRAMMARS_H
#define GRAMMARS_H

#include "syntaxes.h"
#include "syntax_map.h"
#include "pattern_cache.h"
#include <regex.h>
#include <stdlib.h>


typedef struct {
    LanguageType type;
    const char** module_patterns;
    size_t module_pattern_count;
    const char** struct_patterns;
    size_t struct_pattern_count;
    const char** method_patterns;
    size_t method_pattern_count;
} LanguageGrammar;

extern const char** MODULE_PATTERNS[];
extern const char** STRUCT_PATTERNS[];
extern const char** METHOD_PATTERNS[];

const LanguageGrammar* languageGrammars(LanguageType type);
extern const LanguageGrammar LANGUAGE_GRAMMARS[];

#endif // GRAMMARS_H