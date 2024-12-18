#ifndef PATTERNS_H
#define PATTERNS_H

#include "grammars.h"

// First Layer: Module-level patterns (existing patterns)
static const char* RUST_MODULE_PATTERNS[] = {
    "^\\s*use\\s+([a-zA-Z0-9_:]+)",
    "^\\s*extern\\s+crate\\s+([a-zA-Z0-9_]+)",
    "^\\s*mod\\s+([a-zA-Z0-9_]+)",
    "^\\s*include!\\s*\\(\\s*\"([^\"]+)\"\\s*\\)",
};

// Second Layer: Struct/Class-level patterns
static const char* RUST_STRUCT_PATTERNS[] = {
    "^\\s*struct\\s+([a-zA-Z0-9_]+)",     // Basic struct definition
    "^\\s*enum\\s+([a-zA-Z0-9_]+)",       // Enum definition
    "^\\s*trait\\s+([a-zA-Z0-9_]+)",      // Trait definition
    "^\\s*impl\\s+([a-zA-Z0-9_]+)"        // Implementation block
};

// Third Layer: Method/Parameter patterns
static const char* RUST_METHOD_PATTERNS[] = {
    "^\\s*fn\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)",     // Basic function definition
    "^\\s*self\\.([a-zA-Z0-9_]+)\\s*\\(.*\\)",         // Method calls
    "^\\s*([a-zA-Z0-9_]+)::([a-zA-Z0-9_]+)\\s*\\(.*\\)" // Static method calls
};

// C/C++ patterns for each layer
static const char* C_MODULE_PATTERNS[] = {
    "^\\s*#include\\s*[<\"]([^>\"]+)[>\"]",
    "^\\s*#import\\s*[<\"]([^>\"]+)[>\"]",
    "^\\s*#pragma\\s+once",
};

static const char* C_STRUCT_PATTERNS[] = {
    "(typedef\\s+struct)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\{[^}]*\\}\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*;",  // typedef struct X { ... } Y;
    "(typedef\\s+struct)\\s*\\{[^}]*\\}\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*;",  // typedef struct { ... } X;
    "(struct)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\{",  // struct X {
    "(typedef\\s+enum)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\{[^}]*\\}\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*;",  // typedef enum X { ... } Y;
    "(enum)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\{",  // enum X {
    "(class)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\{",  // class X {
    "(union)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\{",  // union X {
    "(template)\\s+<\\s*class\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*>",  // template <class X>
    "(template)\\s+<\\s*typename\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*>",  // template <typename X>
};

static const char* C_METHOD_PATTERNS[] = {
    // Function definition with return type and name
    "([a-zA-Z_][a-zA-Z0-9_]*\\s+)+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^)]*)\\)\\s*\\{",
    
    // Function with storage class specifiers
    "(static|extern|inline)\\s+([a-zA-Z_][a-zA-Z0-9_]*\\s+)+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\([^)]*)\\)\\s*\\{",
    
    // Function pointer typedef
    "typedef\\s+([a-zA-Z_][a-zA-Z0-9_]*\\s+)+\\(\\*([a-zA-Z_][a-zA-Z0-9_]*)\\)\\s*\\([^)]*\\)",
    
    // Method calls
    "([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\([^)]*\\)",
    
    // Method calls with namespace/scope
    "([a-zA-Z_][a-zA-Z0-9_]*)::[a-zA-Z_][a-zA-Z0-9_]*\\s*\\([^)]*\\)"
};

// JavaScript/TypeScript patterns
static const char* JS_MODULE_PATTERNS[] = {
    "^\\s*import\\s+.*\\s+from\\s+['\"]([^'\"]+)['\"]",    // ES6 imports
    "^\\s*require\\s*\\(['\"]([^'\"]+)['\"]\\)",           // CommonJS require
    "^\\s*export\\s+.*\\s+from\\s+['\"]([^'\"]+)['\"]",    // ES6 re-exports
};

static const char* JS_STRUCT_PATTERNS[] = {
    "^\\s*class\\s+([a-zA-Z0-9_]+)",             // Class definitions
    "^\\s*interface\\s+([a-zA-Z0-9_]+)",         // TypeScript interfaces
    "^\\s*type\\s+([a-zA-Z0-9_]+)\\s*="         // TypeScript type aliases
};

