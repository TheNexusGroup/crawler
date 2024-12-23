#ifndef SYNTAXES_H
#define SYNTAXES_H

#include <regex.h>
#include <stdbool.h>

// Common constants
#define MAX_PATTERN_LENGTH 256
#define MAX_MATCHES 10
#define MAX_LANGUAGES 8
#define MAX_TRAITS 32
#define MAX_PARAMETERS 16
#define MAX_DEPENDENCIES 64

// Structure size limits
#define MAX_METHODS_PER_STRUCT 32
#define MAX_PARAMS_PER_METHOD 16
#define MAX_TRAITS_PER_STRUCT 8

// Enum to represent different supported languages
typedef enum {
    LANG_RUST = 0,        // Index 0
    LANG_C = 1,          // Index 1
    LANG_JAVASCRIPT = 2, // Index 2
    LANG_GO = 3,         // Index 3
    LANG_PYTHON = 4,     // Index 4
    LANG_JAVA = 5,       // Index 5
    LANG_PHP = 6,        // Index 6
    LANG_RUBY = 7        // Index 7
} LanguageType;

// Layer definitions for granular analysis
typedef enum {
    LAYER_MODULE,    // First layer: modules, files, packages
    LAYER_STRUCT,    // Second layer: classes, structs, traits
    LAYER_METHOD     // Third layer: methods, functions, parameters
} AnalysisLayer;

// Language-specific syntax patterns for each layer
typedef struct {
    const char* patterns[MAX_MATCHES];
    int pattern_count;
    regex_t* compiled_patterns;
    AnalysisLayer layer;
} SyntaxPatterns;

// Configuration for analysis depth
typedef struct {
    int analyzeModules;      // First layer
    int analyze_structures;   // Second layer
    int analyzeMethods;      // Third layer
    int max_depth;           // Maximum recursion depth
    int follow_external;     // Whether to analyze external dependencies
} AnalysisConfig;

// Language-specific analyzers
typedef struct {
    ExtractedDependency* (*analyzeModule)(const char* content);
    Structure* (*analyze_structure)(const char* content);
    Method* (*analyzeMethod)(const char* content);
} LanguageAnalyzer;

// Language type functions
LanguageType languageType(const char* filename);
const char* languageName(LanguageType type);


#endif // SYNTAXES_H
