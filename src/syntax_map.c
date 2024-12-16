#include "syntax_map.h"

// Optimized lookup function
static inline const LanguagePlugin* get_language_plugin(LanguageType type) {
    for (size_t i = 0; i < LANGUAGE_PLUGIN_COUNT; i++) {
        if (LANGUAGE_PLUGINS[i].type == type) {
            return &LANGUAGE_PLUGINS[i];
        }
    }
    return NULL;
}

// Initialize patterns for all languages
static inline int initialize_pattern_cache(void) {
    if (pattern_cache.initialized) return 0;

    for (size_t i = 0; i < LANGUAGE_PLUGIN_COUNT; i++) {
        const LanguagePlugin* plugin = &LANGUAGE_PLUGINS[i];
        
        // Compile module patterns
        pattern_cache.module_patterns[i].compiled_patterns = 
            malloc(sizeof(regex_t) * plugin->module_pattern_count);
        pattern_cache.module_patterns[i].pattern_count = plugin->module_pattern_count;
        
        for (size_t j = 0; j < plugin->module_pattern_count; j++) {
            if (regcomp(&pattern_cache.module_patterns[i].compiled_patterns[j],
                       plugin->module_patterns[j], REG_EXTENDED) != 0) {
                return -1;
            }
        }

        // Compile struct patterns
        pattern_cache.struct_patterns[i].compiled_patterns = 
            malloc(sizeof(regex_t) * plugin->struct_pattern_count);
        pattern_cache.struct_patterns[i].pattern_count = plugin->struct_pattern_count;
        
        for (size_t j = 0; j < plugin->struct_pattern_count; j++) {
            if (regcomp(&pattern_cache.struct_patterns[i].compiled_patterns[j],
                       plugin->struct_patterns[j], REG_EXTENDED) != 0) {
                return -1;
            }
        }

        // Compile method patterns
        pattern_cache.method_patterns[i].compiled_patterns = 
            malloc(sizeof(regex_t) * plugin->method_pattern_count);
        pattern_cache.method_patterns[i].pattern_count = plugin->method_pattern_count;
        
        for (size_t j = 0; j < plugin->method_pattern_count; j++) {
            if (regcomp(&pattern_cache.method_patterns[i].compiled_patterns[j],
                       plugin->method_patterns[j], REG_EXTENDED) != 0) {
                return -1;
            }
        }
    }

    pattern_cache.initialized = 1;
    return 0;
}

// Clean up pattern cache
static inline void cleanup_pattern_cache(void) {
    if (!pattern_cache.initialized) return;

    for (size_t i = 0; i < LANGUAGE_PLUGIN_COUNT; i++) {
        for (size_t j = 0; j < pattern_cache.module_patterns[i].pattern_count; j++) {
            regfree(&pattern_cache.module_patterns[i].compiled_patterns[j]);
        }
        free(pattern_cache.module_patterns[i].compiled_patterns);

        for (size_t j = 0; j < pattern_cache.struct_patterns[i].pattern_count; j++) {
            regfree(&pattern_cache.struct_patterns[i].compiled_patterns[j]);
        }
        free(pattern_cache.struct_patterns[i].compiled_patterns);

        for (size_t j = 0; j < pattern_cache.method_patterns[i].pattern_count; j++) {
            regfree(&pattern_cache.method_patterns[i].compiled_patterns[j]);
        }
        free(pattern_cache.method_patterns[i].compiled_patterns);
    }

    pattern_cache.initialized = 0;
}

// Get compiled patterns for a specific language and layer
static inline const CompiledPatterns* get_compiled_patterns(LanguageType type, AnalysisLayer layer) {
    const LanguagePlugin* plugin = get_language_plugin(type);
    if (!plugin || !pattern_cache.initialized) return NULL;

    size_t index = plugin - LANGUAGE_PLUGINS;
    switch (layer) {
        case LAYER_MODULE:
            return &pattern_cache.module_patterns[index];
        case LAYER_STRUCT:
            return &pattern_cache.struct_patterns[index];
        case LAYER_METHOD:
            return &pattern_cache.method_patterns[index];
        default:
            return NULL;
    }
}
