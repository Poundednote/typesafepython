#include <assert.h>
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <stdint.h>

#include "parser.h"
#include "tokeniser.h"

static std::string debug_static_type_to_string(StaticType type) {
    switch (type) {
        case StaticType::INTEGER:
            return "INTEGER";
        case StaticType::DECIMAL:
            return "DECIMAL";
        case StaticType::STRING:
            return "STRING";
    }

    return "";
}

inline ASTNodeArena::ASTNodeArena(uint32_t nodes_max) {
    this->nodes_max = nodes_max;
    this->memory = new ASTNode[nodes_max];
}

inline ASTNode *ASTNodeArena::node_alloc(ASTNode *node) {
    assert(this->offset < this->nodes_max);
    this->memory[this->offset] = *node;
    return &this->memory[this->offset++];
};

inline ASTNode ASTNode::create_unary(Token *token, ASTNode *child) {
    ASTNode node = {
        .token = token, .type = ASTNodeType::UNARY, .children = child};
    return node;
}

inline ASTNode ASTNode::create_binary(Token *token, ASTNode *left, ASTNode *right) {
    left->next = right;
    ASTNode node = {.token = token,
                    .type = ASTNodeType::BINARY,
                    .children = left};
    return node;
}

inline ASTNode ASTNode::create_nary(Token *token, ASTNode *children) {
    ASTNode node = {
        .token = token, .type = ASTNodeType::NARY, .children = children};
    return node;
}

ASTNode *parse_expression(TokenStream *token_stream, ASTNodeArena *arena, bool in_paren) {
    Token *current_token = token_stream->peek(0);
    Token *next_token = token_stream->next_token();

    if (current_token == nullptr) {
        std::cout << "UNEXPECTED END OF TOKEN STREAM" << std::endl;
        return nullptr;
    }
    ASTNode node = {.token = current_token};

    if (current_token->type == TokenType::COMMA ||
        current_token->type == TokenType::ENDFILE) {
        return arena->node_alloc(&node);
    }

    while (current_token->type == TokenType::NEWLINE && next_token) {
        current_token = token_stream->peek(0);
        next_token = token_stream->next_token();
    }


    if (next_token && next_token->is_binary_op()) {
        token_stream->next_token(); // skip a tokens ahead
        ASTNode *right = parse_expression(token_stream, arena, in_paren);

        if(!right) {
            std::cerr << "UNEXPECTED END OF TOKEN STREAM" << std::endl;
            return nullptr;
        }

        ASTNode binary_op_node = ASTNode::create_binary(next_token,
                                                        arena->node_alloc(&node), right);

        ASTNode *binary = arena->node_alloc(&binary_op_node);

        if (binary->token->precendence() > right->token->precendence()) {
            //shuffle around tree based on precedence
            ASTNode *new_root = binary->children->next;
            ASTNode *temp = new_root->children;
            new_root->children->next = binary->children->next->children->next;
            new_root->children = binary;
            binary->children->next = temp;

            assert(new_root->children->next->token->precendence() >= new_root->token->precendence());

            return new_root;
        }

        return binary;
    }

    if (current_token->is_atom()) {
        return arena->node_alloc(&node);
    }

    return nullptr;
}

static ASTNode *parse_statement(TokenStream *token_stream, ASTNodeArena *arena, bool in_paren) {
    Token *current_token = token_stream->peek(0);

    if (current_token->type == TokenType::ENDFILE) {
        return nullptr;
    }

    ASTNode node = {.token = current_token};

    switch (current_token->type) {
        case TokenType::COMMA: {
            return arena->node_alloc(&node);
        } break;

        case TokenType::ENDFILE: {
            return arena->node_alloc(&node);
        } break;

        case TokenType::RETURN: {
            token_stream->next_token();
            ASTNode *child = parse_expression(token_stream, arena, in_paren);
            ASTNode return_statement = ASTNode::create_unary(current_token, child);
            return arena->node_alloc(&return_statement);
        }

        case TokenType::RAISE: {
            token_stream->next_token();
            ASTNode *child = parse_expression(token_stream, arena, in_paren);
            ASTNode raise_statement = ASTNode::create_unary(current_token, child);

            return arena->node_alloc(&raise_statement);
        }

        case TokenType::GLOBAL: {
            Token *next_token = token_stream->next_token();
            if (next_token->type != TokenType::IDENTIFIER) {
                std::cout << "PARSE ERROR GLOBAL";
                exit(1);
            }

            ASTNode child = {.token = next_token};

            ASTNode global_statement = ASTNode::create_nary(current_token,
                                                            arena->node_alloc(&child));


            ASTNode *prev_child = nullptr;
            while (token_stream->peek(1)->type == TokenType::COMMA) {
                token_stream->next_token();
                ASTNode next_node = {.token = token_stream->next_token()};
                ASTNode *child = arena->node_alloc(&next_node);


                if (!prev_child) {
                    global_statement.children = child;
                }

                else {
                    prev_child->next = child;
                }

                prev_child = child;


            }

            return arena->node_alloc(&global_statement);
        }

        case TokenType::NONLOCAL: {
            Token *next_token = token_stream->next_token();
            if (next_token->type != TokenType::IDENTIFIER) {
                std::cout << "PARSE ERROR NONLOCAL";
                exit(1);
            }
            ASTNode child = {.token = next_token};

            ASTNode nonlocal_statement = ASTNode::create_nary(current_token,
                                                            arena->node_alloc(&child));

            ASTNode *prev_child = nullptr;
            while (token_stream->peek(1)->type == TokenType::COMMA) {
                token_stream->next_token();
                ASTNode node = {.token = token_stream->next_token()};
                ASTNode *child = arena->node_alloc(&node);

                if (!prev_child) {
                    nonlocal_statement.children = child;
                }

                else {
                    prev_child->next = child;
                }

                prev_child = child;
            }

            return arena->node_alloc(&nonlocal_statement);
        }

        default: {
            return parse_expression(token_stream, arena, in_paren);
        }
    }
}

