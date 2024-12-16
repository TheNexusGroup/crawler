# Dependency Crawler

## Overview

Crawler is a versatile, multi-language static analysis tool designed to map and visualize dependencies across different programming languages and project structures. It provides comprehensive insights into code relationships at various levels of granularity.

## Features

- Support for multiple programming languages:
  - Rust (.rs)
  - C/C++ (.c, .cpp, .h) (todo)
  - JavaScript (.js) (todo)
  - Go (.go) (todo)
  - Python (.py) (todo)

- Dependency Mapping Levels:
  - File and directory dependencies
  - Module imports
  - Class relationships
  - Function dependencies
  - Parameter-level connections

- Flexible Crawling Options:
  - Crawl current working directory by default
  - Specify custom entry point directory
  - Analyze library/dependency directories
  - Configurable depth and scope of analysis

## Installation

```bash
git clone https://github.com/vaziolabs/crawler.git
cd crawler
make
```

## Usage

### Basic Usage

```bash
# Crawl current directory
./crawler

# Crawl specific directory
./crawler /path/to/project

# Crawl with library directory
./crawler /path/to/project -l /path/to/libraries
```

### Command Line Options

```
Usage: crawler [OPTIONS] [ENTRY_POINT]

Options:
  -l, --library DIR     Specify additional library directory to search for dependencies
  -d, --depth NUM       Set maximum crawl depth (default: unlimited)
  -o, --output FORMAT   Output format
  -v, --verbose         Enable verbose output
  --help                Show this help message
```

### Output Formats

- `terminal`: Human-readable console output (default)
- `json`: Structured JSON for further analysis (todo)
- `graphviz`: Graph visualization format (not implemented)

## Configuration

Create a `.depcrawler.conf` in your project root to customize:
- Ignored directories
- Language-specific parsing rules
- Custom dependency filters

Example configuration:
```toml
[global]
ignore_dirs = ["node_modules", "target", "build"]

[rust]
parse_private_modules = true

[python]
follow_imports = true
```

## Example Scenarios

### 1. Analyzing a Rust Project

```bash
./crawler ~/projects/my-rust-project
```

### 2. Deep Analysis with Library Dependencies

```bash
./crawler ~/projects/complex-app -l ~/projects/shared-libraries -d 3 -o json
```

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Limitations

- Static analysis only (no runtime dependency tracking)
- Parsing complexity varies by language
- Large projects may require significant memory

## License

Distributed under the Ancillary License. See `LICENSE` for more information.

## Contact

Your Name - dev@vaziolabs.com

Project Link: [https://github.com/vaziolabs/crawler](https://github.com/vaziolabs/crawler)
