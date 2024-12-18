#ifndef SYNTAXES_H
#define SYNTAXES_H

#include <regex.h>
#include <stdbool.h>

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

// Language type functions
LanguageType languageType(const char* filename);
const char* languageName(LanguageType type);

// Layer definitions for granular analysis
typedef enum {
    LAYER_MODULE,    // First layer: modules, files, packages
    LAYER_STRUCT,    // Second layer: classes, structs, traits
    LAYER_METHOD     // Third layer: methods, functions, parameters
} AnalysisLayer;

// Parameter structure
typedef struct {
    char* name;
    char* type;
    char* default_value;
} Parameter;

// Forward declarations first
typedef struct Method Method;
typedef struct Structure Structure;
typedef struct ExtractedDependency ExtractedDependency;
typedef struct Dependency Dependency;

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

// Then the full struct definitions
typedef struct Method {
    char* name;
    char* return_type;
    Parameter* parameters;
    int param_count;
    char* dependencies;
    char* defined_in;
    struct MethodReference* references;
    int reference_count;
    struct Method* next;      // Sibling methods
    struct Method* children;  // Child methods (e.g., class methods)
    bool is_static;
    bool is_public;
    bool is_definition;
} Method;

typedef struct Structure {
    char* name;
    Method* methods;
    int method_count;
    char** implemented_traits;
    int trait_count;
    char* dependencies;
    int dependency_count;
    Structure* next;
} Structure;

typedef struct ExtractedDependency {
    char* file_path;
    char* target;
    char* module_name;
    Structure* structures;
    int structure_count;
    Method* methods;
    int method_count;
    LanguageType language;
    AnalysisLayer layer;
    struct ExtractedDependency* next;
    struct ExtractedDependency* modules;
} ExtractedDependency;

typedef struct Dependency {
    char* source;
    char* target;
    LanguageType language;
    AnalysisLayer level;
    Method* methods;
    int method_count;
    struct Dependency* next;
} Dependency;

// Language-specific syntax patterns for each layer
typedef struct {
    const char* patterns[MAX_MATCHES];
    int pattern_count;
    regex_t* compiled_patterns;
    AnalysisLayer layer;
} SyntaxPatterns;

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
void exportGraph(DependencyGraph* graph, const char* format, const char* output_path);
void freeStructures(Structure* structs);
extern void freeMethods(Method* methods);
void freeDependency(ExtractedDependency* dep);

// Add this before the MethodDefinition struct (around line 180)
typedef struct MethodReference {
    char* called_in;
    struct MethodReference* next;
} MethodReference;

// Then the existing MethodDefinition struct
typedef struct MethodDefinition {
    char* name;
    char* defined_in;
    char* dependencies;
    MethodReference* references;
    int reference_count;
} MethodDefinition;


#endif // SYNTAXES_H
