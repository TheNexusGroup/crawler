#ifndef ANALYZERS_H
#define ANALYZERS_H

#include "syntaxes.h"
#include "grammars.h"

// Structure definition tracking
typedef struct {
    char* name;
    char* type;
    char* defined_in;
    int reference_count;
    char** referenced_in;
    size_t max_references;
} StructureDefinition;

extern StructureDefinition* structure_definitions;
extern size_t structure_def_count;

// Add this at the top with other structs if not already present
typedef struct ScopeContext {
    char* class_name;    
    char* namespace_name;
    char* scope_type;
    int brace_depth;    
} ScopeContext;

// Add this structure to track method references
typedef struct MethodReference {
    char* called_in;
    struct MethodReference* next;
} MethodReference;

// Method definition tracking
typedef struct MethodDefinition {
    char* name;
    char* defined_in;
    MethodReference* references;  // Change from referenced_in array
    int reference_count;
    int max_references;  // Keep this if still needed
} MethodDefinition;

extern MethodDefinition* method_definitions;
extern size_t method_def_count;

// Core analysis functions
ExtractedDependency* analyze_module(const char* content, const LanguageGrammar* grammar);
Structure* analyze_structure(const char* content, const char* file_path, const LanguageGrammar* grammar);
Method* analyze_method(const char* file_path, const char* content, const LanguageGrammar* grammar);

// Structure tracking functions
void collect_structure_definitions(const char* file_path, const char* content, const LanguageGrammar* grammar);
StructureDefinition* get_structure_definitions(size_t* count);

// Method tracking functions
void collect_method_definitions(const char* file_path, const char* content, const LanguageGrammar* grammar);
MethodDefinition* get_method_definitions(size_t* count);

// Dependency conversion helpers
Dependency* create_dependency_from_extracted(ExtractedDependency* extracted);
void add_to_dependency_graph(Dependency* graph, ExtractedDependency* extracted);
void free_extracted_dependency(ExtractedDependency* dep);

// Add these if they're not already present
void free_method_references(MethodReference* refs);
void free_method_definitions(void);

#endif // ANALYZERS_H
