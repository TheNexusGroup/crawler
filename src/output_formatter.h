#ifndef OUTPUT_FORMATTER_H
#define OUTPUT_FORMATTER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "dependency.h"
#include "syntaxes.h"

// Output format types
typedef enum {
    OUTPUT_FORMAT_TERMINAL,
    OUTPUT_FORMAT_JSON,
    OUTPUT_FORMAT_GRAPHVIZ,
    OUTPUT_FORMAT_HTML,
    OUTPUT_FORMAT_XML,
    OUTPUT_FORMAT_CSV,
    OUTPUT_FORMAT_MERMAID,
    OUTPUT_FORMAT_PLANTURL
} OutputFormat;

// Layer filtering options
typedef enum {
    FILTER_NONE = 0,
    FILTER_MODULE = 1 << 0,
    FILTER_STRUCT = 1 << 1,
    FILTER_METHOD = 1 << 2,
    FILTER_ALL = FILTER_MODULE | FILTER_STRUCT | FILTER_METHOD
} LayerFilter;

// Connection type filtering
typedef enum {
    CONNECTION_IMPORT = 1 << 0,
    CONNECTION_INHERITANCE = 1 << 1,
    CONNECTION_COMPOSITION = 1 << 2,
    CONNECTION_DEPENDENCY = 1 << 3,
    CONNECTION_CALL = 1 << 4,
    CONNECTION_ALL = 0xFF
} ConnectionFilter;

// Output styling options
typedef struct OutputStyle {
    bool use_colors;
    bool show_line_numbers;
    bool show_file_paths;
    bool show_statistics;
    bool compact_mode;
    bool show_timestamps;
    int max_depth;
    int max_width;
} OutputStyle;

// Filtering configuration
typedef struct FilterConfig {
    LayerFilter layer_filter;
    ConnectionFilter connection_filter;
    char** include_patterns;
    size_t include_pattern_count;
    char** exclude_patterns;
    size_t exclude_pattern_count;
    char** file_extensions;
    size_t extension_count;
    LanguageType* languages;
    size_t language_count;
} FilterConfig;

// Graph layout options
typedef struct LayoutOptions {
    char* layout_engine;  // dot, neato, fdp, sfdp, twopi, circo
    char* node_shape;
    char* edge_style;
    bool cluster_by_directory;
    bool cluster_by_language;
    bool show_external_deps;
    int max_nodes;
    int max_edges;
} LayoutOptions;

// Output formatter context
typedef struct OutputFormatter {
    OutputFormat format;
    OutputStyle style;
    FilterConfig filters;
    LayoutOptions layout;
    FILE* output_stream;
    char* output_file_path;
    
    // Formatter state
    bool header_written;
    bool footer_needed;
    size_t nodes_written;
    size_t edges_written;
    
    // Statistics
    size_t total_dependencies;
    size_t filtered_dependencies;
    size_t output_size_bytes;
    
    bool is_initialized;
} OutputFormatter;

// Dependency node for graph representation
typedef struct DependencyNode {
    char* id;
    char* label;
    char* file_path;
    LanguageType language;
    AnalysisLayer layer;
    char* node_type;  // module, class, function, etc.
    
    // Visual attributes
    char* color;
    char* shape;
    char* style;
    
    struct DependencyNode* next;
} DependencyNode;

// Dependency edge for graph representation
typedef struct DependencyEdge {
    char* source_id;
    char* target_id;
    char* label;
    char* edge_type;  // import, extends, implements, calls, etc.
    AnalysisLayer layer;
    
    // Visual attributes
    char* color;
    char* style;
    char* arrow_type;
    
    struct DependencyEdge* next;
} DependencyEdge;

// Output formatter functions
OutputFormatter* output_formatter_create(OutputFormat format, const char* output_file);
void output_formatter_destroy(OutputFormatter* formatter);

// Configuration
int output_formatter_set_style(OutputFormatter* formatter, const OutputStyle* style);
int output_formatter_set_filters(OutputFormatter* formatter, const FilterConfig* filters);
int output_formatter_set_layout(OutputFormatter* formatter, const LayoutOptions* layout);

// Main output functions
int output_formatter_write_dependencies(
    OutputFormatter* formatter,
    const ExtractedDependency* dependencies
);
int output_formatter_write_graph(
    OutputFormatter* formatter,
    const DependencyNode* nodes,
    const DependencyEdge* edges
);
int output_formatter_write_statistics(
    OutputFormatter* formatter,
    const ExtractedDependency* dependencies
);

// Format-specific writers
int output_write_terminal(OutputFormatter* formatter, const ExtractedDependency* deps);
int output_write_json(OutputFormatter* formatter, const ExtractedDependency* deps);
int output_write_graphviz(OutputFormatter* formatter, const DependencyNode* nodes, const DependencyEdge* edges);
int output_write_html(OutputFormatter* formatter, const ExtractedDependency* deps);
int output_write_mermaid(OutputFormatter* formatter, const DependencyNode* nodes, const DependencyEdge* edges);

// Graph construction
DependencyNode* output_build_nodes(const ExtractedDependency* dependencies, const FilterConfig* filters);
DependencyEdge* output_build_edges(const ExtractedDependency* dependencies, const FilterConfig* filters);
void dependency_node_free(DependencyNode* nodes);
void dependency_edge_free(DependencyEdge* edges);

// Filtering functions
bool output_filter_dependency(const ExtractedDependency* dep, const FilterConfig* filters);
bool output_filter_by_layer(const ExtractedDependency* dep, LayerFilter filter);
bool output_filter_by_pattern(const char* name, char** patterns, size_t count);
bool output_filter_by_language(LanguageType language, const FilterConfig* filters);

// Utility functions
const char* output_format_to_string(OutputFormat format);
const char* output_layer_to_string(AnalysisLayer layer);
const char* output_connection_type_to_string(const ExtractedDependency* dep);
char* output_escape_string(const char* str, OutputFormat format);

// Statistics calculation
typedef struct OutputStats {
    size_t total_files;
    size_t total_dependencies;
    size_t dependencies_by_layer[3];  // MODULE, STRUCT, METHOD
    size_t dependencies_by_language[MAX_LANGUAGES];
    size_t circular_dependencies;
    size_t external_dependencies;
} OutputStats;

OutputStats output_calculate_stats(const ExtractedDependency* dependencies);
void output_print_stats(const OutputStats* stats, OutputFormat format, FILE* stream);

#endif // OUTPUT_FORMATTER_H