static const char* JS_METHOD_PATTERNS[] = {
    "^\\s*function\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)",       // Regular functions
    "^\\s*async\\s+function\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)", // Async functions
    "^\\s*([a-zA-Z0-9_]+)\\s*[=:]\\s*function\\s*\\(([^)]*)\\)" // Function expressions
};

// Python patterns
static const char* PYTHON_MODULE_PATTERNS[] = {
    "^\\s*import\\s+([a-zA-Z0-9_.]+)",                     // Import statements
    "^\\s*from\\s+([a-zA-Z0-9_.]+)\\s+import",            // From imports
    "^\\s*__import__\\s*\\(['\"]([^'\"]+)['\"]\\)",       // Dynamic imports
};

static const char* PYTHON_STRUCT_PATTERNS[] = {
    "^\\s*class\\s+([a-zA-Z0-9_]+)",            // Class definitions
    "^\\s*@dataclass\\s*class\\s+([a-zA-Z0-9_]+)" // Dataclass definitions
};

static const char* PYTHON_METHOD_PATTERNS[] = {
    // Match Python functions
    "^\\s*def\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(",
    "^\\s*async\\s+def\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\("
};

// Java patterns
static const char* JAVA_MODULE_PATTERNS[] = {
    "^\\s*import\\s+([a-zA-Z0-9_.]+\\*?);",           // Import statements (with optional *)
    "^\\s*package\\s+([a-zA-Z0-9_.]+);"               // Package declarations
};

static const char* JAVA_STRUCT_PATTERNS[] = {
    "^\\s*public\\s+class\\s+([a-zA-Z0-9_]+)",  // Public class
    "^\\s*class\\s+([a-zA-Z0-9_]+)",            // Package-private class
    "^\\s*public\\s+interface\\s+([a-zA-Z0-9_]+)", // Public interface
    "^\\s*interface\\s+([a-zA-Z0-9_]+)",        // Package-private interface
    "^\\s*public\\s+enum\\s+([a-zA-Z0-9_]+)",   // Public enum
    "^\\s*enum\\s+([a-zA-Z0-9_]+)"              // Package-private enum
};

static const char* JAVA_METHOD_PATTERNS[] = {
    "^\\s*public\\s+[a-zA-Z0-9_<>]+\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)",    // Public methods
    "^\\s*private\\s+[a-zA-Z0-9_<>]+\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)",   // Private methods
    "^\\s*protected\\s+[a-zA-Z0-9_<>]+\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)", // Protected methods
    "^\\s*[a-zA-Z0-9_<>]+\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)"              // Package-private methods
};

// Go patterns
static const char* GO_MODULE_PATTERNS[] = {
    "^\\s*import\\s+[(\"]([^)\"]+)[\")]",                 // Import statements
    "^\\s*package\\s+([a-zA-Z0-9_]+)",                    // Package declarations
};

static const char* GO_STRUCT_PATTERNS[] = {
    "^\\s*type\\s+([a-zA-Z0-9_]+)\\s+struct",   // Struct definitions
    "^\\s*type\\s+([a-zA-Z0-9_]+)\\s+interface" // Interface definitions
};

static const char* GO_METHOD_PATTERNS[] = {
    "^\\s*func\\s+\\(([^)]*)\\)\\s*([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)", // Method definitions
    "^\\s*func\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)"                    // Function definitions
};

// PHP patterns
static const char* PHP_MODULE_PATTERNS[] = {
    "^\\s*require\\s+['\"]([^'\"]+)['\"]",          // require
    "^\\s*require_once\\s+['\"]([^'\"]+)['\"]",     // require_once
    "^\\s*include\\s+['\"]([^'\"]+)['\"]",          // include
    "^\\s*include_once\\s+['\"]([^'\"]+)['\"]",     // include_once
    "^\\s*namespace\\s+([a-zA-Z0-9_\\\\]+)",        // namespace
    "^\\s*use\\s+([a-zA-Z0-9_\\\\]+)"              // use
};

static const char* PHP_STRUCT_PATTERNS[] = {
    "^\\s*class\\s+([a-zA-Z0-9_]+)",                      // Class definitions
    "^\\s*interface\\s+([a-zA-Z0-9_]+)",                 // Interface definitions
    "^\\s*trait\\s+([a-zA-Z0-9_]+)"                      // Trait definitions
};

