#ifndef METHOD_H
#define METHOD_H

#include "dependency.h"

// Forward declarations first
typedef struct Method Method;

// Parameter structure
typedef struct {
    char* name;
    char* type;
    char* default_value;
} Parameter;

// Then the full struct definitions
typedef struct Method {
    char* name;
    char* prefix;
    char* return_type;
    Parameter* parameters;
    int param_count;
    char* dependencies;
    char* defined_in;
    struct MethodReference* references;
    int reference_count;
    struct Method* next;      // Sibling methods
    struct Method* children;  // Child methods (e.g., class methods)
    bool is_static;
    bool is_public;
    bool is_definition;
    const char* body_start;
    const char* body_end;
} Method;

typedef struct MethodReference {
    char* called_in;
    struct MethodReference* next;
} MethodReference;

typedef struct MethodDependency {
    char* name;
    struct MethodDependency* next;
} MethodDependency;

typedef struct MethodDefinition {
    char* name;
    char* return_type;
    char* defined_in;
    MethodDependency* dependencies;  // Changed to linked list
    size_t param_count;
    MethodReference* references;
    size_t reference_count;
} MethodDefinition;

void addMethod(const char* method_name, const char* file_path, const char* return_type);
Method* extractMatchingMethod(const char* content, regmatch_t* matches, const LanguageGrammar* grammar);
void graphMethods(DependencyCrawler* crawler, const char* file_path, 
                                Method* methods);
void scanMethodBodyForCalls(const char* body_start, MethodDefinition* method, 
                                 const LanguageGrammar* grammar);
const char* findMethodBody(const char* start);
void printMethods(Method* method);//, const char* source_file);
extern void freeMethods(Method* methods);
Method* analyzeMethod(const char* file_path, const char* content, const LanguageGrammar* grammar);
void freeMethodReferences(MethodReference* refs);
void freeMethod_definitions(void);
int countMethods(Method* methods);
char* formatMethodSignature(Method* method);
MethodDefinition* findMethodDefinition(const char* method_name);
void collectDefinitions(const char* file_path, const char* content, const LanguageGrammar* grammar);

#endif // METHOD_H