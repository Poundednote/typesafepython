#ifndef PARSER_H_
#define PARSER_H_

#include <cstdint>
#include <stdint.h>
#include "tokeniser.h"


struct ASTNode;

enum class StaticType {
    INTEGER,
    DECIMAL,
    STRING,
};

enum class ASTNodeType {
    NARY,
    BINARY,
    UNARY,
    TERMINAL,
};

struct ASTNodeUnary {
    ASTNode *child;
};

struct ASTNodeNary {
    ASTNode *children;
};

struct ASTNodeBinary {
    ASTNode *left;
    ASTNode *right;
};

// ASTNode build into a tree structure each node has a pointer to its first child
// each child node has a next pointer to the next child of that level in the tree
struct ASTNode {
    Token *token = nullptr;
    ASTNodeType type = ASTNodeType::TERMINAL;
    StaticType static_type = StaticType::INTEGER;
    ASTNode *next = nullptr;
    ASTNode *children = nullptr;

    static inline ASTNode create_unary(Token *token, ASTNode *child);
    static inline ASTNode create_binary(Token *token, ASTNode *left, ASTNode *right);
    static inline ASTNode create_nary(Token *token, ASTNode *children);
};

struct ASTNodeArena {
    ASTNode *memory = nullptr;
    uint32_t nodes_max = 0;
    uint32_t offset = 0;

    inline ASTNodeArena(uint32_t nodes_max);
    inline ASTNode *node_alloc(ASTNode *node);
    ~ASTNodeArena() {delete[] this->memory;}
};

static ASTNode *parse_expression(TokenStream *token_stream, ASTNodeArena *arena, bool in_paren);
static std::string debug_static_type_to_string(StaticType type);
static void debug_print_parse_tree(ASTNode *node, std::string indent);
static void type_parse_tree(ASTNode *node);
#endif // PARSER_H_