static const char* PHP_METHOD_PATTERNS[] = {
    "^\\s*public\\s+function\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)",    // Public methods
    "^\\s*private\\s+function\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)",   // Private methods
    "^\\s*protected\\s+function\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)", // Protected methods
    "^\\s*function\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)"              // Regular functions
};

// Ruby patterns
static const char* RUBY_MODULE_PATTERNS[] = {
    "^\\s*require\\s+['\"]([^'\"]+)['\"]",          // require
    "^\\s*require_relative\\s+['\"]([^'\"]+)['\"]", // require_relative
    "^\\s*module\\s+([a-zA-Z0-9_:]+)"              // module
};

static const char* RUBY_STRUCT_PATTERNS[] = {
    "^\\s*class\\s+([a-zA-Z0-9_]+)",                      // Class definitions
    "^\\s*module\\s+([a-zA-Z0-9_]+)"                      // Module definitions
};

static const char* RUBY_METHOD_PATTERNS[] = {
    "^\\s*def\\s+([a-zA-Z0-9_?!]+)",                     // Method definitions
    "^\\s*define_method\\s+:([a-zA-Z0-9_?!]+)"          // Dynamic method definitions
};

// Svelte patterns
static const char* SVELTE_MODULE_PATTERNS[] = {
    "^\\s*import\\s+.*\\s+from\\s+['\"]([^'\"]+)['\"]",  // imports
    "^\\s*export\\s+.*\\s+from\\s+['\"]([^'\"]+)['\"]"   // exports
};

static const char* SVELTE_STRUCT_PATTERNS[] = {
    "^\\s*<script[^>]*>",                       // Script block
    "^\\s*<style[^>]*>"                        // Style block
};

static const char* SVELTE_METHOD_PATTERNS[] = {
    "^\\s*function\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)", // Function definitions
    "^\\s*const\\s+([a-zA-Z0-9_]+)\\s*=\\s*\\(([^)]*)\\)\\s*=>", // Arrow functions
    "^\\s*export\\s+function\\s+([a-zA-Z0-9_]+)\\s*\\(([^)]*)\\)" // Exported functions
};

static const char* RUST_KEYWORDS[] = {
    "if", "else", "while", "for", "loop", "match", "break", "continue",
    "return", "yield", "in", "where", "move", "mut", "ref", "type"
};

// Language keywords to filter from method detection
static const char* C_KEYWORDS[] = {
    "if", "else", "while", "for", "do", "switch", "case", "break", "continue", 
    "return", "goto", "sizeof", "typedef", "volatile", "register", "auto"
};

static const char* PYTHON_KEYWORDS[] = {
    "if", "else", "elif", "while", "for", "in", "try", "except", "finally",
    "with", "break", "continue", "return", "yield", "pass", "raise"
};

static const char* JAVA_KEYWORDS[] = {
    "if", "else", "while", "for", "do", "switch", "case", "break", "continue",
    "return", "try", "catch", "finally", "throw", "instanceof", "new"
};

static const char* RUBY_KEYWORDS[] = {
    "if", "else", "elsif", "unless", "while", "until", "for", "break", "next",
    "return", "yield", "begin", "rescue", "ensure", "retry", "redo"
};

static const char* GO_KEYWORDS[] = {
    "if", "else", "for", "range", "switch", "case", "break", "continue",
    "return", "goto", "fallthrough", "defer", "select", "type"
};

static const char* PHP_KEYWORDS[] = {
    "if", "else", "while", "for", "foreach", "do", "switch", "case", "break",
    "continue", "return", "try", "catch", "finally", "throw", "instanceof"
};

static const char* JS_KEYWORDS[] = {
    "if", "else", "while", "for", "do", "switch", "case", "break", "continue",
    "return", "try", "catch", "finally", "throw", "typeof", "instanceof",
    "new", "class", "extends", "super", "import", "export", "default", "await"
};

static const char* SVELTE_KEYWORDS[] = {
    "if", "else", "each", "await", "then", "catch", "as",
    "export", "const", "let", "var", "function", "import", "from"
};


#endif // PATTERNS_H