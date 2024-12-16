#ifndef LANGUAGE_ANALYZERS_H
#define LANGUAGE_ANALYZERS_H

#include "syntaxes.h"

// Language-specific analyzer functions
ExtractedDependency* analyze_rust_module(const char* content);
Structure* analyze_rust_structure(const char* content);
Method* analyze_rust_method(const char* content);

ExtractedDependency* analyze_c_module(const char* content);
Structure* analyze_c_structure(const char* content);
Method* analyze_c_method(const char* content);

// Add other language analyzers...

#endif // LANGUAGE_ANALYZERS_H
