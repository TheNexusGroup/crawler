#ifndef SYNTAXES_H
#define SYNTAXES_H

#include <regex.h>

// Common constants
#define MAX_PATTERN_LENGTH 256
#define MAX_MATCHES 10
#define MAX_LANGUAGES 10
#define MAX_TRAITS 32
#define MAX_PARAMETERS 16
#define MAX_DEPENDENCIES 64

// Structure size limits
#define MAX_METHODS_PER_STRUCT 32
#define MAX_PARAMS_PER_METHOD 16
#define MAX_TRAITS_PER_STRUCT 8

// Enum to represent different supported languages
typedef enum {
    LANG_RUST,
    LANG_C,
    LANG_JAVASCRIPT,
    LANG_GO,
    LANG_PYTHON,
    LANG_JAVA,
    LANG_PHP,
    LANG_RUBY,
    LANG_SVELTE,
} LanguageType;

// Layer definitions for granular analysis
typedef enum {
    LAYER_MODULE,    // First layer: modules, files, packages
    LAYER_STRUCT,    // Second layer: classes, structs, traits
    LAYER_METHOD     // Third layer: methods, functions, parameters
} AnalysisLayer;

// Update DependencyLevel to match AnalysisLayer
typedef enum {
    DEP_MODULE = LAYER_MODULE,
    DEP_STRUCT = LAYER_STRUCT,
    DEP_METHOD = LAYER_METHOD
} DependencyLevel;

// Parameter structure
typedef struct {
    char* name;
    char* type;
    char* default_value;
} Parameter;

// Method structure
typedef struct {
    char* name;
    char* return_type;
    Parameter parameters[MAX_PARAMETERS];
    int param_count;
} Method;

// Structure definition
typedef struct {
    char* name;
    Method* methods;
    int method_count;
    char** implemented_traits;
    int trait_count;
    char** dependencies;
    int dependency_count;
} Structure;

// Flexible dependency struct to capture relationships
typedef struct Dependency {
    char* source;
    char* target;
    DependencyLevel level;
    LanguageType language;
    struct Dependency* next;
} Dependency;



// Language-specific syntax patterns for each layer
typedef struct {
    const char* patterns[MAX_MATCHES];
    int pattern_count;
    regex_t* compiled_patterns;
    AnalysisLayer layer;
} SyntaxPatterns;

// Enhanced dependency features
typedef struct {
    // First layer features (existing)
    int is_pub_mod;
    int is_conditional;
    int is_system_header;
    int is_local_header;
    
    // Second layer features
    int is_public_struct;
    int has_generic_params;
    int implements_trait;
    int is_abstract;
    
    // Third layer features
    int is_public_method;
    int is_static;
    int is_virtual;
    int has_default_impl;
} DependencyFeatures;

// Enhanced dependency extraction
typedef struct {
    // Basic info
    char* module_name;
    char* file_path;
    
    // Structural info
    Structure* structures;
    int structure_count;
    
    // Method info
    Method* methods;
    int method_count;
    
    // Features
    DependencyFeatures features;
    
    // Analysis metadata
    AnalysisLayer layer;
    int depth;
} ExtractedDependency;

// Configuration for analysis depth
typedef struct {
    int analyze_modules;      // First layer
    int analyze_structures;   // Second layer
    int analyze_methods;      // Third layer
    int max_depth;           // Maximum recursion depth
    int follow_external;     // Whether to analyze external dependencies
} AnalysisConfig;

// Function prototypes for multi-layer analysis
ExtractedDependency* analyze_file(const char* file_path, AnalysisConfig* config);
Structure* analyze_structure(const char* file_content, const char* struct_name);
Method* analyze_method(const char* method_content);

// Language-specific analyzers
typedef struct {
    ExtractedDependency* (*analyze_module)(const char* content);
    Structure* (*analyze_structure)(const char* content);
    Method* (*analyze_method)(const char* content);
} LanguageAnalyzer;

// Helper functions for relationship mapping
typedef struct {
    char* from;
    char* to;
    char* relationship_type;  // "inherits", "implements", "uses", etc.
    AnalysisLayer layer;
} Relationship;

// Graph generation helpers
typedef struct {
    Relationship** relationships;
    int relationship_count;
    AnalysisLayer current_layer;
} DependencyGraph;

// Function prototypes for graph generation
DependencyGraph* create_dependency_graph(ExtractedDependency** deps, int dep_count);
void export_graph(DependencyGraph* graph, const char* format, const char* output_path);

#endif // SYNTAXES_H
