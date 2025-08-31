# CLAUDE.md - Dependency Crawler Project

## Project Overview

The Dependency Crawler is a high-performance, multi-language static analysis tool written in C that maps and visualizes code dependencies across different programming languages. The project implements a sophisticated three-layer analysis system to understand relationships between files, structures, and methods across large codebases.

## Build Commands

```bash
# Build the project
./build.sh

# Run the crawler
./crawler [OPTIONS] [ENTRY_POINT]

# Basic usage examples
./crawler                           # Crawl current directory
./crawler /path/to/project         # Crawl specific directory  
./crawler -v -o json ./src         # Verbose JSON output
```

## Architecture Overview

### Core Components

1. **Three-Layer Analysis System**:
   - `LAYER_MODULE`: Files, modules, packages, import statements
   - `LAYER_STRUCT`: Classes, structs, traits, interfaces
   - `LAYER_METHOD`: Functions, methods, parameters, dependencies

2. **Multi-Language Support Framework**:
   - Currently: 8 languages defined (only Rust fully implemented)
   - Languages: Rust, C/C++, JavaScript, Go, Python, Java, PHP, Ruby
   - Extensible pattern-based parsing system

3. **Memory Management**:
   - Linked list data structures for dependencies
   - Comprehensive free functions for all allocated structures
   - Pattern caching system for compiled regex patterns

4. **Key Data Structures**:
   - `ExtractedDependency`: Core dependency representation
   - `Structure`: Class/struct definitions with methods and traits
   - `Method`: Function signatures with parameters and return types
   - `LanguageGrammar`: Language-specific parsing rules

### File Organization

- **src/main.c**: Entry point with CLI argument parsing
- **src/crawler.c/h**: Main crawler orchestration logic
- **src/dependency.c/h**: Core dependency data structures
- **src/analyzers.c/h**: Language-specific analysis implementations
- **src/syntaxes.c/h**: Language detection and naming
- **src/grammars.c/h**: Pattern definitions for each language
- **src/structure.c/h**: Structure analysis and collection
- **src/method.c**: Method analysis and parameter extraction
- **src/pattern_cache.c/h**: Regex compilation and caching
- **src/logger.c/h**: Debugging and logging system

## Project Goals

### Primary Objectives

1. **Enterprise-Scale Performance**: Handle massive codebases (100k+ files) efficiently
2. **Deep Dependency Analysis**: Map relationships from file-level imports down to function parameter dependencies
3. **Multi-Language Mastery**: Complete implementation for all 8 target languages
4. **Layered Filtering**: Allow users to filter output by connection type (module, struct, method)
5. **Rich Visualization**: Generate meaningful graphs, diagrams, and interactive outputs
6. **Lightning-Fast Execution**: Optimize for sub-second analysis of medium-sized projects

### Strategic Vision

- **Developer Workflow Integration**: Seamless CI/CD pipeline integration
- **Refactoring Support**: Identify impact of code changes across dependency chains
- **Architecture Visualization**: Clear visual representation of system architecture
- **Technical Debt Detection**: Highlight circular dependencies and coupling issues

## Comprehensive TODO

### Priority 1: Core Infrastructure ✅
- [x] Sync and push current state to upstream git repository
- [x] Update CLAUDE.md with project goals and comprehensive TODO

### Priority 2: Testing & Validation (In Progress)
- [ ] Create comprehensive test suite with multiple language examples
  - [ ] `./tests/js/` - Complex JavaScript/TypeScript project with imports
  - [ ] `./tests/py/` - Python project with modules and classes  
  - [ ] `./tests/go/` - Go project with packages and interfaces
  - [ ] `./tests/rust/` - Advanced Rust project with traits and generics
  - [ ] `./tests/c/` - C project with headers and function dependencies
  - [ ] `./tests/java/` - Java project with inheritance and packages
  - [ ] `./tests/php/` - PHP project with namespaces and traits
  - [ ] `./tests/ruby/` - Ruby project with modules and mixins
