#ifndef ANALYZERS_H
#define ANALYZERS_H

#include "syntaxes.h"
#include "grammars.h"

extern StructureDefinition* structure_definitions;
extern size_t structure_def_count;

// Add this at the top with other structs if not already present
typedef struct ScopeContext {
    char* class_name;    
    char* namespace_name;
    char* scope_type;
    int brace_depth;    
} ScopeContext;

ExtractedDependency* analyzeModule(const char* content, const LanguageGrammar* grammar);
ExtractedDependency* analyzeModuleWithFile(const char* content, const char* file_path, const LanguageGrammar* grammar);
ExtractedDependency* analyzeModuleGeneric(const char* content, const char* file_path, const LanguageGrammar* grammar);

#endif // ANALYZERS_H
