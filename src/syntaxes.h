#ifndef SYNTAXES_H
#define SYNTAXES_H

#include <regex.h>

// Common patterns across languages
#define MAX_PATTERN_LENGTH 256
#define MAX_MATCHES 10

// Enum to represent different supported languages
typedef enum {
    LANG_RUST,
    LANG_C,
    LANG_CPP,
    LANG_JAVASCRIPT,
    LANG_GO,
    LANG_PYTHON
} LanguageType;

// Update DependencyLevel to match AnalysisLayer
typedef enum {
    DEP_MODULE = LAYER_MODULE,
    DEP_STRUCT = LAYER_STRUCT,
    DEP_METHOD = LAYER_METHOD
} DependencyLevel;

// Flexible dependency struct to capture relationships
typedef struct Dependency {
    char* source;
    char* target;
    DependencyLevel level;
    LanguageType language;
    struct Dependency* next;
} Dependency;

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

// First Layer: Module-level patterns (existing patterns)
static const char* RUST_MODULE_PATTERNS[] = {
    "^\\s*use\\s+([a-zA-Z0-9_:]+)",
    "^\\s*extern\\s+crate\\s+([a-zA-Z0-9_]+)",
    "^\\s*mod\\s+([a-zA-Z0-9_]+)",
    "^\\s*include!\\s*\\(\\s*\"([^\"]+)\"\\s*\\)",
};

// Second Layer: Struct/Class-level patterns
static const char* RUST_STRUCT_PATTERNS[] = {
    "^\\s*(?:pub\\s+)?struct\\s+([a-zA-Z0-9_]+)",     // Struct definitions
    "^\\s*(?:pub\\s+)?trait\\s+([a-zA-Z0-9_]+)",      // Trait definitions
    "^\\s*(?:pub\\s+)?enum\\s+([a-zA-Z0-9_]+)",       // Enum definitions
    "^\\s*impl(?:\\s+[a-zA-Z0-9_]+)?\\s+for\\s+([a-zA-Z0-9_]+)", // Implementations
};

// Third Layer: Method/Parameter patterns
static const char* RUST_METHOD_PATTERNS[] = {
    "^\\s*(?:pub\\s+)?fn\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)",  // Function definitions with params
    "^\\s*self\\.([a-zA-Z0-9_]+)\\s*\\(.*\\)",                   // Method calls
    "^\\s*([a-zA-Z0-9_]+)::([a-zA-Z0-9_]+)\\s*\\(.*\\)",        // Static method calls
};

// C/C++ patterns for each layer
static const char* C_MODULE_PATTERNS[] = {
    "^\\s*#include\\s*[<\"]([^>\"]+)[>\"]",
    "^\\s*#import\\s*[<\"]([^>\"]+)[>\"]",
    "^\\s*#pragma\\s+once",
};

static const char* C_STRUCT_PATTERNS[] = {
    "^\\s*(?:typedef\\s+)?struct\\s+([a-zA-Z0-9_]+)",  // Struct definitions
    "^\\s*(?:typedef\\s+)?enum\\s+([a-zA-Z0-9_]+)",    // Enum definitions
    "^\\s*(?:typedef\\s+)?union\\s+([a-zA-Z0-9_]+)",   // Union definitions
    "^\\s*class\\s+([a-zA-Z0-9_]+)",                   // C++ class definitions
};

static const char* C_METHOD_PATTERNS[] = {
    "^\\s*([a-zA-Z0-9_]+)\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)", // Function definitions
    "^\\s*([a-zA-Z0-9_]+)::[a-zA-Z0-9_]+\\s*\\(.*\\)",          // C++ method calls
};

// Extended structures for deeper analysis
typedef struct {
    char* name;
    char* type;
    char* default_value;
} Parameter;

typedef struct {
    char* name;
    Parameter* parameters;
    int param_count;
    char* return_type;
    int is_public;
} Method;

typedef struct {
    char* name;
    Method* methods;
    int method_count;
    char** implemented_traits;
    int trait_count;
    char** dependencies;
    int dependency_count;
} Structure;

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
