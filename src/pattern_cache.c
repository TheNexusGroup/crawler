#include "pattern_cache.h"
#include "grammars.h"
#include <stdlib.h>
#include <stdio.h>

// Global pattern cache instance
PatternCache pattern_cache = {0};

int initPatternCache(void) {
    //printf("Initializing pattern cache...\n");
    
    if (pattern_cache.initialized) {
        //printf("Pattern cache already initialized\n");
        return 0;
    }

    // Initialize for each language
    for (LanguageType lang = LANG_RUST; lang <= LANG_SVELTE; lang++) {
        //printf("Processing language %d...\n", lang);
        
        const LanguageGrammar* grammar = languageGrammars(lang);
        if (!grammar) {
            //printf("Failed to get grammar for language %d\n", lang);
            continue;
        }

        // Allocate and compile module patterns
        //printf("Compiling %zu module patterns...\n", grammar->module_pattern_count);
        pattern_cache.module_patterns[lang].compiled_patterns = 
            malloc(sizeof(regex_t) * grammar->module_pattern_count);
        if (!pattern_cache.module_patterns[lang].compiled_patterns) {
            //printf("Failed to allocate memory for module patterns\n");
            return -1;
        }
        pattern_cache.module_patterns[lang].pattern_count = grammar->module_pattern_count;

        for (size_t i = 0; i < grammar->module_pattern_count; i++) {
            //printf("Compiling module pattern: %s\n", grammar->module_patterns[i]);
            int result = regcomp(&pattern_cache.module_patterns[lang].compiled_patterns[i],
                                grammar->module_patterns[i], REG_EXTENDED);
            if (result != 0) {
                char error_buf[100];
                regerror(result, &pattern_cache.module_patterns[lang].compiled_patterns[i],
                        error_buf, sizeof(error_buf));
                //printf("Regex compilation failed: %s\n", error_buf);
                cleanPatternCache();
                return -1;
            }
        }

        // Allocate and compile struct patterns
        //printf("Compiling %zu struct patterns...\n", grammar->struct_pattern_count);
        pattern_cache.struct_patterns[lang].compiled_patterns = 
            malloc(sizeof(regex_t) * grammar->struct_pattern_count);
        if (!pattern_cache.struct_patterns[lang].compiled_patterns) {
            //printf("Failed to allocate memory for struct patterns\n");
            cleanPatternCache();
            return -1;
        }
        pattern_cache.struct_patterns[lang].pattern_count = grammar->struct_pattern_count;

        for (size_t i = 0; i < grammar->struct_pattern_count; i++) {
            //printf("Compiling struct pattern: %s\n", grammar->struct_patterns[i]);
            int result = regcomp(&pattern_cache.struct_patterns[lang].compiled_patterns[i],
                                grammar->struct_patterns[i], REG_EXTENDED);
            if (result != 0) {
                char error_buf[100];
                regerror(result, &pattern_cache.struct_patterns[lang].compiled_patterns[i],
                        error_buf, sizeof(error_buf));
                //printf("Regex compilation failed: %s\n", error_buf);
                cleanPatternCache();
                return -1;
            }
        }

        // Allocate and compile method patterns
        //printf("Compiling %zu method patterns...\n", grammar->method_pattern_count);
        pattern_cache.method_patterns[lang].compiled_patterns = 
            malloc(sizeof(regex_t) * grammar->method_pattern_count);
        if (!pattern_cache.method_patterns[lang].compiled_patterns) {
            //printf("Failed to allocate memory for method patterns\n");
            cleanPatternCache();
            return -1;
        }
        pattern_cache.method_patterns[lang].pattern_count = grammar->method_pattern_count;

        for (size_t i = 0; i < grammar->method_pattern_count; i++) {
            //printf("Compiling method pattern: %s\n", grammar->method_patterns[i]);
            int result = regcomp(&pattern_cache.method_patterns[lang].compiled_patterns[i],
                                grammar->method_patterns[i], REG_EXTENDED);
            if (result != 0) {
                char error_buf[100];
                regerror(result, &pattern_cache.method_patterns[lang].compiled_patterns[i],
                        error_buf, sizeof(error_buf));
                //printf("Regex compilation failed: %s\n", error_buf);
                cleanPatternCache();
                return -1;
            }
        }
        
        //printf("Successfully processed language %d\n", lang);
    }

    pattern_cache.initialized = 1;
    //printf("Pattern cache initialization complete\n");
    return 0;
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
        for (size_t i = 0; i < pattern_cache.method_patterns[lang].pattern_count; i++) {
            regfree(&pattern_cache.method_patterns[lang].compiled_patterns[i]);
        }
        free(pattern_cache.method_patterns[lang].compiled_patterns);
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
