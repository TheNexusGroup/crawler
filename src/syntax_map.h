#ifndef SYNTAX_MAP_H
#define SYNTAX_MAP_H

#include "syntaxes.h"
#include "grammars.h"

// Language grammars array declaration
extern const size_t LANGUAGE_GRAMMAR_COUNT;

// Helper function to free dependency
void free_dependency(ExtractedDependency* dep);

#endif // SYNTAX_MAP_H
