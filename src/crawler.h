#ifndef CRAWLER_H
#define CRAWLER_H

#include "syntaxes.h"
#include "syntax_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Enhanced Language Parser interface
typedef struct {
    LanguageType type;
    ExtractedDependency* (*analyze_module)(const char* content);
    Structure* (*analyze_structure)(const char* content);
    Method* (*analyze_method)(const char* content);
} LanguageParser;

// Enhanced Crawler configuration
typedef struct {
    char** root_directories;
    int directory_count;
    LanguageParser* parsers;
    int parser_count;
    Dependency* dependency_graph;
    AnalysisConfig analysis_config;
    DependencyGraph* result_graph;
} DependencyCrawler;

// Function prototypes
DependencyCrawler* create_crawler(char** dirs, int dir_count, AnalysisConfig* config);
void register_language_parser(DependencyCrawler* crawler, LanguageType type, 
                            const LanguageParser* parser);
void crawl_dependencies(DependencyCrawler* crawler);
void print_dependencies(DependencyCrawler* crawler);
void export_dependencies(DependencyCrawler* crawler, const char* output_format);
void free_crawler(DependencyCrawler* crawler);

#endif // CRAWLER_H