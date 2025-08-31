#include "output_formatter.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Color definitions for terminal output
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

OutputFormatter* output_formatter_create(OutputFormat format, const char* output_file) {
    OutputFormatter* formatter = malloc(sizeof(OutputFormatter));
    if (!formatter) {
        logr(ERROR, "[OutputFormatter] Failed to allocate formatter");
        return NULL;
    }

    memset(formatter, 0, sizeof(OutputFormatter));
    formatter->format = format;

    // Set default style
    formatter->style.use_colors = true;
    formatter->style.show_line_numbers = false;
    formatter->style.show_file_paths = true;
    formatter->style.show_statistics = false;
    formatter->style.compact_mode = false;
    formatter->style.show_timestamps = false;
    formatter->style.max_depth = 10;
    formatter->style.max_width = 100;

    // Set default filters
    formatter->filters.layer_filter = FILTER_ALL;
    formatter->filters.connection_filter = CONNECTION_ALL;
    formatter->filters.include_patterns = NULL;
    formatter->filters.include_pattern_count = 0;
    formatter->filters.exclude_patterns = NULL;
    formatter->filters.exclude_pattern_count = 0;

    // Open output stream
    if (output_file) {
        formatter->output_file_path = strdup(output_file);
        formatter->output_stream = fopen(output_file, "w");
        if (!formatter->output_stream) {
            logr(ERROR, "[OutputFormatter] Failed to open output file: %s", output_file);
            free(formatter);
            return NULL;
        }
    } else {
        formatter->output_stream = stdout;
    }

    formatter->is_initialized = true;
    logr(DEBUG, "[OutputFormatter] Created formatter for format %d", format);
    return formatter;
}

void output_formatter_destroy(OutputFormatter* formatter) {
    if (!formatter) return;

    if (formatter->output_stream && formatter->output_stream != stdout) {
        fclose(formatter->output_stream);
    }

    free(formatter->output_file_path);
    
    // Free filter arrays
    for (size_t i = 0; i < formatter->filters.include_pattern_count; i++) {
        free(formatter->filters.include_patterns[i]);
    }
    free(formatter->filters.include_patterns);

    for (size_t i = 0; i < formatter->filters.exclude_pattern_count; i++) {
        free(formatter->filters.exclude_patterns[i]);
    }
    free(formatter->filters.exclude_patterns);

    free(formatter->filters.file_extensions);
    free(formatter->filters.languages);
    free(formatter->layout.layout_engine);
    free(formatter->layout.node_shape);
    free(formatter->layout.edge_style);

    free(formatter);
    logr(DEBUG, "[OutputFormatter] Formatter destroyed");
}

int output_formatter_set_filters(OutputFormatter* formatter, const FilterConfig* filters) {
    if (!formatter || !filters) return -1;

    formatter->filters = *filters;
    
    // Deep copy string arrays
    if (filters->include_patterns && filters->include_pattern_count > 0) {
        formatter->filters.include_patterns = malloc(filters->include_pattern_count * sizeof(char*));
        for (size_t i = 0; i < filters->include_pattern_count; i++) {
            formatter->filters.include_patterns[i] = strdup(filters->include_patterns[i]);
        }
    }

    if (filters->exclude_patterns && filters->exclude_pattern_count > 0) {
        formatter->filters.exclude_patterns = malloc(filters->exclude_pattern_count * sizeof(char*));
        for (size_t i = 0; i < filters->exclude_pattern_count; i++) {
            formatter->filters.exclude_patterns[i] = strdup(filters->exclude_patterns[i]);
        }
    }

    logr(DEBUG, "[OutputFormatter] Filters configured");
    return 0;
}

bool output_filter_dependency(const ExtractedDependency* dep, const FilterConfig* filters) {
    if (!dep || !filters) return true;

    // Layer filtering
    if (filters->layer_filter != FILTER_ALL) {
        bool layer_matches = false;
        if ((filters->layer_filter & FILTER_MODULE) && dep->layer == LAYER_MODULE) layer_matches = true;
        if ((filters->layer_filter & FILTER_STRUCT) && dep->layer == LAYER_STRUCT) layer_matches = true;
        if ((filters->layer_filter & FILTER_METHOD) && dep->layer == LAYER_METHOD) layer_matches = true;
        
        if (!layer_matches) return false;
    }

    // Pattern filtering
    if (filters->exclude_pattern_count > 0) {
        for (size_t i = 0; i < filters->exclude_pattern_count; i++) {
            if (dep->module_name && strstr(dep->module_name, filters->exclude_patterns[i])) {
                return false;
            }
        }
    }

    if (filters->include_pattern_count > 0) {
        bool matches_include = false;
        for (size_t i = 0; i < filters->include_pattern_count; i++) {
            if (dep->module_name && strstr(dep->module_name, filters->include_patterns[i])) {
                matches_include = true;
                break;
            }
        }
        if (!matches_include) return false;
    }

    return true;
}

