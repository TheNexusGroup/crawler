# Dependency Crawler Examples

This directory contains example outputs and visualizations demonstrating the capabilities of the Dependency Crawler across different programming languages.

## Contents

### Analysis Examples
- **js_example_analysis.md** - Complete JavaScript project analysis showing all three layers (module, structure, method)
- **rust_example_analysis.md** - Rust project analysis with trait dependencies and ownership patterns
- **python_dependencies.dot** - GraphViz visualization of Python project dependencies
- **go_dependencies.json** - JSON format analysis of Go project with package structure
- **java_class_diagram.mmd** - Mermaid class diagram showing Java Spring Boot application structure

### Output Formats Demonstrated

#### 1. Terminal Output
Human-readable console output with color coding and hierarchical structure display.

#### 2. JSON Output
Structured data format suitable for integration with other tools and automated processing.

#### 3. GraphViz (DOT) Format
Professional graph visualizations showing dependency relationships with customizable styling.

#### 4. Mermaid Diagrams
Modern diagram format for documentation, supporting:
- Dependency graphs
- Class diagrams
- Flowcharts
- Sequence diagrams

## Language-Specific Features

### JavaScript/TypeScript
- ES6 import/export analysis
- CommonJS require statements
- Class and method detection
- Async/await pattern recognition
- External vs internal dependency classification

### Python
- Import statement analysis
- Class hierarchy detection
- Decorator pattern recognition
- Async function analysis
- Package structure mapping

### Rust
- Cargo.toml dependency analysis
- Trait and implementation relationships
- Ownership and lifetime patterns
- Async trait detection
- Error handling pattern analysis

### Go
- Go module analysis (go.mod)
- Package import analysis
- Interface and struct relationships
- Method receiver patterns
- Standard library vs external dependency classification

### Java
- Package and import analysis
- Class inheritance hierarchies
- Interface implementation detection
- Annotation processing
- Spring framework pattern recognition

### C/C++
- Include statement analysis
- Header dependency tracking
- Function declaration/definition relationships
- Macro usage detection
- Template instantiation analysis

### PHP
- Namespace and use statement analysis
- Class and trait relationships
- Composer dependency tracking
- Method visibility detection
- Include/require statement processing

### Ruby
- Module and class analysis
- Mixin pattern detection
- Gem dependency tracking
- Method definition patterns
- Metaprogramming feature detection

## Filtering Examples

### Layer-Based Filtering
```bash
# Only module-level dependencies
./crawler --layer module ./tests/js

# Only structure-level (classes/structs)
./crawler --layer struct ./tests/rust

# Only method-level dependencies
./crawler --layer method ./tests/go
```

### Pattern-Based Filtering
```bash
# Exclude test files
./crawler --exclude "*test*" ./tests

# Include only specific file types
./crawler --include "*.js" --include "*.ts" ./project

# External dependencies only
./crawler --external-only ./tests/python
```

### Output Format Selection
```bash
# Generate GraphViz output
./crawler -o graphviz -f dependencies.dot ./tests/rust

# Generate JSON for processing
./crawler -o json -f analysis.json ./tests/go

# Generate Mermaid diagram
./crawler -o mermaid -f diagram.mmd ./tests/java
```

## Performance Examples

### Large Codebase Analysis
```bash
# Use parallel processing with 8 threads
./crawler --parallel -j 8 ./large-project

# Enable incremental analysis
./crawler --incremental --cache ./large-project

# Use memory pool optimization
./crawler --memory-pool 256 ./large-project
```

### Batch Processing
```bash
# Analyze multiple projects
./crawler --batch projects.txt

# Generate reports for all
./crawler --batch-output ./reports projects.txt
```

## Integration Examples

### CI/CD Pipeline
```yaml
# Example GitHub Actions integration
- name: Analyze Dependencies
  run: |
    ./crawler -o json -f dependencies.json ./src
    ./tools/validate-deps.sh dependencies.json
```

### Development Workflow
```bash
# Pre-commit hook
./crawler --incremental --quiet ./src
if [ $? -ne 0 ]; then
  echo "Dependency analysis failed"
  exit 1
fi
```

## Visualization Best Practices

### GraphViz Layouts
- **dot**: Hierarchical layouts (good for imports)
- **neato**: Force-directed layouts (good for complex relationships)
- **fdp**: Force-directed with clustering
- **sfdp**: Large graph layouts
- **twopi**: Radial layouts
- **circo**: Circular layouts

### Color Coding Conventions
- **Red**: External dependencies
- **Blue**: Entry points/main files
- **Green**: Service/business logic layers
- **Yellow**: Data models/entities
- **Orange**: Database/persistence layers
- **Purple**: Utilities/helpers
- **Gray**: Configuration/setup

### Node Shapes
- **Box**: Regular modules/classes
- **Ellipse**: External dependencies
- **Diamond**: Interfaces/traits
- **Hexagon**: Configuration/setup
- **Circle**: Utilities/helpers

## Advanced Analysis Features

### Circular Dependency Detection
The crawler automatically detects and reports circular dependencies across all layers.

### Dependency Metrics
- **Fan-in**: Number of modules that depend on this module
- **Fan-out**: Number of modules this module depends on
- **Complexity**: Overall dependency complexity score
- **Coupling**: Degree of coupling between modules

### Architecture Analysis
- **Layering violations**: Dependencies that break architectural layers
- **Interface segregation**: Analysis of interface usage patterns
- **Dependency inversion**: Detection of dependency inversion principle adherence

## Generating These Examples

To regenerate these examples with the latest version of the crawler:

```bash
# Generate all examples
./scripts/generate-examples.sh

# Generate specific language example
./scripts/generate-examples.sh --language rust

# Generate specific format
./scripts/generate-examples.sh --format graphviz
```