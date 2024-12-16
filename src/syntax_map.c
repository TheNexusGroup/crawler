#include "syntax_map.h"
#include <string.h>

void free_dependency(ExtractedDependency* dep) {
    if (!dep) return;
    
    free(dep->module_name);
    free(dep->file_path);
    
    // Free structures
    for (int i = 0; i < dep->structure_count; i++) {
        // Free methods within structures
        for (int j = 0; j < dep->structures[i].method_count; j++) {
            free(dep->structures[i].methods[j].name);
            free(dep->structures[i].methods[j].return_type);
            // Free parameters
            for (int k = 0; k < dep->structures[i].methods[j].param_count; k++) {
                free(dep->structures[i].methods[j].parameters[k].name);
                free(dep->structures[i].methods[j].parameters[k].type);
                free(dep->structures[i].methods[j].parameters[k].default_value);
            }
            free(dep->structures[i].methods[j].parameters);
        }
        free(dep->structures[i].methods);
        free(dep->structures[i].name);
        
        // Free implemented traits
        for (int j = 0; j < dep->structures[i].trait_count; j++) {
            free(dep->structures[i].implemented_traits[j]);
        }
        free(dep->structures[i].implemented_traits);
        
        // Free dependencies
        for (int j = 0; j < dep->structures[i].dependency_count; j++) {
            free(dep->structures[i].dependencies[j]);
        }
        free(dep->structures[i].dependencies);
    }
    free(dep->structures);
    
    // Free methods
    for (int i = 0; i < dep->method_count; i++) {
        free(dep->methods[i].name);
        free(dep->methods[i].return_type);
        for (int j = 0; j < dep->methods[i].param_count; j++) {
            free(dep->methods[i].parameters[j].name);
            free(dep->methods[i].parameters[j].type);
            free(dep->methods[i].parameters[j].default_value);
        }
        free(dep->methods[i].parameters);
    }
    free(dep->methods);
    
    free(dep);
}
