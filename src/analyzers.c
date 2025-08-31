#include "analyzers.h"
#include "pattern_cache.h"
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "logger.h"
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#include "language_analyzers.h"

// Enhanced module analyzer that uses language-specific analyzers
ExtractedDependency* analyzeModule(const char* content, const LanguageGrammar* grammar) {
    return analyzeModuleWithFile(content, "unknown", grammar);
}

ExtractedDependency* analyzeModuleWithFile(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    if (!content || !grammar || !file_path) {
        logr(ERROR, "[Analyzer] Invalid parameters for module analysis");
        return NULL;
    }

    logr(DEBUG, "[Analyzer] Analyzing %s with language-specific analyzer", languageName(grammar->type));

    // Use language-specific analyzers for better accuracy
    switch (grammar->type) {
        case LANG_RUST:
            return analyze_rust(content, file_path, grammar);
        case LANG_C:
            return analyze_c_cpp(content, file_path, grammar);
        case LANG_JAVASCRIPT:
            return analyze_javascript(content, file_path, grammar);
        case LANG_PYTHON:
            return analyze_python(content, file_path, grammar);
        case LANG_JAVA:
            return analyze_java(content, file_path, grammar);
        case LANG_GO:
            return analyze_go(content, file_path, grammar);
        case LANG_PHP:
            return analyze_php(content, file_path, grammar);
        case LANG_RUBY:
            return analyze_ruby(content, file_path, grammar);
        default:
            logr(WARN, "[Analyzer] No specific analyzer for language %d, using generic", grammar->type);
            return analyzeModuleGeneric(content, file_path, grammar);
    }
}

// Generic fallback analyzer for unsupported languages
ExtractedDependency* analyzeModuleGeneric(const char* content, const char* file_path, const LanguageGrammar* grammar) {
    ExtractedDependency* head = NULL;
    ExtractedDependency* current = NULL;

    // Get compiled patterns
    const CompiledPatterns* patterns = compiledPatterns(grammar->type, LAYER_MODULE);
    if (!patterns) {
        logr(ERROR, "[Analyzer] Failed to get compiled patterns for module analysis");
        return NULL;
    }

    // Process each pattern using the pre-compiled patterns
    for (size_t i = 0; i < patterns->pattern_count; i++) {
        regex_t* regex = &patterns->compiled_patterns[i];
        regmatch_t matches[2];  // [0] full match, [1] module name
        const char* pos = content;

        while (regexec(regex, pos, 2, matches, 0) == 0) {
            // Extract the matched module name
            size_t len = matches[1].rm_eo - matches[1].rm_so;
            char* module_name = malloc(len + 1);
            strncpy(module_name, pos + matches[1].rm_so, len);
            module_name[len] = '\0';

            logr(DEBUG, "[Analyzer] Found module dependency: %s", module_name);

            // Create new dependency node
            ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
            memset(dep, 0, sizeof(ExtractedDependency));
            dep->module_name = module_name;
            dep->target = strdup(module_name);
            dep->file_path = strdup(file_path);
            dep->layer = LAYER_MODULE;
            dep->next = NULL;

            // Add to list
            if (!head) {
                head = dep;
                current = dep;
            } else {
                current->next = dep;
                current = dep;
            }

            pos += matches[0].rm_eo;
        }
    }

    return head;
}

