#include <assert.h>
#include <cstdint>
#include <stdint.h>

#include "parser.h"
#include "tokeniser.h"

inline ASTNodeArena::ASTNodeArena(uint32_t nodes_max) {
    this->nodes_max = nodes_max;
    this->memory = new ASTNode[nodes_max];
}

inline ASTNode *ASTNodeArena::node_alloc(ASTNode *node) {
    assert(this->offset < this->nodes_max);
    this->memory[this->offset] = *node;
    return &this->memory[this->offset++];
};

inline ASTNode ast_node_binary_create(Token *token, ASTNode *left, ASTNode *right) {
    ASTNodeBinary binary_node = {.left = left, .right = right};
    ASTNode node = {.token = token,
                    .n_children = 2,
                    .children = binary_node.left,
                    .type = ASTNodeType::BINARY,
                    .binary = binary_node};
    return node;
}

ASTNode *parse_expression(TokenStream *token_stream, ASTNodeArena *arena, bool in_paren) {
    Token *current_token = token_stream->peek(0);
    Token *next_token = token_stream->next_token();
    ASTNode node = {.token = current_token};

    if (current_token->type == TokenType::COMMA) {
        return arena->node_alloc(&node);
    }

    if (current_token->type == TokenType::NEWLINE) {
        return arena->node_alloc(&node);
    }

    if (current_token->type == TokenType::ENDFILE) {
        return arena->node_alloc(&node);
    }

    if (next_token->is_binary_op()) {
        token_stream->next_token(); // skip to next token for parsing;
        ASTNode *right = parse_expression(token_stream, arena, in_paren);

        ASTNode binary_op_node = ast_node_binary_create(
            next_token, arena->node_alloc(&node), right);

        ASTNode *binary = arena->node_alloc(&binary_op_node);

        if (binary->token->precendence() > right->token->precendence()) {
            //shuffle around tree based on precedence
            ASTNode *new_root = binary->binary.right;
            ASTNode *temp = new_root->binary.left;
            new_root->binary.right = binary->binary.right->binary.right;
            new_root->binary.left = binary;
            binary->binary.right = temp;

            return new_root;
        }

        return binary;
    }

    switch (current_token->type) {
        case TokenType::IDENTIFIER: {
            node.type = ASTNodeType::TERMINAL;
            return arena->node_alloc(&node);
            break;
        }

        case TokenType::NONE: {
            node.type = ASTNodeType::TERMINAL;
            return arena->node_alloc(&node);
            break;
        }
        case TokenType::TRUE: {
            node.type = ASTNodeType::TERMINAL;
            return arena->node_alloc(&node);
            break;
        }
        case TokenType::FALSE: {
            node.type = ASTNodeType::TERMINAL;
            return arena->node_alloc(&node);
            break;
        }

        case TokenType::INT_LIT: {
            node.type = ASTNodeType::TERMINAL;
            return arena->node_alloc(&node);
            break;
        }

        case TokenType::STRING_LIT: {
            node.type = ASTNodeType::TERMINAL;
            return arena->node_alloc(&node);
            break;
        }
    }

    token_stream->next_token();
    return parse_expression(token_stream, arena, in_paren);
}