- [ ] Each test should include nested dependencies and complex linking
- [ ] Generate example outputs in `./examples/` for each test case

### Priority 3: Language Implementation (Critical)
- [ ] Complete Rust analyzer implementation (currently partial)
- [ ] Implement C/C++ analyzer (header dependencies, function calls)
- [ ] Implement JavaScript/TypeScript analyzer (imports, exports, classes)
- [ ] Implement Go analyzer (packages, interfaces, struct embedding)
- [ ] Implement Python analyzer (imports, classes, decorators)
- [ ] Implement Java analyzer (packages, inheritance, annotations)
- [ ] Implement PHP analyzer (namespaces, traits, includes)
- [ ] Implement Ruby analyzer (modules, mixins, requires)

### Priority 4: Performance Optimization
- [ ] Profile memory usage and optimize allocation patterns
- [ ] Implement parallel processing for multiple files
- [ ] Add progress indicators for large codebase analysis
- [ ] Optimize regex compilation and caching
- [ ] Implement incremental analysis for changed files only
- [ ] Add memory-mapped file reading for large files

### Priority 5: Output & Visualization
- [ ] Complete JSON output format implementation
- [ ] Implement GraphViz (.dot) output generation
- [ ] Add interactive HTML output with filtering
- [ ] Create layered dependency filtering system:
  - Module-level dependencies only
  - Struct-level relationships
  - Method-level connections
  - Parameter-level tracking
- [ ] Generate dependency matrices and metrics
- [ ] Add circular dependency detection and warnings

### Priority 6: Advanced Features
- [ ] Configuration file support (.depcrawler.conf)
- [ ] Custom ignore patterns and filters  
- [ ] External dependency resolution (package.json, Cargo.toml, etc.)
- [ ] Plugin system for custom analyzers
- [ ] API endpoints for integration tools
- [ ] Real-time file watching and incremental updates

### Priority 7: Quality & Maintenance
- [ ] Add comprehensive error handling and recovery
- [ ] Implement memory leak detection and testing
- [ ] Add benchmark suite for performance regression testing
- [ ] Create extensive documentation and usage examples
- [ ] Add unit tests for all core functions
- [ ] Implement fuzzing tests for parser robustness

## Development Guidelines

### Code Standards
- Follow existing C style and naming conventions
- Always provide corresponding free functions for allocated structures
- Use defensive programming practices (null checks, bounds checking)
- Maintain clear separation between language-specific and generic code
- Document complex algorithms and data structure relationships

### Testing Strategy
- Each language analyzer must have dedicated test cases
- Test edge cases: empty files, syntax errors, complex nesting
- Benchmark performance on large codebases (>10k files)
- Validate memory management with valgrind
- Test output formats for correctness and completeness

### Performance Targets
- **Small projects** (< 100 files): < 1 second
- **Medium projects** (< 1000 files): < 5 seconds  
- **Large projects** (< 10k files): < 30 seconds
- **Enterprise projects** (< 100k files): < 5 minutes

## Current Limitations

1. **Language Support**: Only Rust partially implemented
2. **Output Formats**: Terminal output only, JSON/GraphViz incomplete
3. **Memory Usage**: No optimization for very large codebases
4. **Error Handling**: Limited error recovery and reporting
5. **Configuration**: No configuration file support
6. **Performance**: Single-threaded processing only

## Critical Success Factors

1. **Accuracy**: Dependency analysis must be precise and reliable
2. **Performance**: Must handle enterprise-scale codebases efficiently
3. **Usability**: Clear, actionable output for developers
4. **Maintainability**: Clean, extensible code architecture
5. **Completeness**: Full language support as advertised

## Next Immediate Actions

1. Create comprehensive test suite structure
2. Implement missing language analyzers starting with most common (JavaScript, Python)
3. Complete JSON output format implementation
4. Add performance benchmarking and memory profiling
5. Generate example outputs and documentation

---

*This document serves as the authoritative guide for all development work on the Dependency Crawler project. Update this document as goals and priorities evolve.*