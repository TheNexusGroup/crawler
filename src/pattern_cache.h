#ifndef PATTERN_CACHE_H
#define PATTERN_CACHE_H

#include "syntax_map.h"

// Compiled patterns structure
typedef struct {
    regex_t* compiled_patterns;
    size_t pattern_count;
} CompiledPatterns;

// Pattern cache structure
typedef struct {
    CompiledPatterns module_patterns[MAX_LANGUAGES];
    CompiledPatterns struct_patterns[MAX_LANGUAGES];
    CompiledPatterns method_patterns[MAX_LANGUAGES];
    int initialized;
} PatternCache;

extern PatternCache pattern_cache;


// Function declarations
int initPatternCache(void);
void cleanPatternCache(void);
const CompiledPatterns* compiledPatterns(LanguageType type, AnalysisLayer layer);

#endif // PATTERN_CACHE_H