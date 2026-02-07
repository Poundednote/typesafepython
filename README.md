Here is a comprehensive architecture walkthrough of your codebase, designed for a `GitHub README` or technical documentation.

---

# System Architecture

This project is a high-performance, **custom C++ compiler frontend and static type checker for Python**. It implements a complete pipeline from source code to typed AST, featuring custom memory management, a recursive descent parser, descriptive error messages and, a distinct type inference engine capable of handling complex Python features like unions and generics.

## High-Level Pipeline

The system follows a classic compiler frontend architecture, optimized for speed using arena allocation and pointer-based structures.

1. **Input Stream**: Reads Python source files into a raw character buffer.
2. **Tokeniser**: Converts raw characters into a stream of `Token` structs, handling Python's whitespace-sensitive indentation logic.
3. **Parser**: A recursive descent parser that builds an Abstract Syntax Tree (AST).
4. **Symbol Resolution**: Populates a hash-based symbol table with scopes (functions, classes).
5. **Type Checker**: Traverses the AST to infer types, check compatibility, and resolve `Union` types.

---

## Core Components

### 1. Memory Management (Arena Allocation)

The project eschews standard C++ memory management (`new`/`delete` or smart pointers) in favor of **Arena Allocation** (Linear Allocators).

* **Design**: Large blocks of memory are pre-allocated. Objects (AST nodes, Tokens, TypeInfos) are allocated sequentially by bumping a pointer.
* **Benefits**:
* **Zero Overhead**: Allocation is a simple pointer increment.
* **Cache Locality**: Related objects (like AST nodes created during parsing) are contiguous in memory.
* **Instant Cleanup**: The entire memory state can be wiped by resetting the arena offset, avoiding expensive destructors.


* **Usage**: See `Arena` usage in `main.cpp` passed down to `Parser` and `Tables`.

### 2. The Tokeniser (`tokeniser.cpp`)

The lexer transforms the character stream into tokens. It explicitly handles Python's unique **Off-side Rule** (indentation blocks).

* **Indentation Stack**: Tracks indentation levels to inject `INDENT` and `DEDENT` tokens synthetically, allowing the parser to treat blocks like C-style curly brace blocks.
* **String Parsing**: Supports complex Python string literals, including f-strings, raw strings (`r""`), and byte strings (`b""`).
* **Lookahead**: The `TokenArray` structure maintains `current` and `lookahead` tokens to simplify parsing logic.

### 3. The Parser (`parser.cpp`)

A **Recursive Descent Parser** that constructs the AST.

* **AST Structure**:
* **Union-Based Nodes**: `AstNode` uses a large `union` to store data for specific node types (`AstNodeBinaryExpr`, `AstNodeIf`, etc.), keeping the struct flat and memory-efficient.
* **Linked-List Siblings**: Rather than `std::vector<AstNode*>`, children are stored as a linked list via the `adjacent_child` pointer. This reduces heap fragmentation and plays well with the Arena allocator.


* **Precedence Handling**: Uses a hybrid approach (Precedence Climbing) to handle binary expression operator precedence correctly (e.g., `*` binds tighter than `+`).
* **Error Recovery**: Implements `ParseResult` to propagate errors up the stack, with "panic mode" recovery (`token_array_goto_end_of_statement`) to continue finding errors after the first failure.

### 4. The Type Checker (`typing.cpp`)

The core analysis engine. It performs a **static pass** over the AST to assign `TypeInfo` to every node.

* **Type Representation**: Types are defined in `TypeInfo` structs. It supports:
* **Primitives**: `INTEGER`, `FLOAT`, `STRING`, `BOOLEAN`.
* **Complex Types**: `CLASS`, `FUNCTION`.
* **Generics**: `LIST`, `DICT` (parameterized types).
* **Algebraic Types**: `UNION` (e.g., `int | str`).


* **Inference Logic**:
* **Shadow Execution**: The type checker "runs" the code abstractly. When it encounters `a = 10`, it updates `a` in the symbol table to `INTEGER`.
* **Union Generation**: If a list contains mixed types `[1, "hello"]`, the engine iteratively builds a `UNION` type (`Union[int, str]`) via `generate_union_and_update_type_to_unionise`.
* **Compatibility**: `static_types_is_rhs_equal_lhs` validates assignments, ensuring `int` cannot be assigned to `str`, while handling inheritance and interface implementation checks.



### 5. Symbol Table & Scoping (`tables.cpp`)

A custom hash-table implementation that manages variable visibility.

* **Scoped Lookup**: The table supports hierarchical lookups. If a symbol isn't found in the local scope, it bubbles up to enclosing scopes (e.g., searching for a variable inside a function, then the class, then global).
* **Scope Stack**: During parsing and type checking, a `scope_stack` tracks the current context (Global -> Class -> Method).
* **Python C API Integration**: The `main.cpp` utilizes `Python.h` to verify and bootstrap built-in types (like `int`, `str`, `list`) against the actual Python interpreter's definitions, ensuring compatibility.

---

## Code Walkthrough Example

Here is how the system processes a simple line of code: `x: int = 10 + 20`

1. **Tokenisation**:
* Input: `x: int = 10 + 20`
* Output: `[IDENTIFIER("x"), COLON, IDENTIFIER("int"), ASSIGN, INT_LIT("10"), ADDITION, INT_LIT("20")]`


2. **Parsing (`parse_declaration`)**:
* Identifies `x` as the declaration name.
* Parses `int` as the `annotation`.
* Parses `10 + 20` as a `BINARY_EXPR` (right-hand side).
* Allocates an `AstNodeDeclaration` in the AST Arena.


3. **Symbol Table**:
* Inserts `x` into the current scope with the type defined by the annotation (`INTEGER`).


4. **Type Checking**:
* Evaluates `10` -> `INTEGER`.
* Evaluates `20` -> `INTEGER`.
* Evaluates `+` -> Checks if `INTEGER` + `INTEGER` is valid -> Result `INTEGER`.
* **Verification**: Checks if the RHS type (`INTEGER`) matches the annotation `x` (`INTEGER`).
* Result: Success

# Things I have learnt since then
* Instead of using pointers to reference things in the AST and TypeInfo use 32bit indices this this reduces the size of the structures and also has the added benifit of not being invalidated if you need to reallocate/resize the memory for whatever reason.
* SoA layout might also be benificial for type information and AST Nodes
