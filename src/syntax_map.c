#include "syntax_map.h"
#include <string.h>
#include <stdlib.h>

void freeDependency(ExtractedDependency* dep) {
    while (dep) {
        ExtractedDependency* next = dep->next;
        
        // Free module name and file path
        free(dep->module_name);
        free(dep->file_path);
        
        // Free structures
        if (dep->structures) {
            for (int i = 0; i < dep->structure_count; i++) {
                free(dep->structures[i].name);
                
                // Free implemented traits
                if (dep->structures[i].implemented_traits) {
                    for (int j = 0; j < dep->structures[i].trait_count; j++) {
                        free(dep->structures[i].implemented_traits[j]);
                    }
                    free(dep->structures[i].implemented_traits);
                }
                
                // Free dependencies (it's a single string now)
                free(dep->structures[i].dependencies);
                
                // Free methods
                if (dep->structures[i].methods) {
                    for (int j = 0; j < dep->structures[i].method_count; j++) {
                        free(dep->structures[i].methods[j].name);
                        free(dep->structures[i].methods[j].return_type);
                        free(dep->structures[i].methods[j].dependencies);
                    }
                    free(dep->structures[i].methods);
                }
            }
            free(dep->structures);
        }
        
        // Free methods at dependency level
        if (dep->methods) {
            for (int i = 0; i < dep->method_count; i++) {
                free(dep->methods[i].name);
                free(dep->methods[i].return_type);
                free(dep->methods[i].dependencies);
            }
            free(dep->methods);
        }
        
        // Free target
        free(dep->target);
        
        // Free the dependency itself
        free(dep);
        dep = next;
    }
}
