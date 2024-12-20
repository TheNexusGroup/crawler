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
    // Captures: name(1), params(2), return_type(3)
    "^[\\s]*fn[\\s]+([a-zA-Z_][a-zA-Z0-9_]*)[\\s]*\\(([^)]*)\\)[\\s]*->[\\s]*([^{;[\\s]]+)"
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
    // Basic function pattern - just capture the whole declaration
    "^[\\s]*([a-zA-Z_][a-zA-Z0-9_\\s*]+\\([^)]*\\))[\\s]*[{;]",
    
    // Function call pattern
    "([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^;{)]*)\\)"
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
    // Regular function - Group 1: name, Group 2: params
    "^[\\s]*def[\\s]+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^\\)]*)\\)\\s*:",
    
    // Async function - Group 1: name, Group 2: params  
    "^[\\s]*async[\\s]+def[\\s]+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^\\)]*)\\)\\s*:"
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
    // Public methods - Group 1: return type, Group 2: name, Group 3: params
    "^[\\s]*public[\\s]+([a-zA-Z0-9_<>\\[\\]]+)[\\s]+([a-zA-Z0-9_]+)\\s*\\(([^\\)]*)\\)[\\s]*\\{",
    
    // Private methods
    "^[\\s]*private[\\s]+([a-zA-Z0-9_<>\\[\\]]+)[\\s]+([a-zA-Z0-9_]+)\\s*\\(([^\\)]*)\\)[\\s]*\\{",
    
    // Protected methods
    "^[\\s]*protected[\\s]+([a-zA-Z0-9_<>\\[\\]]+)[\\s]+([a-zA-Z0-9_]+)\\s*\\(([^\\)]*)\\)[\\s]*\\{",
    
    // Package-private methods
    "^[\\s]*([a-zA-Z0-9_<>\\[\\]]+)[\\s]+([a-zA-Z0-9_]+)\\s*\\(([^\\)]*)\\)[\\s]*\\{"
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

static const char* RUST_KEYWORDS[] = {
    "if", "else", "while", "for", "loop", "match", "break", "continue",
    "return", "yield", "in", "where", "move", "mut", "ref", "type"
};

// Language keywords to filter from method detection
static const char* C_KEYWORDS[] = {
    "if", "else", "while", "for", "do", "switch", "case", "break", "continue", 
    "return", "goto", "sizeof", "typedef", "volatile", "register",
};

static const char* C_TYPES[] = {
    "void", "int", "float", "double", "char", "bool", "struct", "enum", "union", "typedef", "void*", "int*", "float*", "double*", "char*", "bool*", "struct*", "enum*", "union*", "typedef*"
};

static const char* C_PREFIXES[] = {
    "static", "extern", "inline", "const", "volatile", "register", "auto"
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

// Rust types and declarations
static const char* RUST_TYPES[] = {
    "i8", "i16", "i32", "i64", "i128", "u8", "u16", "u32", "u64", "u128",
    "f32", "f64", "bool", "char", "str", "String", "Vec", "Option", "Result"
};

static const char* RUST_PREFIXES[] = {
    "let", "const", "static", "fn", "struct", "enum", "trait", "impl", "type"
};

// JavaScript/TypeScript types and declarations
static const char* JS_TYPES[] = {
    "number", "string", "boolean", "object", "undefined", "symbol", "bigint",
    "any", "void", "never", "unknown"
};

static const char* JS_PREFIXES[] = {
    "var", "let", "const", "function", "class", "interface", "type", "enum"
};

// Python types and declarations
static const char* PYTHON_TYPES[] = {
    "int", "float", "str", "bool", "list", "tuple", "dict", "set", "frozenset",
    "bytes", "bytearray", "memoryview", "complex"
};

static const char* PYTHON_PREFIXES[] = {
    "def", "class", "lambda", "global", "nonlocal", "import", "from", "as"
};

// Java types and declarations
static const char* JAVA_TYPES[] = {
    "int", "long", "short", "byte", "float", "double", "boolean", "char",
    "String", "Object", "List", "Map", "Set", "ArrayList", "HashMap"
};

static const char* JAVA_PREFIXES[] = {
    "class", "interface", "enum", "abstract", "final", "static", "synchronized"
};

// Go types and declarations
static const char* GO_TYPES[] = {
    "int", "int8", "int16", "int32", "int64", "uint", "uint8", "uint16", "uint32", "uint64",
    "float32", "float64", "complex64", "complex128", "byte", "rune", "string", "bool"
};

static const char* GO_PREFIXES[] = {
    "var", "const", "func", "type", "struct", "interface", "map", "chan"
};

// PHP types and declarations
static const char* PHP_TYPES[] = {
    "int", "float", "string", "bool", "array", "object", "callable", "iterable",
    "mixed", "void", "null"
};

static const char* PHP_PREFIXES[] = {
    "class", "interface", "trait", "function", "const", "var", "public", "private", "protected"
};

// Ruby types and declarations
static const char* RUBY_TYPES[] = {
    "Integer", "Float", "String", "Array", "Hash", "Symbol", "TrueClass", "FalseClass",
    "NilClass", "Object", "Class", "Module"
};

static const char* RUBY_PREFIXES[] = {
    "def", "class", "module", "begin", "rescue", "ensure", "alias", "undef"
};

#endif // PATTERNS_H