const char* output_layer_to_string(AnalysisLayer layer) {
    switch (layer) {
        case LAYER_MODULE: return "MODULE";
        case LAYER_STRUCT: return "STRUCT";
        case LAYER_METHOD: return "METHOD";
        default: return "UNKNOWN";
    }
}

const char* output_connection_type_to_string(const ExtractedDependency* dep) {
    if (!dep) return "UNKNOWN";
    
    switch (dep->layer) {
        case LAYER_MODULE: return "IMPORT";
        case LAYER_STRUCT: return "EXTENDS";
        case LAYER_METHOD: return "CALLS";
        default: return "DEPENDS";
    }
}

char* output_escape_string(const char* str, OutputFormat format) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char* escaped = malloc(len * 2 + 1); // Worst case: every character needs escaping
    size_t j = 0;
    
    for (size_t i = 0; i < len; i++) {
        switch (format) {
            case OUTPUT_FORMAT_JSON:
                if (str[i] == '"' || str[i] == '\\') {
                    escaped[j++] = '\\';
                }
                escaped[j++] = str[i];
                break;
                
            case OUTPUT_FORMAT_GRAPHVIZ:
                if (str[i] == '"') {
                    escaped[j++] = '\\';
                }
                escaped[j++] = str[i];
                break;
                
            default:
                escaped[j++] = str[i];
                break;
        }
    }
    
    escaped[j] = '\0';
    return escaped;
}