static ASTNode *parse_statements(TokenStream *token_stream, ASTNodeArena *ast_arena, bool in_paren) {
    ASTNode file_node = ASTNode::create_nary(token_stream->peek(0), nullptr);
    ASTNode *prev_child = nullptr;
    token_stream->next_token();
    while (token_stream->peek(0)) {
        ASTNode *child = parse_statement(token_stream, ast_arena, false);
        if (!child && token_stream->peek(0)->type == TokenType::NEWLINE) {
            std::cout << "PARSING SUCCESFUL" << std::endl;
            break;
        }

        if (!(token_stream->peek(0)->type == TokenType::NEWLINE)) {
            std::cerr << ("parse ERROR expected newline token after end of expression");
        }

        if (!prev_child) {
            file_node.children = child;
        }

        else {
            prev_child->next = child;
        }

        prev_child = child;

        token_stream->next_token();
    }

    return ast_arena->node_alloc(&file_node);
}

static void debug_print_parse_tree(ASTNode *node, std::string indent) {
    ASTNode *child = node->children;
    switch (node->type) {
        case ASTNodeType::TERMINAL:
          std::cout << indent << "node: " << node->token->debug_to_string()
                    << ", type: "
                    << debug_static_type_to_string(node->static_type)
                    << ", value: " << node->token->value << "\n";
          break;

        default:
          std::cout << indent << "node: " << node->token->debug_to_string()
                    << ", type: "
                    << debug_static_type_to_string(node->static_type)
                    << ", value: " << node->token->value << "\n";
          indent += indent;
          while (child) {
            debug_print_parse_tree(child, indent);
            child = child->next;
          }
    }
}

void fail_typing_with_debug(ASTNode *node) {
    std::cerr << "Invalid Types\n";
    debug_print_parse_tree(node, "   ");
    exit(1);
}

static void type_parse_tree(ASTNode *node) {
    ASTNode *child = node->children;

    switch (node->type) {
    case ASTNodeType::TERMINAL:
        if (node->token->type == TokenType::INT_LIT) {
            node->static_type = StaticType::INTEGER;
        }

        if (node->token->type == TokenType::FLOAT_LIT) {
            node->static_type = StaticType::DECIMAL;
        }

        if (node->token->type == TokenType::STRING_LIT) {
            node->static_type = StaticType::STRING;
        }

        break;

    case ASTNodeType::UNARY:
        type_parse_tree(child);
        node->static_type = child->static_type;

    case ASTNodeType::BINARY:
        type_parse_tree(child);
        type_parse_tree(child->next);

        switch (node->token->type) {

        case TokenType::FLOOR_DIV:
            if (child->static_type != StaticType::INTEGER &&
                child->static_type != StaticType::DECIMAL) {
                fail_typing_with_debug(node);
            }

            else if (child->next->static_type != StaticType::INTEGER &&
                child->next->static_type != StaticType::DECIMAL) {
                fail_typing_with_debug(node);
            }

            node->static_type = StaticType::INTEGER;

        case TokenType::DIVISION:
            if (child->static_type != StaticType::INTEGER &&
                child->static_type != StaticType::DECIMAL) {
                fail_typing_with_debug(node);
            }

            else if (child->next->static_type != StaticType::INTEGER &&
                child->next->static_type != StaticType::DECIMAL) {
                fail_typing_with_debug(node);
            }

            node->static_type = StaticType::DECIMAL;

        default:
            if (child->static_type == child->next->static_type) {
                node->static_type = child->static_type;
            }

            else if (child->static_type == StaticType::INTEGER) {
                if (child->next->static_type != StaticType::DECIMAL) {
                    std::cerr << "Invalid Types\n";
                    fail_typing_with_debug(node);
                }

                else {
                    node->static_type = StaticType::DECIMAL;
                }
            }

            else if (child->static_type == StaticType::DECIMAL) {
                if (child->next->static_type != StaticType::INTEGER) {
                    fail_typing_with_debug(node);
                }

                else {
                    node->static_type = StaticType::DECIMAL;
                }
            }
        }

    case ASTNodeType::NARY:
        while (child) {
                type_parse_tree(child);
                child = child->next;
        }
        break;
    }
}
