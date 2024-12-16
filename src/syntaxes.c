#include "syntaxes.h"
#include "pattern_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Analyze a file based on the given configuration
ExtractedDependency* analyze_file(const char* file_path, AnalysisConfig* config) {
    if (!file_path || !config) return NULL;

    // Read file content
    FILE* file = fopen(file_path, "r");
    if (!file) return NULL;

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    // Allocate memory for content
    char* content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    // Read file content
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    fclose(file);

    // Create dependency structure
    ExtractedDependency* dep = malloc(sizeof(ExtractedDependency));
    memset(dep, 0, sizeof(ExtractedDependency));
    dep->file_path = strdup(file_path);

    // Analyze each layer according to config
    if (config->analyze_modules) {
        // TODO: Implement module analysis
        dep->layer = LAYER_MODULE;
    }
    if (config->analyze_structures) {
        // Analyze structures in the file
        dep->structures = analyze_structure(content, NULL);
        if (dep->structures) {
            dep->structure_count = 1; // Update based on actual count
        }
    }
    if (config->analyze_methods) {
        // Analyze methods not associated with structures
        dep->methods = analyze_method(content);
        if (dep->methods) {
            dep->method_count = 1; // Update based on actual count
        }
    }

    free(content);
    return dep;
}

// Analyze structure definitions in the content
Structure* analyze_structure(const char* file_content, const char* struct_name) {
    if (!file_content) return NULL;

    Structure* structure = malloc(sizeof(Structure));
    memset(structure, 0, sizeof(Structure));

    if (struct_name) {
        structure->name = strdup(struct_name);
    }

    // Initialize arrays
    structure->methods = malloc(sizeof(Method) * MAX_METHODS_PER_STRUCT);
    structure->implemented_traits = malloc(sizeof(char*) * MAX_TRAITS_PER_STRUCT);
    structure->dependencies = malloc(sizeof(char*) * MAX_DEPENDENCIES);

    // TODO: Implement pattern matching for structure analysis
    // This would use the pattern_cache to find structure definitions

    return structure;
}

// Analyze method definitions
Method* analyze_method(const char* method_content) {
    if (!method_content) return NULL;

    Method* method = malloc(sizeof(Method));
    memset(method, 0, sizeof(Method));

    // Initialize parameters array
    method->param_count = 0;

    // TODO: Implement pattern matching for method analysis
    // This would use the pattern_cache to find method definitions

    return method;
}

// Create a dependency graph from extracted dependencies
DependencyGraph* create_dependency_graph(ExtractedDependency** deps, int dep_count) {
    if (!deps || dep_count <= 0) return NULL;

    DependencyGraph* graph = malloc(sizeof(DependencyGraph));
    graph->relationships = malloc(sizeof(Relationship*) * dep_count * MAX_DEPENDENCIES);
    graph->relationship_count = 0;
    graph->current_layer = LAYER_MODULE;

    for (int i = 0; i < dep_count; i++) {
        ExtractedDependency* dep = deps[i];
        if (!dep) continue;

        // Process module-level relationships
        if (dep->module_name) {
            Relationship* rel = malloc(sizeof(Relationship));
            rel->from = strdup(dep->file_path);
            rel->to = strdup(dep->module_name);
            rel->relationship_type = strdup("imports");
            rel->layer = LAYER_MODULE;
            graph->relationships[graph->relationship_count++] = rel;
        }

        // Process structure-level relationships
        for (int j = 0; j < dep->structure_count; j++) {
            Structure* structure = &dep->structures[j];
            for (int k = 0; k < structure->dependency_count; k++) {
                Relationship* rel = malloc(sizeof(Relationship));
                rel->from = strdup(structure->name);
                rel->to = strdup(structure->dependencies[k]);
                rel->relationship_type = strdup("depends_on");
                rel->layer = LAYER_STRUCT;
                graph->relationships[graph->relationship_count++] = rel;
            }
        }
    }

    return graph;
}

// Export the dependency graph to a specific format
void export_graph(DependencyGraph* graph, const char* format, const char* output_path) {
    if (!graph || !format || !output_path) return;

    FILE* output = fopen(output_path, "w");
    if (!output) return;

    if (strcmp(format, "dot") == 0) {
        // Export as GraphViz DOT format
        fprintf(output, "digraph Dependencies {\n");
        for (int i = 0; i < graph->relationship_count; i++) {
            Relationship* rel = graph->relationships[i];
            fprintf(output, "  \"%s\" -> \"%s\" [label=\"%s\"];\n",
                    rel->from, rel->to, rel->relationship_type);
        }
        fprintf(output, "}\n");
    } else if (strcmp(format, "json") == 0) {
        // Export as JSON format
        fprintf(output, "{\n  \"relationships\": [\n");
        for (int i = 0; i < graph->relationship_count; i++) {
            Relationship* rel = graph->relationships[i];
            fprintf(output, "    {\n");
            fprintf(output, "      \"from\": \"%s\",\n", rel->from);
            fprintf(output, "      \"to\": \"%s\",\n", rel->to);
            fprintf(output, "      \"type\": \"%s\",\n", rel->relationship_type);
            fprintf(output, "      \"layer\": %d\n", rel->layer);
            fprintf(output, "    }%s\n", i < graph->relationship_count - 1 ? "," : "");
        }
        fprintf(output, "  ]\n}\n");
    }

    fclose(output);
}
