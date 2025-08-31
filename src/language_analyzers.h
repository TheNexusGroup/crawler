#ifndef LANGUAGE_ANALYZERS_H
#define LANGUAGE_ANALYZERS_H

#include "syntaxes.h"
#include "dependency.h"
#include "grammars.h"

// Language-specific analyzer function types
typedef ExtractedDependency* (*LanguageAnalyzerFunc)(const char* content, const char* file_path, const LanguageGrammar* grammar);

// Structure for language-specific analysis context
typedef struct LanguageContext {
    LanguageType type;
    const char* file_path;
    const char* content;
    size_t content_length;
    const LanguageGrammar* grammar;
    
    // Analysis state
    int current_line;
    int current_column;
    int scope_depth;
    char* current_namespace;
    char* current_class;
    
    // Result accumulation
    ExtractedDependency* dependencies;
    size_t dependency_count;
} LanguageContext;

// Core analyzer functions for each language
ExtractedDependency* analyze_rust(const char* content, const char* file_path, const LanguageGrammar* grammar);
ExtractedDependency* analyze_c_cpp(const char* content, const char* file_path, const LanguageGrammar* grammar);
ExtractedDependency* analyze_javascript(const char* content, const char* file_path, const LanguageGrammar* grammar);
ExtractedDependency* analyze_typescript(const char* content, const char* file_path, const LanguageGrammar* grammar);
ExtractedDependency* analyze_python(const char* content, const char* file_path, const LanguageGrammar* grammar);
ExtractedDependency* analyze_java(const char* content, const char* file_path, const LanguageGrammar* grammar);
ExtractedDependency* analyze_go(const char* content, const char* file_path, const LanguageGrammar* grammar);
ExtractedDependency* analyze_php(const char* content, const char* file_path, const LanguageGrammar* grammar);
ExtractedDependency* analyze_ruby(const char* content, const char* file_path, const LanguageGrammar* grammar);

// Helper functions for language analysis
LanguageContext* language_context_create(LanguageType type, const char* content, const char* file_path, const LanguageGrammar* grammar);
void language_context_destroy(LanguageContext* context);

// Common analysis utilities
ExtractedDependency* extract_imports(const LanguageContext* context, AnalysisLayer layer);
ExtractedDependency* extract_structures(const LanguageContext* context);
ExtractedDependency* extract_methods(const LanguageContext* context);

// String processing utilities
bool is_keyword(const char* word, const char** keywords, size_t keyword_count);
bool is_builtin_type(const char* word, const char** types, size_t type_count);
char* extract_namespace_from_import(const char* import_statement, LanguageType type);
char* normalize_dependency_name(const char* name, LanguageType type);

// Pattern matching utilities
bool match_import_pattern(const char* line, LanguageType type, char** import_name);
bool match_class_pattern(const char* line, LanguageType type, char** class_name);
bool match_method_pattern(const char* line, LanguageType type, char** method_name, char** return_type, char** parameters);

// Language-specific parsing helpers
ExtractedDependency* parse_rust_use_statements(const char* content, const char* file_path);
ExtractedDependency* parse_c_includes(const char* content, const char* file_path);
ExtractedDependency* parse_js_imports(const char* content, const char* file_path);
ExtractedDependency* parse_python_imports(const char* content, const char* file_path);
ExtractedDependency* parse_java_imports(const char* content, const char* file_path);
ExtractedDependency* parse_go_imports(const char* content, const char* file_path);
ExtractedDependency* parse_php_includes(const char* content, const char* file_path);
ExtractedDependency* parse_ruby_requires(const char* content, const char* file_path);

// Advanced analysis features
typedef struct {
    char* name;
    char* type;
    int line_number;
    char* visibility; // public, private, protected, etc.
} StructureMember;

typedef struct {
    char* name;
    char* return_type;
    char** parameter_types;
    size_t parameter_count;
    char* visibility;
    bool is_static;
    bool is_async;
    int line_number;
} MethodInfo;

typedef struct {
    char* name;
    char* type; // class, interface, struct, enum, trait, etc.
    char* extends; // inheritance
    char** implements; // interfaces implemented
    size_t implements_count;
    StructureMember* members;
    size_t member_count;
    MethodInfo* methods;
    size_t method_count;
    int line_number;
} StructureInfo;

// Advanced structure analysis
StructureInfo* analyze_structure_info(const char* content, LanguageType type);
void structure_info_free(StructureInfo* info);

// Dependency resolution
char* resolve_import_path(const char* import_statement, const char* current_file_path, LanguageType type);
bool is_external_dependency(const char* dependency_name, LanguageType type);
bool is_standard_library(const char* dependency_name, LanguageType type);

// Language-specific standard library detection
bool is_rust_std_library(const char* name);
bool is_c_std_library(const char* name);
bool is_js_builtin(const char* name);
bool is_python_builtin(const char* name);
bool is_java_std_library(const char* name);
bool is_go_std_library(const char* name);
bool is_php_builtin(const char* name);
bool is_ruby_builtin(const char* name);

#endif // LANGUAGE_ANALYZERS_H