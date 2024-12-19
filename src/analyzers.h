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


extern MethodDefinition* method_definitions;
extern size_t method_def_count;

// Core analysis functions
ExtractedDependency* analyzeModule(const char* content, const LanguageGrammar* grammar);
Structure* analyze_structure(const char* content, const char* file_path, const LanguageGrammar* grammar);
Method* analyzeMethod(const char* file_path, const char* content, const LanguageGrammar* grammar);

// Structure tracking functions
void collectStructures(const char* file_path, const char* content, const LanguageGrammar* grammar);
StructureDefinition* get_structure_definitions(size_t* count);

// Method tracking functions
void collectDefinitions(const char* file_path, const char* content, const LanguageGrammar* grammar);

// Dependency conversion helpers
Dependency* create_dependency_from_extracted(ExtractedDependency* extracted);
void graphDependency(Dependency* graph, ExtractedDependency* extracted);
void freeExtractedDep(ExtractedDependency* dep);

// Add these if they're not already present
void freeMethod_references(MethodReference* refs);
void freeMethod_definitions(void);

#endif // ANALYZERS_H
