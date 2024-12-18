#ifndef ANALYZERS_H
#define ANALYZERS_H

#include "syntaxes.h"
#include "grammars.h"

// Core analysis functions
ExtractedDependency* analyze_module(const char* content, const LanguageGrammar* grammar);
Structure* analyze_structure(const char* content, const LanguageGrammar* grammar);
Method* analyze_method(const char* content, const LanguageGrammar* grammar);

// Dependency conversion helpers
Dependency* create_dependency_from_extracted(ExtractedDependency* extracted);
void add_to_dependency_graph(Dependency* graph, ExtractedDependency* extracted);
void free_extracted_dependency(ExtractedDependency* dep);

#endif // ANALYZERS_H
