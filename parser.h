#ifndef PARSER_H_
#define PARSER_H_

#include <cstdint>
#include <stdint.h>
#include "tokeniser.h"


struct ASTNode;

enum class ASTNodeType {
    FILE,
    STATEMENT,
    BINARY,
    TERMINAL,
};

struct ASTNodeBinary {
    ASTNode *left;
    ASTNode *right;
};

struct ASTNode {
    Token *token;
    uint32_t n_children = 0;
    ASTNode *children = nullptr;

    ASTNodeType type = ASTNodeType::TERMINAL;
    union {
        ASTNodeBinary binary;
    };
};

struct ASTNodeArena {
    ASTNode *memory = 0;
    uint32_t nodes_max = 0;
    uint32_t offset = 0;

    inline ASTNodeArena(uint32_t nodes_max);
    inline ASTNode *node_alloc(ASTNode *node);
    ~ASTNodeArena() {delete [] this->memory;}
};

inline ASTNode ast_node_binary_create(Token token, ASTNode *left, ASTNode *right);
ASTNode parse_expression(TokenStream *token_stream, ASTNodeArena *arena, bool in_paren);
#endif // PARSER_H_
