#include "pattern_cache.h"
#include "grammars.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "logger.h"

// Global pattern cache instance
PatternCache pattern_cache = {0};

int initPatternCache(void) {
    logr(DEBUG, "[PatternCache] Initializing pattern cache");
    
    // Initialize all patterns to zero
    memset(&pattern_cache, 0, sizeof(PatternCache));
    pattern_cache.initialized = 0;  // Explicitly mark as not initialized
    
    // For each language type
    for (LanguageType lang = 0; lang < LANGUAGE_GRAMMAR_COUNT; lang++) {
        const LanguageGrammar* grammar = languageGrammars(lang);
        if (!grammar) {
            logr(ERROR, "[PatternCache] Failed to get grammar for language %d", lang);
            return 0;
        }
        
        // Compile module patterns
        pattern_cache.module_patterns[lang].compiled_patterns = 
            malloc(grammar->module_pattern_count * sizeof(regex_t));
        if (!pattern_cache.module_patterns[lang].compiled_patterns) {
            logr(ERROR, "[PatternCache] Failed to allocate memory for module patterns");
            return 0;
        }
        
        pattern_cache.module_patterns[lang].pattern_count = grammar->module_pattern_count;
        for (size_t i = 0; i < grammar->module_pattern_count; i++) {
            if (regcomp(&pattern_cache.module_patterns[lang].compiled_patterns[i],
                       grammar->module_patterns[i], REG_EXTENDED) != 0) {
                logr(ERROR, "[PatternCache] Failed to compile module pattern %zu for language %d", i, lang);
                return 0;
            }
        }
        
        // Compile struct patterns
        pattern_cache.struct_patterns[lang].compiled_patterns = 
            malloc(grammar->struct_pattern_count * sizeof(regex_t));
        if (!pattern_cache.struct_patterns[lang].compiled_patterns) {
            logr(ERROR, "[PatternCache] Failed to allocate memory for struct patterns");
            return 0;
        }
        
        pattern_cache.struct_patterns[lang].pattern_count = grammar->struct_pattern_count;
        for (size_t i = 0; i < grammar->struct_pattern_count; i++) {
            logr(DEBUG, "[PatternCache] Compiling struct pattern %zu for language %d: %s", 
                 i, lang, grammar->struct_patterns[i]);
            if (regcomp(&pattern_cache.struct_patterns[lang].compiled_patterns[i],
                       grammar->struct_patterns[i], REG_EXTENDED) != 0) {
                logr(ERROR, "[PatternCache] Failed to compile struct pattern %zu for language %d", i, lang);
                return 0;
            }
        }
        
        // Compile method patterns
        if (grammar->method_patterns && grammar->method_pattern_count > 0) {
            pattern_cache.method_patterns[lang].compiled_patterns = 
                malloc(grammar->method_pattern_count * sizeof(regex_t));
            if (!pattern_cache.method_patterns[lang].compiled_patterns) {
                logr(ERROR, "[PatternCache] Failed to allocate memory for method patterns");
                return 0;
            }
            
            pattern_cache.method_patterns[lang].pattern_count = grammar->method_pattern_count;
            for (size_t i = 0; i < grammar->method_pattern_count; i++) {
                logr(DEBUG, "[PatternCache] Compiling method pattern %zu for language %d: %s", 
                     i, lang, grammar->method_patterns[i]);
                if (regcomp(&pattern_cache.method_patterns[lang].compiled_patterns[i],
                           grammar->method_patterns[i], REG_EXTENDED) != 0) {
                    logr(ERROR, "[PatternCache] Failed to compile method pattern %zu for language %d", i, lang);
                    return 0;
                }
            }
        }
    }
    
    pattern_cache.initialized = 1;  // Mark as initialized if successful
    logr(DEBUG, "[PatternCache] Pattern cache initialized successfully");
    return pattern_cache.initialized;
}

void cleanPatternCache(void) {
    //printf("Cleaning pattern cache...\n");
    
    if (!pattern_cache.initialized) {
        //printf("Pattern cache was not initialized\n");
        return;
    }

    for (LanguageType lang = LANG_RUST; lang <= LANG_SVELTE; lang++) {
        //printf("Cleaning patterns for language %d...\n", lang);
        
        // Free module patterns
        for (size_t i = 0; i < pattern_cache.module_patterns[lang].pattern_count; i++) {
            regfree(&pattern_cache.module_patterns[lang].compiled_patterns[i]);
        }
        free(pattern_cache.module_patterns[lang].compiled_patterns);

        // Free struct patterns
        for (size_t i = 0; i < pattern_cache.struct_patterns[lang].pattern_count; i++) {
            regfree(&pattern_cache.struct_patterns[lang].compiled_patterns[i]);
        }
        free(pattern_cache.struct_patterns[lang].compiled_patterns);

        // Free method patterns
        if (pattern_cache.method_patterns[lang].pattern_count > 0) {
            for (size_t i = 0; i < pattern_cache.method_patterns[lang].pattern_count; i++) {
                regfree(&pattern_cache.method_patterns[lang].compiled_patterns[i]);
            }
            free(pattern_cache.method_patterns[lang].compiled_patterns);
        }
    }

    pattern_cache.initialized = 0;
    //printf("Pattern cache cleanup complete\n");
}

const CompiledPatterns* compiledPatterns(LanguageType lang, AnalysisLayer layer) {
    if (!pattern_cache.initialized || lang < LANG_RUST || lang > LANG_SVELTE) {
        return NULL;
    }

    switch (layer) {
        case LAYER_MODULE:
            return &pattern_cache.module_patterns[lang];
        case LAYER_STRUCT:
            return &pattern_cache.struct_patterns[lang];
        case LAYER_METHOD:
            return &pattern_cache.method_patterns[lang];
        default:
            return NULL;
    }
}
