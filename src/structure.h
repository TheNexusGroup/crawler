#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "method.h"

typedef struct Structure Structure;

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

// Structure definition tracking
typedef struct {
    char* name;
    char* type;
    char* defined_in;
    int reference_count;
    char** referenced_in;
    size_t max_references;
} StructureDefinition;


// Structure tracking functions
void collectStructures(const char* file_path, const char* content, const LanguageGrammar* grammar);
StructureDefinition* get_structure_definitions(size_t* count);
Structure* analyze_structure(const char* content, const char* file_path, const LanguageGrammar* grammar);
void freeStructures(Structure* structs);


#endif // STRUCTURE_H