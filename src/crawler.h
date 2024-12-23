#ifndef CRAWLER_H
#define CRAWLER_H

#include "syntaxes.h"
#include "syntax_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function prototypes
ExtractedDependency* analyzeFile(const char* file_path, AnalysisConfig* config);
DependencyCrawler* createCrawler(char** dirs, int dir_count, AnalysisConfig* config);
void registerParser(DependencyCrawler* crawler, LanguageType type, 
                            const LanguageParser* parser);
void freeCrawler(DependencyCrawler* crawler);
void exportGraph(DependencyGraph* graph, const char* format, const char* output_path);

#endif // CRAWLER_H