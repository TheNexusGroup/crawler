#ifndef LANGUAGE_ANALYZERS_H
#define LANGUAGE_ANALYZERS_H

#include "syntaxes.h"


// Process matches functions
void process_module_matches(ExtractedDependency* dep, char** matches, size_t match_count);
void process_struct_matches(ExtractedDependency* dep, char** matches, size_t match_count);
void process_method_matches(ExtractedDependency* dep, char** matches, size_t match_count);

// Main analyzer function
ExtractedDependency* analyze_dependencies(const char* content, 
                                        LanguageType lang,
                                        AnalysisLayer layer);

#endif // LANGUAGE_ANALYZERS_H