int output_write_terminal(OutputFormatter* formatter, const ExtractedDependency* deps) {
    if (!formatter || !formatter->output_stream) return -1;

    FILE* out = formatter->output_stream;
    const ExtractedDependency* current = deps;
    int depth = 0;

    // Write header
    if (formatter->style.use_colors) {
        fprintf(out, "%s%sDependency Analysis Results%s\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET);
        fprintf(out, "%s================================%s\n\n", COLOR_BLUE, COLOR_RESET);
    } else {
        fprintf(out, "Dependency Analysis Results\n");
        fprintf(out, "================================\n\n");
    }

    while (current) {
        // Apply filtering
        if (!output_filter_dependency(current, &formatter->filters)) {
            current = current->next;
            continue;
        }

        // Print dependency information
        const char* layer_str = output_layer_to_string(current->layer);
        const char* connection_str = output_connection_type_to_string(current);

        if (formatter->style.use_colors) {
            const char* layer_color = COLOR_GREEN;
            if (current->layer == LAYER_STRUCT) layer_color = COLOR_YELLOW;
            else if (current->layer == LAYER_METHOD) layer_color = COLOR_CYAN;

            fprintf(out, "%s[%s]%s ", layer_color, layer_str, COLOR_RESET);
        } else {
            fprintf(out, "[%s] ", layer_str);
        }

        if (current->module_name) {
            if (formatter->style.use_colors) {
                fprintf(out, "%s%s%s", COLOR_BOLD, current->module_name, COLOR_RESET);
            } else {
                fprintf(out, "%s", current->module_name);
            }
        }

        if (current->target && strcmp(current->target, current->module_name) != 0) {
            if (formatter->style.use_colors) {
                fprintf(out, " %s->%s %s", COLOR_MAGENTA, COLOR_RESET, current->target);
            } else {
                fprintf(out, " -> %s", current->target);
            }
        }

        if (formatter->style.show_file_paths && current->file_path) {
            if (formatter->style.use_colors) {
                fprintf(out, " %s(%s)%s", COLOR_CYAN, current->file_path, COLOR_RESET);
            } else {
                fprintf(out, " (%s)", current->file_path);
            }
        }

        fprintf(out, "\n");

        // Print structures if available
        if (current->structures && current->structure_count > 0) {
            for (int i = 0; i < current->structure_count; i++) {
                if (formatter->style.use_colors) {
                    fprintf(out, "  %s├─ %s%s%s", COLOR_BLUE, COLOR_YELLOW, 
                           current->structures[i].name, COLOR_RESET);
                } else {
                    fprintf(out, "  ├─ %s", current->structures[i].name);
                }

                if (current->structures[i].methods && current->structures[i].method_count > 0) {
                    fprintf(out, " (%d methods)", current->structures[i].method_count);
                }
                fprintf(out, "\n");

                // Print methods if layer filter includes them
                if (formatter->filters.layer_filter & FILTER_METHOD) {
                    for (int j = 0; j < current->structures[i].method_count; j++) {
                        if (formatter->style.use_colors) {
                            fprintf(out, "    %s└─ %s%s()%s", COLOR_BLUE, COLOR_CYAN,
                                   current->structures[i].methods[j].name, COLOR_RESET);
                        } else {
                            fprintf(out, "    └─ %s()", current->structures[i].methods[j].name);
                        }

                        if (current->structures[i].methods[j].return_type) {
                            fprintf(out, " -> %s", current->structures[i].methods[j].return_type);
                        }
                        fprintf(out, "\n");
                    }
                }
            }
        }

        fprintf(out, "\n");
        formatter->nodes_written++;
        current = current->next;
    }

    return 0;
}

int output_write_json(OutputFormatter* formatter, const ExtractedDependency* deps) {
    if (!formatter || !formatter->output_stream) return -1;

    FILE* out = formatter->output_stream;
    const ExtractedDependency* current = deps;
    bool first = true;

    fprintf(out, "{\n");
    fprintf(out, "  \"dependencies\": [\n");

    while (current) {
        if (!output_filter_dependency(current, &formatter->filters)) {
            current = current->next;
            continue;
        }

        if (!first) {
            fprintf(out, ",\n");
        }
        first = false;

        fprintf(out, "    {\n");
        fprintf(out, "      \"layer\": \"%s\",\n", output_layer_to_string(current->layer));
        fprintf(out, "      \"connection_type\": \"%s\",\n", output_connection_type_to_string(current));

        if (current->module_name) {
            char* escaped_name = output_escape_string(current->module_name, OUTPUT_FORMAT_JSON);
            fprintf(out, "      \"module_name\": \"%s\",\n", escaped_name);
            free(escaped_name);
        }

        if (current->target) {
            char* escaped_target = output_escape_string(current->target, OUTPUT_FORMAT_JSON);
            fprintf(out, "      \"target\": \"%s\",\n", escaped_target);
            free(escaped_target);
        }

        if (current->file_path) {
            char* escaped_path = output_escape_string(current->file_path, OUTPUT_FORMAT_JSON);
            fprintf(out, "      \"file_path\": \"%s\",\n", escaped_path);
            free(escaped_path);
        }

        // Add structures array
        if (current->structures && current->structure_count > 0) {
            fprintf(out, "      \"structures\": [\n");
            for (int i = 0; i < current->structure_count; i++) {
                if (i > 0) fprintf(out, ",\n");
                fprintf(out, "        {\n");
                fprintf(out, "          \"name\": \"%s\",\n", current->structures[i].name);
                fprintf(out, "          \"method_count\": %d\n", current->structures[i].method_count);
                fprintf(out, "        }");
            }
            fprintf(out, "\n      ]\n");
        } else {
            fprintf(out, "      \"structures\": []\n");
        }

        fprintf(out, "    }");
        formatter->nodes_written++;
        current = current->next;
    }

    fprintf(out, "\n  ],\n");
    
    // Add metadata
    time_t now = time(NULL);
    fprintf(out, "  \"metadata\": {\n");
    fprintf(out, "    \"generated_at\": \"%s\",", ctime(&now)); // Note: ctime includes \n
    fprintf(out, "    \"total_dependencies\": %zu,\n", formatter->nodes_written);
    fprintf(out, "    \"format_version\": \"1.0\"\n");
    fprintf(out, "  }\n");
    fprintf(out, "}\n");

    return 0;
}

int output_write_graphviz(OutputFormatter* formatter, const DependencyNode* nodes, const DependencyEdge* edges) {
    if (!formatter || !formatter->output_stream) return -1;

    FILE* out = formatter->output_stream;

    // Write GraphViz header
    fprintf(out, "digraph dependencies {\n");
    fprintf(out, "  rankdir=TB;\n");
    fprintf(out, "  node [shape=box, style=rounded];\n");
    fprintf(out, "  edge [arrowhead=vee];\n\n");

    // Write nodes
    const DependencyNode* current_node = nodes;
    while (current_node) {
        char* escaped_label = output_escape_string(current_node->label, OUTPUT_FORMAT_GRAPHVIZ);
        
        fprintf(out, "  \"%s\" [label=\"%s\"", current_node->id, escaped_label);
        
        if (current_node->color) {
            fprintf(out, ", color=\"%s\"", current_node->color);
        }
        if (current_node->shape) {
            fprintf(out, ", shape=\"%s\"", current_node->shape);
        }
        
        fprintf(out, "];\n");
        
        free(escaped_label);
        current_node = current_node->next;
    }

    fprintf(out, "\n");

    // Write edges
    const DependencyEdge* current_edge = edges;
    while (current_edge) {
        fprintf(out, "  \"%s\" -> \"%s\"", current_edge->source_id, current_edge->target_id);
        
        if (current_edge->label) {
            char* escaped_label = output_escape_string(current_edge->label, OUTPUT_FORMAT_GRAPHVIZ);
            fprintf(out, " [label=\"%s\"", escaped_label);
            free(escaped_label);
        } else {
            fprintf(out, " [");
        }
        
        if (current_edge->color) {
            fprintf(out, ", color=\"%s\"", current_edge->color);
        }
        if (current_edge->style) {
            fprintf(out, ", style=\"%s\"", current_edge->style);
        }
        
        fprintf(out, "];\n");
        
        current_edge = current_edge->next;
    }

    fprintf(out, "}\n");
    return 0;
}

int output_formatter_write_dependencies(OutputFormatter* formatter, const ExtractedDependency* dependencies) {
    if (!formatter || !dependencies) return -1;

    logr(DEBUG, "[OutputFormatter] Writing dependencies in format %d", formatter->format);

    switch (formatter->format) {
        case OUTPUT_FORMAT_TERMINAL:
            return output_write_terminal(formatter, dependencies);
            
        case OUTPUT_FORMAT_JSON:
            return output_write_json(formatter, dependencies);
            
        case OUTPUT_FORMAT_GRAPHVIZ:
            // For GraphViz, we need to build nodes and edges first
            {
                DependencyNode* nodes = output_build_nodes(dependencies, &formatter->filters);
                DependencyEdge* edges = output_build_edges(dependencies, &formatter->filters);
                int result = output_write_graphviz(formatter, nodes, edges);
                dependency_node_free(nodes);
                dependency_edge_free(edges);
                return result;
            }
            
        default:
            logr(ERROR, "[OutputFormatter] Unsupported output format: %d", formatter->format);
            return -1;
    }
}

OutputStats output_calculate_stats(const ExtractedDependency* dependencies) {
    OutputStats stats = {0};
    
    const ExtractedDependency* current = dependencies;
    while (current) {
        stats.total_dependencies++;
        
        if (current->layer < 3) {
            stats.dependencies_by_layer[current->layer]++;
        }
        
        // Note: Language counting would require additional language field in ExtractedDependency
        
        current = current->next;
    }
    
    return stats;
}

void output_print_stats(const OutputStats* stats, OutputFormat format, FILE* stream) {
    if (!stats || !stream) return;

    switch (format) {
        case OUTPUT_FORMAT_JSON:
            fprintf(stream, "{\n");
            fprintf(stream, "  \"statistics\": {\n");
            fprintf(stream, "    \"total_dependencies\": %zu,\n", stats->total_dependencies);
            fprintf(stream, "    \"module_dependencies\": %zu,\n", stats->dependencies_by_layer[0]);
            fprintf(stream, "    \"struct_dependencies\": %zu,\n", stats->dependencies_by_layer[1]);
            fprintf(stream, "    \"method_dependencies\": %zu\n", stats->dependencies_by_layer[2]);
            fprintf(stream, "  }\n");
            fprintf(stream, "}\n");
            break;
            
        default:
            fprintf(stream, "\nDependency Statistics:\n");
            fprintf(stream, "=====================\n");
            fprintf(stream, "Total Dependencies: %zu\n", stats->total_dependencies);
            fprintf(stream, "Module Level: %zu\n", stats->dependencies_by_layer[0]);
            fprintf(stream, "Struct Level: %zu\n", stats->dependencies_by_layer[1]);
            fprintf(stream, "Method Level: %zu\n", stats->dependencies_by_layer[2]);
            break;
    }
}

// Simplified implementations for node/edge building
DependencyNode* output_build_nodes(const ExtractedDependency* dependencies, const FilterConfig* filters) {
    // This is a simplified implementation
    // In a full implementation, this would build a proper graph structure
    return NULL;
}

DependencyEdge* output_build_edges(const ExtractedDependency* dependencies, const FilterConfig* filters) {
    // This is a simplified implementation
    // In a full implementation, this would build proper edge relationships
    return NULL;
}

void dependency_node_free(DependencyNode* nodes) {
    while (nodes) {
        DependencyNode* next = nodes->next;
        free(nodes->id);
        free(nodes->label);
        free(nodes->file_path);
        free(nodes->node_type);
        free(nodes->color);
        free(nodes->shape);
        free(nodes->style);
        free(nodes);
        nodes = next;
    }
}

void dependency_edge_free(DependencyEdge* edges) {
    while (edges) {
        DependencyEdge* next = edges->next;
        free(edges->source_id);
        free(edges->target_id);
        free(edges->label);
        free(edges->edge_type);
        free(edges->color);
        free(edges->style);
        free(edges->arrow_type);
        free(edges);
        edges = next;
    }
}