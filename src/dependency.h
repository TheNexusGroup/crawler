#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "syntaxes.h"
#include "structure.h"
#include "method.h"
#include "grammars.h"

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

// Enhanced Crawler configuration
typedef struct {
    char** root_directories;
    int directory_count;
    LanguageParser* parsers;
    int parser_count;
    Dependency* dependency_graph;
    AnalysisConfig analysis_config;
    DependencyGraph* result_graph;
} DependencyCrawler;


// Function prototypes for multi-layer analysis
void crawlDeps(DependencyCrawler* crawler);
void printDependencies(DependencyCrawler* crawler);
void exportDeps(DependencyCrawler* crawler, const char* output_format);
void graphDependency(Dependency* graph, ExtractedDependency* extracted);
void freeDependency(ExtractedDependency* dep);
void freeExtractedDep(ExtractedDependency* dep);
DependencyGraph* createDependencyGraph(ExtractedDependency** deps, int dep_count);
Dependency* create_dependency_from_extracted(ExtractedDependency* extracted);

#endif // DEPENDENCY_H
