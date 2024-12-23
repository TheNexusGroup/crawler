#include "syntaxes.h"
#include "pattern_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "analyzers.h"
#include <ctype.h>
#include "logger.h"

LanguageType languageType(const char* filename) {
    if (!filename) return LANG_RUST; // Default to first language
    
    const char* ext = strrchr(filename, '.');
    if (!ext) {
        logr(DEBUG, "[Syntaxes] Skipping file without extension: %s", filename);
        return LANG_RUST; // Default to first language
    }
    
    ext++; // Skip the dot
    
    // Convert extension to lowercase for comparison
    char ext_lower[32] = {0};
    size_t i;
    for (i = 0; i < sizeof(ext_lower) - 1 && ext[i]; i++) {
        ext_lower[i] = tolower(ext[i]);
    }
    
    // Match extensions to language types
    if (strcmp(ext_lower, "rs") == 0) {
        return LANG_RUST;
    } else if (strcmp(ext_lower, "c") == 0 || 
               strcmp(ext_lower, "h") == 0 || 
               strcmp(ext_lower, "cpp") == 0 || 
               strcmp(ext_lower, "hpp") == 0) {
        return LANG_C;
    } else if (strcmp(ext_lower, "js") == 0 || 
               strcmp(ext_lower, "jsx") == 0 || 
               strcmp(ext_lower, "ts") == 0 || 
               strcmp(ext_lower, "tsx") == 0) {
        return LANG_JAVASCRIPT;
    } else if (strcmp(ext_lower, "go") == 0) {
        return LANG_GO;
    } else if (strcmp(ext_lower, "py") == 0) {
        return LANG_PYTHON;
    } else if (strcmp(ext_lower, "java") == 0) {
        return LANG_JAVA;
    } else if (strcmp(ext_lower, "php") == 0) {
        return LANG_PHP;
    } else if (strcmp(ext_lower, "rb") == 0) {
        return LANG_RUBY;
    }
    
    logr(DEBUG, "[Syntaxes] Unsupported file extension: %s", ext_lower);
    return LANG_RUST; // Default to first language instead of -1
}

const char* languageName(LanguageType type) {
    switch (type) {
        case LANG_RUST: return "Rust";
        case LANG_C: return "C/C++";
        case LANG_JAVASCRIPT: return "JavaScript";
        case LANG_GO: return "Go";
        case LANG_PYTHON: return "Python";
        case LANG_JAVA: return "Java";
        case LANG_PHP: return "PHP";
        case LANG_RUBY: return "Ruby";
        default: return "Unknown";
    }
}
