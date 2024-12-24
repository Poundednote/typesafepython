#include <cstdint>
#include <assert.h>
#include "parser.h"
#include "tokeniser.h"
#include "typing.h"
#include "tables.h"
#include "debug.h"
#include <stdint.h>

// NOTE: THIS needs to be read only do not ever modify it ever 
static AstNode ERR_NODE = {.type = AstNodeType::INVALID};

// TODO: FIGURE OUT T_PRIMARY PARSING
//
//
//
// TODO: fix semicolons and newlines for blocks
// and make it so that when invalid syntax occurs move to the next newline or semicolon
// TODO: MATCH STATMENTS
// TODO: slash arguments
// TODO: negative numbers unary operator
// TODO: YIELD STATMENTS
// TODO: ASSERT STATMENTS
// TODO: Augmented assign e.g. += -= stuff
// TODO: Test parenthesis and atom parsing
// TODO: enforce declarations to assign= Baby()
// TODO: allow , at the end of sequences with no next item
//
//NOTE: python grammar reference
//https://docs.python.org/3/reference/grammar.html
//
static AstNode *node_alloc(Arena *ast_arena)
{
        AstNode *allocated_node =
                (AstNode *)ast_arena->alloc(sizeof(AstNode));
        // call constructor because AstNode contains std::string .....
        *allocated_node = AstNode();

        return allocated_node;
}

static ParseResult parser_create_error_from_msg(Parser *parser, 
                                                const char *message)
{
        ParseError error = {};
        error.type = ParseErrorType::GENERAL;
        error.token = &parser->token_arr->tokens[parser->token_arr->position];
        error.msg = message;

        ParseResult result = {};
        result.node = node_alloc(parser->ast_arena);
        *result.node = ERR_NODE;
        result.error = error;

        return result;
}

static SymbolTableEntry *assert_no_redefinition_and_insert_to_symbol_table(
        Arena *table_arena, SymbolTable *table, SymbolTableEntry *scope,
        Token *symbol, SymbolTableValue *value, bool func)
{
        SymbolTableEntry *entry = table->lookup(symbol->value, scope);
        if (entry) {
#if NOREDEF
                fprintf_s(
                        stderr,
                        "Syntax Error: line: %d, col: %d redefinition of '%s'",
                        symbol->line, symbol->column, symbol->value.c_str());
                exit(1);
#else
                entry->value = *value;
                return entry;
#endif
        }

        if (!func)
                return table->insert(table_arena, symbol->value, scope, value);

        return table->insert_function(table_arena, symbol->value, scope, value);
}

static AstNode *parse_single_token_into_node(Parser *parser)
{
        AstNode *node = node_alloc(parser->ast_arena);
        node->token = parser->token_arr->current;
        parser->token_arr->next_token();
        return node;
}

inline AstNode AstNode::create_unary(Token token, AstNode *child)
{
        AstNode node = {};
        node.token = token;
        node.type = AstNodeType::UNARY;
        node.unary.child = child;

        return node;
}

inline AstNode AstNode::create_binary(Token token, AstNode *left,
                                      AstNode *right)
{
        AstNode node = {};
        AstNodeBinaryExpr *binary = &node.binary;
        node.token = token;
        node.type = AstNodeType::BINARYEXPR;
        binary->left = left;
        binary->right = right;

        return node;
}

static ParseResult assert_token_and_print_debug(Parser *parser,
                                                enum TokenType type,
                                                const char *message)
{
        Token token = parser->token_arr->current;
        if (token.type != type) {
                char buffer[1024] = {};
                snprintf(buffer, sizeof(buffer), "%s Expected Token '%s' instead of '%s'",
                         message,
                         TOKEN_STRINGS[(int)type],
                         TOKEN_STRINGS[(int)parser->token_arr->current.type]);

                return parser_create_error_from_msg(parser, buffer);
                
        }

        return ParseResult{};
}

static AstNode *wrap_in_tuple(Parser *parser,
                              AstNode *head)
{
        AstNode *node = node_alloc(parser->ast_arena);
        node->type = AstNodeType::TUPLE;
        node->token = parser->token_arr->current;
        node->tuple.children = head;
        return node;
}

static ParseResult parse_single_target(Parser *parser)
{
        if (parser->token_arr->current.type == TokenType::OPEN_PAREN) {
                ParseResult result = parse_single_target(parser);

                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::CLOSED_PAREN,
                        "Mistmatched parenthesis");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;

                return result;
        }

        return parse_single_subscript_attribute_primary(parser);
}

static ParseResult 
parse_comma_seperated_func(Parser *parser, ParseSingleFunc func)
{
        ParseResult result =
                func(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *head = result.node;

        AstNode **child = &head->adjacent_child;

        while (parser->token_arr->current.type == TokenType::COMMA) {
                parser->token_arr->next_token();
                result = func(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                *child = result.node;
                child = &((*child)->adjacent_child);
        }

        return ParseResult{.node = head};
}

static ParseResult parse_comma_seperated_in_tuple_func(Parser *parser, ParseSingleFunc func)
{
        ParseResult result =
                func(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *head = result.node;

        if (parser->token_arr->current.type != TokenType::COMMA) {
                return ParseResult{.node = head};
        }

        AstNode *tuple = wrap_in_tuple(parser, head);
        AstNode **child = &tuple->tuple.children->adjacent_child;

        while (parser->token_arr->current.type == TokenType::COMMA) {
                parser->token_arr->next_token();
                result = func(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                *child = result.node;
                child = &((*child)->adjacent_child);
        }

        return ParseResult{.node = tuple};
}

static ParseResult parse_name(Parser *parser, bool add_to_symbol_table)
{
        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::IDENTIFIER, "Failed to parse name");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;

        AstNode *name = node_alloc(parser->ast_arena);
        name->type = AstNodeType::IDENTIFIER;
        name->token = parser->token_arr->current;
        parser->token_arr->next_token();
        return ParseResult{.node = name};
}

static ParseResult parse_list_of_names_and_return_modifier(Parser *parser,
                                                           Token token)
{
        AstNode *parent = node_alloc(parser->ast_arena);
        parent->type = AstNodeType::NARY;
        parent->token = token;
        ParseResult result =
                parse_name(parser, false); 

        if (result.error.type != ParseErrorType::NONE) {
                return result;
        }

        AstNode *current_child = result.node;

        AstNode *head_child = current_child;
        while (parser->token_arr->current.type == TokenType::COMMA) {
                parser->token_arr->next_token();
                result = parse_name(parser, false);

                if (result.error.type != ParseErrorType::NONE) {
                        return result;
                }
                
                AstNode *name = result.node;

                current_child->adjacent_child = name;
                current_child = current_child->adjacent_child;
        }

        parent->nary.children = head_child;
        return ParseResult{.node = parent};
}

static ParseResult parse_type_expression(Parser *parser)
{
        AstNode *node = node_alloc(parser->ast_arena);
        Token current_token = parser->token_arr->current;
        Token next_token = parser->token_arr->lookahead;

        node->type = AstNodeType::TYPE_ANNOTATION;
        node->token = current_token;
        ParseResult result = parse_name(parser, false);

        if (result.error.type != ParseErrorType::NONE) {
                return result;
        }

        node->type_annotation.type = result.node;

        if (next_token.type == TokenType::SQUARE_OPEN_PAREN) {
                parser->token_arr->next_token();
                AstNode **child = &node->type_annotation.parameters;
                while (parser->token_arr->current.type !=
                       TokenType::SQUARE_CLOSED_PAREN) {

                        result = parse_type_annotation(parser);

                        if (result.error.type != ParseErrorType::NONE) {
                                return result;
                        }

                        *child = result.node;

                        if (parser->token_arr->current.type ==
                            TokenType::SQUARE_CLOSED_PAREN) {
                                break;
                        }

                        child = &((*child)->adjacent_child);

                        assert_comma_and_skip_over(parser);
                }

                parser->token_arr->next_token();
        }

        return ParseResult{.node = node};
}

static ParseResult parse_type_annotation(Parser *parser)
{
        ParseResult result = parse_type_expression(parser);
        if (result.error.type != ParseErrorType::NONE) {
                return result;
        }

        AstNode *left = result.node;

        if (parser->token_arr->current.type == TokenType::BWOR) {
                AstNode *union_type = node_alloc(parser->ast_arena);
                union_type->type = AstNodeType::UNION;
                union_type->token = parser->token_arr->current;
                union_type->union_type.left = left;
                parser->token_arr->next_token();
                result = parse_type_annotation(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                union_type->union_type.right = result.node;

                return ParseResult{.node = union_type};
        } else {
                return ParseResult{.node = left};
        }
}

static ParseResult 
parse_target_with_star_atom(Parser *parser, bool add_to_symbol_table)
{
        if (parser->token_arr->current.type == TokenType::OPEN_PAREN) {
                parser->token_arr->next_token();


                if (parser->token_arr->current.type == TokenType::MULTIPLICATION) {
                        parser->token_arr->next_token();

                        AstNode *tuple = node_alloc(parser->ast_arena);
                        tuple->type = AstNodeType::TUPLE;

                        ParseResult result = parse_single_star_target(parser);

                        if (result.error.type != ParseErrorType::NONE) {
                                return result;
                        }

                        AstNode *head = result.node;
                        tuple->tuple.children = head;

                        AstNode **child = &head->adjacent_child;

                        if (parser->token_arr->current.type != TokenType::COMMA) {
                                return parser_create_error_from_msg(parser,
                                        "Cannot use starred expression here");
                        }

                        while (parser->token_arr->current.type ==
                               TokenType::COMMA) {
                                parser->token_arr->next_token();

                                if (parser->token_arr->current.type ==
                                    TokenType::CLOSED_PAREN)
                                        break;

                                result = parse_single_star_target(parser, add_to_symbol_table);

                                if (result.error.type != ParseErrorType::NONE) {
                                        return result;
                                }

                                *child = result.node;
                                child = &((*child)->adjacent_child);
                        }

                        ParseResult assert_result =
                                assert_token_and_print_debug(
                                        parser,
                                        TokenType::CLOSED_PAREN,
                                        "Mismatched Parenthesis");

                        if (assert_result.error.type != ParseErrorType::NONE)
                                return assert_result;

                        parser->token_arr->next_token();
                        return ParseResult{.node = tuple};

                } else {
                        ParseResult result = parse_target_with_star_atom(
                                parser, add_to_symbol_table);

                        if (result.error.type != ParseErrorType::NONE) 
                                return result;

                        AstNode *head = result.node;
                        AstNode **child = &head->adjacent_child;

                        if (parser->token_arr->current.type != TokenType::COMMA)
                                return ParseResult{.node = head};


                        AstNode *tuple = node_alloc(parser->ast_arena);
                        tuple->type = AstNodeType::TUPLE;

                        while (parser->token_arr->current.type ==
                               TokenType::COMMA) {
                                parser->token_arr->next_token();

                                if (parser->token_arr->current.type ==
                                    TokenType::CLOSED_PAREN)
                                        break;

                                result = parse_single_star_target(
                                        parser, add_to_symbol_table);

                                if (result.error.type != ParseErrorType::NONE)
                                        return result;

                                *child = result.node;
                                child = &((*child)->adjacent_child);
                        }

                        ParseResult assert_result =
                                assert_token_and_print_debug(
                                        parser,
                                        TokenType::CLOSED_PAREN,
                                        "Mismatched Parenthesis");

                        if (assert_result.error.type != ParseErrorType::NONE)
                                return assert_result;
                        parser->token_arr->next_token();
                        return ParseResult{.node = tuple};
                }

        } else if (parser->token_arr->current.type == TokenType::SQUARE_OPEN_PAREN) {
                AstNode *list = node_alloc(parser->ast_arena);
                list->type = AstNodeType::LIST;
                list->token = parser->token_arr->current;

                parser->token_arr->next_token();

                ParseResult result = parse_single_star_target(
                        parser, add_to_symbol_table);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *head = result.node;
                AstNode **child = &head->adjacent_child;

                list->list.children = head;

                while (parser->token_arr->current.type == TokenType::COMMA) {
                        parser->token_arr->next_token();

                        if (parser->token_arr->current.type ==
                            TokenType::SQUARE_CLOSED_PAREN)
                                break;

                        result = parse_single_star_target(
                                parser, add_to_symbol_table);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        *child = result.node;
                        child = &((*child)->adjacent_child);
                }

                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::SQUARE_CLOSED_PAREN,
                        "Mismatched Parenthesis");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;

                parser->token_arr->next_token();
                return ParseResult{.node = list};
        } else {
                ParseResult result = parse_primary(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *star_atom = result.node;
                // only accept the attribute references subscripts or identifiers
                bool should_accept = false;
                should_accept = should_accept ||
                                star_atom->type == AstNodeType::ATTRIBUTE_REF;
                should_accept = should_accept ||
                                star_atom->type == AstNodeType::SUBSCRIPT;
                should_accept = should_accept ||
                                star_atom->type == AstNodeType::IDENTIFIER;
                if (!should_accept) {
                        return parser_create_error_from_msg(
                                parser, "Cannot assign here");
                }

                return ParseResult{.node = star_atom};
        }
}

static ParseResult parse_single_star_target(Parser *parser,
                                            bool add_to_symbol_table)
{
        // TODO: Test  left reccursion on star_targets
        if (parser->token_arr->current.type == TokenType::MULTIPLICATION) {
                AstNode *starred_target = node_alloc(parser->ast_arena);
                starred_target->type = AstNodeType::STARRED;
                starred_target->token = parser->token_arr->current;
                parser->token_arr->next_token();

                ParseResult result = parse_target_with_star_atom(parser, add_to_symbol_table);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                starred_target->star_expression.expression = result.node;
                return ParseResult{ .node = starred_target };

        } else if (parser->token_arr->current.type == TokenType::EXPONENTIATION) {
                return parser_create_error_from_msg(
                        parser, "Star targets cannot be double starred");
        }

        return parse_target_with_star_atom(parser, add_to_symbol_table);
}

static ParseResult parse_star_targets(Parser *parser)
{
        //TODO implement parsing star_atom
        ParseResult result = parse_single_star_target(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *head = result.node;

        AstNode **child = &head->adjacent_child;
        while (parser->token_arr->current.type == TokenType::COMMA) {
                result = parse_single_star_target(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                *child = result.node;

                child = &((*child)->adjacent_child);
        }

        return ParseResult{.node = head};
}

static ParseResult parse_function_call_arguments(Parser *parser)
{
        AstNode *args_head = nullptr;
        AstNode **arg = &args_head;
        while (parser->token_arr->current.type != TokenType::CLOSED_PAREN) {
                if (parser->token_arr->current.type == TokenType::NEWLINE) {
                        printf("Expected Token ')' before newline\n");
                        exit(1);
                }

                if (parser->token_arr->lookahead.type == TokenType::ASSIGN) {
                        // check to see if the token before '=' is an identifier
                        ParseResult assert_result = assert_token_and_print_debug(
                                parser, TokenType::IDENTIFIER,
                                "Keyword arguments must be valid identifiers\n");

                        if (assert_result.error.type != ParseErrorType::NONE)
                                return assert_result;

                        AstNode *kwarg = node_alloc(parser->ast_arena);
                        kwarg->token = parser->token_arr->current;
                        // FIXME add to symbol table
                        ParseResult result = parse_name(parser, true);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        kwarg->kwarg.name = result.node;
                        parser->token_arr->next_token();

                        result = parse_expression(parser, 0);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        kwarg->kwarg.expression = result.node;

                        if (!(kwarg->kwarg.expression)) {
                                printf("Failed to parse function argument on line: %d col: %d\n",
                                       parser->token_arr->current.line,
                                       parser->token_arr->current.column);
                                break;
                        }

                        *arg = kwarg;
                }

                else {
                        ParseResult result = 
                                parse_single_assignment_star_expression(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        *arg = result.node;

                        result = parse_gen_expr_from_first_child(parser, *arg);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        AstNode *maybe_gen = result.node;

                        if (maybe_gen != *arg) {
                                *arg = maybe_gen;
                                ParseResult assert_result = assert_token_and_print_debug(
                                        parser,
                                        TokenType::CLOSED_PAREN,
                                        "Gen expression without parenthises is only valid for functions with 1 arguemnt");

                                if (assert_result.error.type != ParseErrorType::NONE)
                                        return assert_result;
                                break;
                        }

                        if (!(*arg)) {
                                printf("Failed to parse function argument on line: %d col: %d\n",
                                       parser->token_arr->current.line,
                                       parser->token_arr->current.column);

                                break;
                        }
                }

                // dont check for comma if the adjacent_child token is a closing parenthesis
                if (parser->token_arr->current.type == TokenType::CLOSED_PAREN) {
                        break;
                }

                assert_comma_and_skip_over(parser);
                arg = &((*arg)->adjacent_child);
        }

        parser->token_arr->next_token();
        return ParseResult{.node = args_head};
}

// NOTE: Python distinguishes between regular primary and those that end
// in just a list slice a and attribute access
// See 'single_subscript_attribute_target'
static ParseResult parse_single_subscript_attribute_primary(
        Parser *parser, bool add_to_symbol_table)
{
        AstNode *prev = nullptr;
        AstNode node = {};
        node.token = parser->token_arr->current;

        ParseResult result = parse_atom(parser, add_to_symbol_table);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *left = result.node;

        Token prev_tok = parser->token_arr->current;
        while (left != prev) {
                prev = left;
                result = parse_sub_primary(parser, left, add_to_symbol_table);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                left = result.node;
        }

        if (left->type != AstNodeType::SUBSCRIPT && left->type == AstNodeType::ATTRIBUTE_REF) {
                return parser_create_error_from_msg(
                        parser,
                        "Expected either attribute target or subscript");
        }

        return ParseResult{.node = left};
}

static ParseResult parse_targets(Parser *parser)
{
        return parse_comma_seperated_in_tuple_func(parser, parse_single_target);
}

static ParseResult parse_single_del_target(Parser *parser)
{
        return parse_single_subscript_attribute_primary(parser);
}

static ParseResult parse_del_targets(Parser *parser)
{
        return parse_comma_seperated_in_tuple_func(parser, parse_single_del_target);
}

static ParseResult parse_slice(Parser *parser)
{
        AstNode *slice = node_alloc(parser->ast_arena);
        slice->type = AstNodeType::SLICE;
        slice->token = parser->token_arr->current;

        AstNode *maybe_assignment_expr = nullptr;
        if (parser->token_arr->current.type != TokenType::COLON) {
                ParseResult result = 
                        parse_single_assignment_expression(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                maybe_assignment_expr = result.node;

                if (maybe_assignment_expr->type == AstNodeType::ASSIGNMENT ||
                    parser->token_arr->current.type != TokenType::COLON) {
                        slice->slice.named_expr = maybe_assignment_expr;
                        return ParseResult{.node = slice};
                }
        }

        slice->slice.start = maybe_assignment_expr;
        AstNode **child = &slice->slice.end;
        int i = 0;
        while (parser->token_arr->current.type == TokenType::COLON || i == 2) {
                parser->token_arr->next_token();
                if (parser->token_arr->current.type == TokenType::NEWLINE) {
                        printf("Expected Token ']' before newline\n");
                        exit(1);
                }

                ParseResult result = parse_expression(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                *child = result.node;
                child = ++child;
                ++i;
        }

        if (parser->token_arr->current.type == TokenType::COLON) {
                return parser_create_error_from_msg(
                        parser,
                        "Unexpected ':' at the end of slice expression");
        }

        return ParseResult{ .node = slice };
}

static ParseResult parse_primary(Parser *parser)
{
        AstNode *prev = nullptr;
        ParseResult result = parse_atom(parser, false);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *left = result.node;

        // parse star primary
        // NOTE: What the fuck does this even mean

        while (left != prev) {
                prev = left;
                result = parse_sub_primary(parser, left, false);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                left = result.node;
        }

        return ParseResult{.node = left};
}

static ParseResult parse_sub_primary(Parser *parser, AstNode *left, bool add_to_symbol_table)
{
        Token current_token = parser->token_arr->current;

        if (current_token.type == TokenType::DOT) {
                AstNode *attribute_ref = node_alloc(parser->ast_arena);
                attribute_ref->type = AstNodeType::ATTRIBUTE_REF;
                attribute_ref->token = parser->token_arr->current;
                parser->token_arr->next_token();
                ParseResult result =
                        parse_name(parser, false);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                attribute_ref->attribute_ref.attribute = result.node;
                attribute_ref->attribute_ref.name = left;

                return ParseResult{.node = attribute_ref};
        } else if (current_token.type == TokenType::OPEN_PAREN) {
                parser->token_arr->next_token();

                AstNode *call = node_alloc(parser->ast_arena);
                call->type = AstNodeType::FUNCTION_CALL;
                call->token = current_token;

                ParseResult result = parse_function_call_arguments(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                call->function_call.args = result.node;
                call->function_call.expression = left;

                return parse_sub_primary(parser, call, false);

        } else if (current_token.type == TokenType::SQUARE_OPEN_PAREN) {
                parser->token_arr->next_token();

                AstNode *subscript = node_alloc(parser->ast_arena);
                subscript->token = current_token,
                subscript->type = AstNodeType::SUBSCRIPT;

                AstNodeSubscript *subscript_proper = &subscript->subscript;
                subscript_proper->expression = left;
                ParseResult result = parse_comma_seperated_in_tuple_func(parser, parse_slice);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                subscript_proper->slices = result.node;

                ParseResult assert_result = assert_token_and_print_debug(
                        parser,
                        TokenType::SQUARE_CLOSED_PAREN,
                        "Error parsing subscript");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;
                parser->token_arr->next_token();

                return parse_sub_primary(parser, subscript, add_to_symbol_table);
        } else {
                return ParseResult{.node = left};
        }
}

static ParseResult parse_else(Parser *parser)
{
        Token current_token = parser->token_arr->current;

        if (current_token.type != TokenType::ELSE) {
                return ParseResult{.node = nullptr};
        }

        AstNode *else_node = node_alloc(parser->ast_arena);
        else_node->token = current_token;
        else_node->type = AstNodeType::ELSE;
        AstNodeElse *else_node_proper = &else_node->else_stmt;
        parser->token_arr->next_token();
        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::COLON,
                "\n Error Parsing else satement");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;

        parser->token_arr->next_token(); // skip the colon

        ParseResult result = parse_block(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        else_node_proper->block = result.node;
        return ParseResult{.node = else_node};
}

static ParseResult parse_elif(Parser *parser)
{
        Token current_token = parser->token_arr->current;

        if (current_token.type != TokenType::ELIF) {
                return parse_else(parser);
        }

        parser->token_arr->next_token();
        AstNode *elif = node_alloc(parser->ast_arena);
        elif->token = current_token;
        AstNodeIf *elif_proper = &elif->if_stmt;

        ParseResult result = parse_expression(parser, 0);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        elif_proper->condition = result.node;
        ParseResult assert_result = assert_token_and_print_debug(
                 parser, TokenType::COLON,
                "\n Error Parsing elif satement");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;
        parser->token_arr->next_token(); // skip the colon
        result = parse_block(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        elif_proper->block = result.node;
        result = parse_elif(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        elif_proper->or_else = result.node;

        return ParseResult{.node = elif};
}

static ParseResult assert_comma_and_skip_over(Parser *parser)
{
        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::COMMA,
                "parameters must be seperated by commas");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;

        parser->token_arr->next_token();
        return ParseResult{};
}

static ParseResult 
parse_function_default_assign(Parser *parser, AstNode *left)
{
        if (parser->token_arr->current.type != TokenType::ASSIGN) {
                return ParseResult{.node = left};
        }

        AstNode *node = node_alloc(parser->ast_arena);
        node->type = AstNodeType::ASSIGNMENT;
        AstNodeAssignment *assignment = &node->assignment;
        assignment->left = left;

        if (!parser->tables->symbol_table->lookup(parser->token_arr->current.value,
                                          parser->scope)) {
                SymbolTableValue value = {};
                value.node = node;
                value.static_type.type = TypeInfoType::ANY;
                parser->tables->symbol_table->insert(parser->symbol_table_arena,
                                                     left->token.value, parser->scope, &value);
        }

        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::ASSIGN,
                "\nError in parsing assignment on line: ");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;
        parser->token_arr->next_token();

        ParseResult result = parse_expression(parser, 0);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        assignment->expression = result.node;

        return ParseResult{.node = node};
}

static ParseResult parse_function_default_assignment_or_annotation(Parser *parser, AstNode *left)
{
        ParseResult result = parse_function_default_assign(parser, left);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *maybe_assign = result.node;

        if (maybe_assign != left) {
                return ParseResult{.node = maybe_assign};
        }

        if (parser->token_arr->current.type == TokenType::COLON) {
                AstNode *node = node_alloc(parser->ast_arena);
                node->token = parser->token_arr->current;
                node->type = AstNodeType::DECLARATION;
                AstNodeDeclaration *declaration = &node->declaration;
                declaration->name = left;

                SymbolTableValue value = {};
                value.node = node;
                assert_no_redefinition_and_insert_to_symbol_table(
                        parser->symbol_table_arena, parser->tables->symbol_table, parser->scope,
                        &declaration->name->token, &value, false);

                parser->token_arr->next_token();
                result =
                        parse_type_annotation(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                declaration->annotation = result.node;

                if (parser->token_arr->current.type == TokenType::ASSIGN) {
                        parser->token_arr->next_token();
                        result =
                                parse_expression(parser, 0);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        declaration->expression = result.node;
                }
                return ParseResult{.node = node};

        } else {
                SymbolTableValue value = {};
                value.node = left;
                value.static_type.type = TypeInfoType::ANY;
                assert_no_redefinition_and_insert_to_symbol_table(
                        parser->symbol_table_arena, parser->tables->symbol_table, parser->scope,
                        &left->token, &value, false);
                return ParseResult{.node = left};
        }
}

static bool is_default_arg(AstNode *arg)
{
        if (arg->type == AstNodeType::ASSIGNMENT)
                return true;

        if (arg->type == AstNodeType::DECLARATION &&
            arg->declaration.expression)
                return true;

        return false;
}

static ParseResult parse_function_def_arguments(Parser *parser, AstNodeFunctionDef *function)
{
        AstNode *arg_head = nullptr;
        AstNode **arg = &arg_head;
        int arg_position = 0;
        bool defaults_only = false;

        while (parser->token_arr->current.type != TokenType::CLOSED_PAREN) {
                Token current_token = parser->token_arr->current;
                AstNode argument_node = {};
                argument_node.token = current_token;

                if (current_token.type == TokenType::DIVISION) {
                        parser->token_arr->next_token();
                        function->slash_pos = arg_position++;

                        if (parser->token_arr->current.type ==
                            TokenType::CLOSED_PAREN)
                                break;

                        assert_comma_and_skip_over(parser);
                        continue;
                } else if (current_token.type == TokenType::MULTIPLICATION) {
                        parser->token_arr->next_token();
                        if (function->star) {
                                return parser_create_error_from_msg(
                                        parser,
                                        "Function can only contain one * argument");
                        }

                        function->star_pos = arg_position++;
                        if (parser->token_arr->current.type ==
                            TokenType::CLOSED_PAREN)
                                break;
                        if (parser->token_arr->current.type != TokenType::COMMA) {
                                ParseResult result =
                                        parse_name(parser);

                                if (result.error.type != ParseErrorType::NONE)
                                        return result;

                                function->star = result.node;
                        }

                        if (parser->token_arr->current.type ==
                            TokenType::CLOSED_PAREN)
                                break;

                        assert_comma_and_skip_over(parser);
                        continue;
                } else if (parser->token_arr->current.type ==
                           TokenType::EXPONENTIATION) {
                        parser->token_arr->next_token();

                        ParseResult result = parse_name(parser);
                        if (result.error.type != ParseErrorType::NONE) {
                                return result;
                        }

                        AstNode *name = result.node;
                        result = parse_assignment_or_declaration(parser, name);

                        if (result.error.type != ParseErrorType::NONE) {
                                return result;
                        }

                        function->double_star = result.node;

                        ParseResult assert_result = assert_token_and_print_debug(
                                parser, TokenType::CLOSED_PAREN,

                                "** arguments cannot be followed by any additional arguments");

                        if (assert_result.error.type != ParseErrorType::NONE)
                                return assert_result;
                        break;
                }

                ParseResult result = parse_name(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *left = result.node;

                // parse_optional default assignment
                result = parse_function_default_assignment_or_annotation(
                        parser, left);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                *arg = result.node;

                if (!is_default_arg(*arg) && defaults_only) {
                        return parser_create_error_from_msg(
                                parser,
                                "Non default arguments cannot follow default arguments");
                }

                if (is_default_arg(*arg))
                        defaults_only = true;

                arg = &((*arg)->adjacent_child);

                if (parser->token_arr->current.type == TokenType::CLOSED_PAREN) {
                        break;
                }

                assert_comma_and_skip_over(parser);
                ++arg_position;
        }

        parser->token_arr->next_token();
        return ParseResult{.node = arg_head};
}

static ParseResult parse_lambda_arguments(Parser *parser,
                                          AstNodeLambdaDef *function)
{
        AstNode *arg_head = nullptr;
        AstNode **arg = &arg_head;
        bool defaults_only = false;

        while (parser->token_arr->current.type != TokenType::COLON) {
                Token current_token = parser->token_arr->current;
                AstNode argument_node = {};
                argument_node.token = current_token;

                // *args
                if (current_token.type == TokenType::MULTIPLICATION) {
                        parser->token_arr->next_token();
                        if (function->star) {
                                return parser_create_error_from_msg(
                                        parser,
                                        "Function can only contain one * argument");
                        }

                        if (parser->token_arr->current.type == TokenType::COLON) {
                                break;
                        }
                        if (parser->token_arr->current.type != TokenType::COMMA) {
                                ParseResult result =
                                        parse_name(parser);

                                if (result.error.type != ParseErrorType::NONE)
                                        return result;

                                function->star = result.node;
                        }

                        if (parser->token_arr->current.type == TokenType::COLON) {
                                break;
                        }

                        assert_comma_and_skip_over(parser);
                }

                // **kwargs
                if (parser->token_arr->current.type ==
                    TokenType::EXPONENTIATION) {
                        parser->token_arr->next_token();
                        ParseResult result = parse_name(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        AstNode *name = result.node;

                        result = parse_function_default_assign(parser, name);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        function->double_star = result.node;

                        ParseResult assert_result = assert_token_and_print_debug(
                                parser, TokenType::COLON,

                                "** arguments cannot be followed by any additional arguments");

                        if (assert_result.error.type != ParseErrorType::NONE)
                                return assert_result;
                        break;
                }

                ParseResult result = parse_name(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *left = result.node;

                // parse_optional default assignment
                result = parse_function_default_assign(parser, left);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                *arg = result.node;

                if (!is_default_arg(*arg) && defaults_only) {
                        return parser_create_error_from_msg(
                                parser,
                                "Non default arguments cannot follow default arguments");
                }

                if (is_default_arg(*arg))
                        defaults_only = true;

                arg = &((*arg)->adjacent_child);

                if (parser->token_arr->current.type == TokenType::COLON) {
                        break;
                }

                assert_comma_and_skip_over(parser);
        }

        parser->token_arr->next_token();
        return ParseResult{.node = arg_head};
}

static ParseResult parse_block(Parser *parser)
{
        AstNode *block = node_alloc(parser->ast_arena);
        block->token = parser->token_arr->current;
        block->type = AstNodeType::BLOCK;
        AstNode **child = &block->block.children;

        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::NEWLINE,
                "Error in parsing block");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;

        uint32_t old_indent = parser->token_arr->current.indent_level;
        parser->token_arr->next_token();
        if (parser->token_arr->current.indent_level != old_indent + 1) {
                return parser_create_error_from_msg(
                        parser, "Expected indent in block");
        }

        while (parser->token_arr->current.indent_level > old_indent) {
                if (parser->token_arr->current.type == TokenType::NEWLINE) {
                        parser->token_arr->next_token();
                        continue;
                }

                ParseResult result = parse_statement(parser);

                // must handle errors before assigning the statement to the result
                // result may be null and handle errors re assigns it to a non null erroneous value
                handle_errors_and_assert_end(parser, &result);
                AstNode *statement = result.node;

                *child = statement;
                child = &(*child)->adjacent_child;
        }

        return ParseResult{.node = block};
}

// TODO Figure out the reason for the Python grammars distinction between
// assignment expressions and named expressions
static ParseResult parse_single_assignment_expression(Parser *parser)
{
        AstNode *node = node_alloc(parser->ast_arena);
        node->token = parser->token_arr->current;

        if (parser->token_arr->current.type == TokenType::IDENTIFIER &&
            parser->token_arr->lookahead.type == TokenType::COLON_EQUAL) {
                node->type = AstNodeType::ASSIGNMENT;
                AstNodeAssignment *assignment = &node->assignment;
                // if not already declared then create with any type
                // used for reading python library files
                if (!parser->tables->symbol_table->lookup(
                            parser->token_arr->current.value, parser->scope)) {
                        SymbolTableValue value = {};
                        value.node = node;
                        value.static_type.type = TypeInfoType::ANY;
                        parser->tables->symbol_table->insert(
                                parser->symbol_table_arena,
                                parser->token_arr->current.value, parser->scope, &value);
                }

                ParseResult result = parse_name(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                assignment->left = result.node;
                parser->token_arr->next_token();
                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::ASSIGN,
                        "\nError in parsing assignment on line: ");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;
                parser->token_arr->next_token();
                result = parse_expression(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                assignment->expression = result.node;
                return ParseResult{.node = node};
        }

        else {
                return parse_expression(parser, 0);
        }
}

static ParseResult 
parse_single_assignment_star_expression(Parser *parser)
{
        AstNode node = {};
        node.token = parser->token_arr->current;

        if (parser->token_arr->current.type == TokenType::MULTIPLICATION) {
                return parse_star_expression(parser);
        }

        return parse_single_assignment_expression(parser);
}

static ParseResult 
parse_assignment_star_expressions(Parser *parser, bool wrap_in_tuple)
{
        if (wrap_in_tuple) {
                return parse_comma_seperated_in_tuple_func(
                        parser, parse_single_assignment_star_expression);
        }

        return parse_comma_seperated_func(
                parser, parse_single_assignment_star_expression);
}

static ParseResult parse_single_double_starred_kvpair(Parser *parser)
{
        if (parser->token_arr->current.type == TokenType::EXPONENTIATION) {
                AstNode *double_starred = node_alloc(parser->ast_arena);
                double_starred->type = AstNodeType::STARRED;
                double_starred->token = parser->token_arr->current;
                AstNodeStarExpression *doule_starred_proper =
                        &double_starred->star_expression;
                parser->token_arr->next_token();
                ParseResult result =
                        parse_bitwise_or_minimum_precedence(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                doule_starred_proper->expression = result.node;

                return ParseResult{.node = double_starred};
        }

        else {
                AstNode *kvpair = node_alloc(parser->ast_arena);
                kvpair->type = AstNodeType::KVPAIR;
                kvpair->token = parser->token_arr->current;
                AstNodeKvPair *kvpair_proper = &kvpair->kvpair;
                ParseResult result = parse_expression(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                kvpair_proper->key = result.node;
                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::COLON,
                        "Key Value pairs must be seperated by ':'");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;

                parser->token_arr->next_token();
                result = parse_expression(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                kvpair_proper->value = result.node;

                return ParseResult{.node = kvpair};
        }
}

static ParseResult parse_double_starred_kvpairs(Parser *parser)
{
        ParseResult result = parse_single_double_starred_kvpair(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *current_kvpair = result.node;
        AstNode *head_kvpair = current_kvpair;
        while (parser->token_arr->current.type == TokenType::COMMA) {
                parser->token_arr->next_token();
                result =
                        parse_single_double_starred_kvpair(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                current_kvpair->adjacent_child = result.node;
                current_kvpair = current_kvpair->adjacent_child;
        }

        return ParseResult{.node = head_kvpair};
}

static ParseResult parse_single_for_if_clause(Parser *parser)
{
        AstNode *for_if = node_alloc(parser->ast_arena);
        for_if->type = AstNodeType::FOR_IF;
        for_if->token = parser->token_arr->current;
        AstNodeForIfClause *for_if_proper = &for_if->for_if;
        ParseResult result = parse_star_targets(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        for_if_proper->targets = result.node;
        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::IN_TOK,
                "expected in token in for if clause");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;

        parser->token_arr->next_token();
        result = parse_disjunction(parser, 0);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        for_if_proper->expression = result.node;

        if (parser->token_arr->current.type == TokenType::IF) {
                parser->token_arr->next_token();
                result =
                        parse_expression(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                for_if_proper->if_clause = result.node;
        }

        return ParseResult{.node = for_if};
}

static ParseResult parse_for_if_clauses(Parser *parser)
{
        ParseResult result = parse_single_for_if_clause(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *head = result.node;
        AstNode **child = &head->adjacent_child;
        while (parser->token_arr->current.type == TokenType::FOR) {
                result = parse_single_for_if_clause(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                *child = result.node;
        }

        return ParseResult{.node = head};
}
static ParseResult 
parse_gen_expr_from_first_child(Parser *parser, AstNode *first_child)
{
        if (parser->token_arr->current.type == TokenType::FOR) {
                AstNode *node = node_alloc(parser->ast_arena);
                node->token = parser->token_arr->current;

                if (first_child->type == AstNodeType::STARRED) {
                        return parser_create_error_from_msg(
                                parser,
                                "Generator expressions cannot contain star expressions");
                }
                parser->token_arr->next_token();

                node->type = AstNodeType::GEN_EXPR;
                node->gen_expr.expression = first_child;
                ParseResult result =
                        parse_for_if_clauses(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                node->gen_expr.for_if_clauses = result.node;

                return ParseResult{.node = node};
        }

        return ParseResult{.node = first_child};
}

static ParseResult parse_tuple_or_genxpr_from_first_child(Parser *parser, 
                                                          AstNode *first_child)
{
        ParseResult result = parse_gen_expr_from_first_child(parser,
                                                             first_child);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *maybe_gen = result.node;

        if (maybe_gen != first_child) {
                return ParseResult{.node = maybe_gen};
        }

        AstNode *node = node_alloc(parser->ast_arena);
        node->type = AstNodeType::TUPLE;
        node->tuple.children = first_child;

        if (parser->token_arr->current.type == TokenType::COMMA) {
                node->token = parser->token_arr->current;
                parser->token_arr->next_token();

                if (parser->token_arr->current.type == TokenType::CLOSED_PAREN) {
                        parser->token_arr->next_token();
                        return ParseResult{.node = node};
                }

                result = parse_assignment_star_expressions(parser, false);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                first_child->adjacent_child = result.node;
        }

        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::CLOSED_PAREN,
                "Mismatched parenthesis in tuple declaration");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;
        parser->token_arr->next_token();
        return ParseResult{.node = node};
}

static ParseResult parse_atom(Parser *parser, bool add_to_symbol_table)
{
        if (parser->token_arr->current.type == TokenType::SQUARE_OPEN_PAREN) {
                //parse list
                AstNode *node = node_alloc(parser->ast_arena);
                node->token = parser->token_arr->current;
                node->type = AstNodeType::LIST;

                parser->token_arr->next_token();

                if (parser->token_arr->current.type ==
                    TokenType::SQUARE_CLOSED_PAREN) {
                        parser->token_arr->next_token();
                        return ParseResult{.node = node};
                }

                ParseResult result = parse_single_assignment_star_expression(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                node->list.children = result.node;

                if (parser->token_arr->current.type == TokenType::FOR) {
                        parser->token_arr->next_token();
                        node->type = AstNodeType::LISTCOMP;
                        result = parse_for_if_clauses(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        node->list.children = result.node;
                }

                if (parser->token_arr->current.type == TokenType::COMMA) {
                        parser->token_arr->next_token();
                        result =
                                parse_assignment_star_expressions(parser, false);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        node->list.children->adjacent_child = result.node;
                }

                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::SQUARE_CLOSED_PAREN,
                        "Mismatched parenthesis in list");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;
                parser->token_arr->next_token();
                return ParseResult{.node = node};
        } else if (parser->token_arr->current.type == TokenType::OPEN_PAREN) {
                // parse tuple
                Token token = parser->token_arr->current;
                parser->token_arr->next_token();

                if (parser->token_arr->current.type == TokenType::CLOSED_PAREN) {
                        parser->token_arr->next_token();
                        AstNode *node = node_alloc(parser->ast_arena);
                        node->type = AstNodeType::TUPLE;
                        node->token = token;
                        return ParseResult{.node = node};
                }

                ParseResult result = parse_single_assignment_star_expression(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *first_child = result.node;

                // parse genexpr
                return parse_tuple_or_genxpr_from_first_child(parser, 
                                                              first_child);

        } else if (parser->token_arr->current.type ==
                   TokenType::CURLY_OPEN_PAREN) {
                AstNode *node = node_alloc(parser->ast_arena);
                node->token = parser->token_arr->current;
                parser->token_arr->next_token();

                if (parser->token_arr->current.type ==
                    TokenType::CURLY_CLOSED_PAREN) {
                        parser->token_arr->next_token();
                        node->type = AstNodeType::DICT;
                        return ParseResult{.node = node};
                }

                AstNode maybe_first_child = {};
                maybe_first_child.token = parser->token_arr->current;
                ParseResult result = parse_expression(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *expression = result.node;

                node->type = AstNodeType::DICT;
                if (parser->token_arr->current.type ==
                    TokenType::EXPONENTIATION) {
                        result = parse_double_starred_kvpairs(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        node->dict.children = result.node;
                        return ParseResult{.node = node};
                }

                if (parser->token_arr->current.type == TokenType::COLON_EQUAL) {
                        result = parse_assignment_or_declaration(parser, 
                                                                 expression);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        node->dict.children = result.node;
                        return ParseResult{.node = node};
                }

                AstNode *first_child = node_alloc(parser->ast_arena);
                first_child->token = maybe_first_child.token;

                if (parser->token_arr->current.type == TokenType::COLON) {
                        node->dict.children = first_child;
                        parser->token_arr->next_token();
                        first_child->type = AstNodeType::KVPAIR;
                        AstNodeKvPair *kvpair = &first_child->kvpair;

                        kvpair->key = expression;
                        result = parse_expression(parser, 0);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        kvpair->value = result.node;

                        if (parser->token_arr->current.type == TokenType::FOR) {
                                parser->token_arr->next_token();
                                result =
                                        parse_single_for_if_clause(parser);

                                if (result.error.type != ParseErrorType::NONE)
                                        return result;

                                first_child->adjacent_child = result.node;
                        }

                        if (parser->token_arr->current.type == TokenType::COMMA) {
                                parser->token_arr->next_token();
                                result =
                                        parse_double_starred_kvpairs(parser);

                                if (result.error.type != ParseErrorType::NONE)
                                        return result;

                                first_child->adjacent_child = result.node;
                        }

                } else {
                        // parse set definition
                        if (parser->token_arr->current.type == TokenType::FOR) {
                                parser->token_arr->next_token();
                                result =
                                        parse_single_for_if_clause(parser);

                                if (result.error.type != ParseErrorType::NONE)
                                        return result;

                                first_child->adjacent_child = result.node;
                        }

                        else {
                                parser->token_arr->next_token();
                                result =
                                        parse_assignment_star_expressions(
                                                parser, false);

                                if (result.error.type != ParseErrorType::NONE)
                                        return result;

                                first_child->adjacent_child = result.node;
                        }
                }

                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::CURLY_CLOSED_PAREN,
                        "Mismatched '}' in set definition");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;
                parser->token_arr->next_token();

                return ParseResult{.node = node};
        } else if (parser->token_arr->current.is_literal()) {
                AstNode *node = node_alloc(parser->ast_arena);

                if (parser->token_arr->current.type == TokenType::IDENTIFIER)
                        node->type = AstNodeType::IDENTIFIER;
                else 
                        node->type = AstNodeType::TERMINAL;

                node->token = parser->token_arr->current;
                parser->token_arr->next_token();
                return ParseResult{.node = node};
        }

        else {

                ParseResult result;

                if (parser->token_arr->current.type == TokenType::ENDFILE)
                        result = parser_create_error_from_msg(
                                parser, "Unexpected EOF while parsing");
                else
                        result = parser_create_error_from_msg(
                                parser, "Invalid Syntax");

                parser->token_arr->next_token();

                return result;
        }
}

static ParseResult parse_left(Parser *parser)
{
        AstNode *left = nullptr;
        if (parser->token_arr->current.type == TokenType::OPEN_PAREN) {
                Token token = parser->token_arr->current;
                // parse tuple or genxpr
                parser->token_arr->next_token();

                if (parser->token_arr->current.type == TokenType::CLOSED_PAREN) {
                        parser->token_arr->next_token();
                        AstNode *node = node_alloc(parser->ast_arena);
                        node->type = AstNodeType::TUPLE;
                        node->token = token;
                        return ParseResult{.node = node};
                }

                ParseResult result = parse_star_expression(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                left = result.node;

                if (parser->token_arr->current.type == TokenType::CLOSED_PAREN &&
                    parser->token_arr->lookahead.is_binary_op()) {
                        parser->token_arr->next_token();
                        return ParseResult{.node = left};
                }

                result = parse_assignment_or_declaration(parser, left);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                left = result.node;

                return parse_tuple_or_genxpr_from_first_child(parser, left);

        } else if (parser->token_arr->current.is_unary_op()) {
                left = node_alloc(parser->ast_arena);
                left->type = AstNodeType::UNARY;
                left->token = parser->token_arr->current;
                parser->token_arr->next_token();
                ParseResult result = parse_left(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                left->unary.child = result.node;
        } else {
                ParseResult result = parse_primary(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                left = result.node;
        }

        return ParseResult{.node = left};
}

// NOTE: this is exactly the same as parse_inc_precedence except the precedence cannot be less
// than bitwise_or expressions this is used for python star expressions to disallow boolean operations
// inside star expressions
static ParseResult parse_inc_precedence_minimum_bitwise_or_precedence(
        Parser *parser, AstNode *left, int min_precedence)
{
        Token current_token = parser->token_arr->current;

        if (current_token.type == TokenType::ENDFILE) {
                return parser_create_error_from_msg(
                        parser, "Unexpected EOF while parsing");
        }

        if (!current_token.is_binary_op()) {
                return ParseResult{.node = left};
        }

        int current_precedence = current_token.precedence();

        if (current_precedence >= (int)TokenType::BWOR &&
            current_precedence > min_precedence) {
                parser->token_arr->next_token();
                ParseResult result = parse_expression(parser, current_precedence);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *right = result.node;

                if (!right) {
                        assert(false);
                        return parser_create_error_from_msg(
                                parser, "Unexpected end of token stream");
                }

                AstNode *binary_op_node = node_alloc(parser->ast_arena);
                *binary_op_node =
                        AstNode::create_binary(current_token, left, right);
                return ParseResult{.node = binary_op_node};
        }

        else {
                return ParseResult{.node = left};
        }
}

static ParseResult parse_bitwise_or_minimum_precedence(Parser *parser,
                                                       int min_precedence)
{
        ParseResult result = parse_left(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *left = result.node;
        // parse star primary

        while (true) {
                result =
                        parse_inc_precedence_minimum_bitwise_or_precedence(
                                parser, left, min_precedence);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *node = result.node;

                if (node == left) {
                        break;
                }

                left = node;
        }

        return ParseResult{.node = left};
}

static ParseResult parse_star_expression(Parser *parser)
{
        if (parser->token_arr->current.type != TokenType::MULTIPLICATION) {
                return parse_expression(parser, 0);
        }

        return parse_bitwise_or_minimum_precedence(parser, 0);
}

static ParseResult parse_star_expressions(Parser *parser)
{
        return parse_comma_seperated_in_tuple_func(parser, parse_star_expression);
}

static ParseResult parse_increasing_precedence(Parser *parser, 
                                               AstNode *left, 
                                               int min_precedence)
{
        Token current_token = parser->token_arr->current;

        if (current_token.type == TokenType::ENDFILE) {
                return ParseResult{.node = left};
        }

        if (current_token.type == TokenType::OPEN_PAREN) {
                parser->token_arr->next_token();
                parse_expression(parser, 0);
        }

        if (!current_token.is_binary_op()) {
                return ParseResult{.node = left};
        }

        if (current_token.is_augassign_op() &&
            parser->token_arr->lookahead.type == TokenType::ASSIGN) {
                parser->token_arr->next_token();
                ParseResult result = parse_assignment_or_declaration(parser, left);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *assign = result.node;

                AstNode *binary = node_alloc(parser->ast_arena);
                binary->binary.left = left;
                binary->binary.right = assign->assignment.expression;
                assign->assignment.expression = binary;

                return ParseResult{.node = assign};
        }

        int current_precedence = current_token.precedence();
        if (current_precedence > min_precedence) {
                parser->token_arr->next_token();
                ParseResult result = parse_expression(parser, current_precedence);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *right = result.node;

                if (!right) {
                        assert(false);
                        return parser_create_error_from_msg(
                                parser, "Unexpected end of Token Stream");
                }

                AstNode *binary_op_node = node_alloc(parser->ast_arena);
                *binary_op_node =
                        AstNode::create_binary(current_token, left, right);
                return ParseResult{.node = binary_op_node};
        }

        else {
                return ParseResult{.node = left};
        }
}

static ParseResult parse_disjunction(Parser *parser, int min_precedence)
{
        ParseResult result = parse_left(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *left = result.node;
        // parse star primary

        while (true) {
                result = parse_increasing_precedence(parser, left, min_precedence);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *node = result.node;

                if (node == left) {
                        break;
                }

                left = node;
        }

        return ParseResult{.node = left};
}

static ParseResult parse_expression(Parser *parser, int min_precedence)
{
        ParseResult result = parse_disjunction(parser, min_precedence);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *expr = result.node;

        if (parser->token_arr->current.type != TokenType::IF) {
                return ParseResult{.node = expr};
        }

        AstNode *if_expr = node_alloc(parser->ast_arena);
        if_expr->type = AstNodeType::IF_EXPR;
        if_expr->token = parser->token_arr->current;
        if_expr->if_expr.true_expression = expr;

        parser->token_arr->next_token();
        result = parse_disjunction(parser, 0);
        if (result.error.type != ParseErrorType::NONE)
                return result;

        if_expr->if_expr.condition = result.node;

        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::ELSE,
                "In if expressions else is required");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;
        parser->token_arr->next_token();

        result = parse_expression(parser, min_precedence);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        if_expr->if_expr.false_expression = result.node;

        return ParseResult{.node = if_expr};
}

static ParseResult assert_single_subscript_attribute(Parser *parser, 
                                                     AstNode *node)
{
        if (node->type == AstNodeType::FUNCTION_CALL) {
                return parser_create_error_from_msg(
                        parser, "Cannot assign to function");
        }

        return ParseResult{};
}

static ParseResult parse_declaration(Parser *parser, AstNode *left)
{
        AstNode *node = node_alloc(parser->ast_arena);
        ParseResult result = assert_single_subscript_attribute(parser, left);

        if (result.error.type != ParseErrorType::NONE) {
                return result;
        }

        node->type = AstNodeType::DECLARATION;
        node->token = parser->token_arr->current;
        AstNodeDeclaration *declaration = &node->declaration;
        declaration->name = left;

        SymbolTableValue value = {};
        value.node = node;
        assert_no_redefinition_and_insert_to_symbol_table(
                parser->symbol_table_arena, parser->tables->symbol_table, parser->scope,
                &declaration->name->token, &value, false);

        parser->token_arr->next_token();
        result = parse_type_annotation(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        declaration->annotation = result.node;

        if (parser->token_arr->current.type == TokenType::ASSIGN) {
                parser->token_arr->next_token();
                result =
                        parse_star_expressions(parser);
                if (result.error.type != ParseErrorType::NONE)
                        return result;
                declaration->expression = result.node;
        }

        return ParseResult{.node = node};
}

static ParseResult parse_assignment(Parser *parser, AstNode *left)
{
        AstNode *node = node_alloc(parser->ast_arena);
        node->token = parser->token_arr->current;
        assert_single_subscript_attribute(parser, left);
        node->type = AstNodeType::ASSIGNMENT;
        AstNodeAssignment *assignment = &node->assignment;
        assignment->left = left;
        //

        if (!parser->tables->symbol_table->lookup(parser->token_arr->current.value,
                                          parser->scope)) {
                SymbolTableValue value = {};
                value.node = node;
                value.static_type.type = TypeInfoType::ANY;
                parser->tables->symbol_table->insert(parser->symbol_table_arena,
                                             left->token.value, parser->scope, &value);
        }

        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::ASSIGN,
                "\nError in parsing assignment on line: ");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;
        parser->token_arr->next_token();

        ParseResult result = parse_star_expressions(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        assignment->expression = result.node;

        return ParseResult{.node = node};
}

static ParseResult 
parse_assignment_or_declaration(Parser *parser, AstNode *left)
{
        // FIXME: theres is a bug for multiple assignments
        if (parser->token_arr->current.type == TokenType::ASSIGN) {
                return parse_assignment(parser, left);
        } else if (parser->token_arr->current.type == TokenType::COLON) {
                return parse_declaration(parser, left);
        } else if (parser->token_arr->current.type == TokenType::COLON_EQUAL) {
                return parse_assignment(parser, left);
        }

        return ParseResult{.node = left};
}

static ParseResult parse_next_star_expressions_into_children(Parser *parser, 
                                                             Token token)

{
        AstNode *node = node_alloc(parser->ast_arena);
        node->token = token;
        node->type = AstNodeType::UNARY;
        parser->token_arr->next_token();

        if (parser->token_arr->current.type == TokenType::NEWLINE ||
            parser->token_arr->current.type == TokenType::SEMICOLON) {
                // return node with no error
                return ParseResult{.node = node};
        }

        ParseResult result = parse_star_expression(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *child = result.node;
        node->unary.child = child;

        if (parser->token_arr->current.type == TokenType::COMMA &&
            parser->token_arr->lookahead.type != TokenType::NEWLINE) {
                AstNode *tuple = node_alloc(parser->ast_arena);
                tuple->type = AstNodeType::TUPLE;
                tuple->tuple.children = child;
                AstNode *next_child = tuple->tuple.children->adjacent_child;

                while (parser->token_arr->current.type == TokenType::COMMA) {
                        // if there is more than one return value then make it a tuple
                        parser->token_arr->next_token();
                        result = parse_star_expression(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        next_child = result.node;
                        next_child = next_child->adjacent_child;
                }

                node->unary.child = tuple;
        }

        return ParseResult{.node = node};
}

static ParseResult parse_single_type_param(Parser *parser)
{
        AstNode *param = node_alloc(parser->ast_arena);
        param->type = AstNodeType::TYPE_PARAM;
        param->token = parser->token_arr->current;

        if (parser->token_arr->current.type == TokenType::MULTIPLICATION) {
                parser->token_arr->next_token();
                param->type_param.star = true;
                ParseResult result = parse_name(parser);
                if (result.error.type != ParseErrorType::NONE)
                        return result;
                param->type_param.name = result.node;
                return ParseResult{.node = param};
        }

        if (parser->token_arr->current.type == TokenType::EXPONENTIATION) {
                parser->token_arr->next_token();
                param->type_param.double_star = true;
                ParseResult result = parse_name(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                param->type_param.name = result.node;
                return ParseResult{.node = param};
        }

        ParseResult result = parse_name(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        param->type_param.name = result.node;
        if (parser->token_arr->current.type != TokenType::COLON) {
                return ParseResult{.node = param};
        }

        parser->token_arr->next_token();
        result = parse_expression(parser, 0);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        param->type_param.bound = result.node;

        return ParseResult{.node = param};
}

static ParseResult parse_type_params(Parser *parser)
{
        if (parser->token_arr->current.type != TokenType::SQUARE_OPEN_PAREN) {
                return ParseResult{};
        }

        parser->token_arr->next_token();
        ParseResult result = parse_comma_seperated_func(
                parser, parse_single_type_param);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *params_head = result.node;

        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::SQUARE_CLOSED_PAREN,
                "Error in type parameter sequence");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;
        parser->token_arr->next_token();

        return ParseResult{.node = params_head};
}

static ParseResult parse_function_def(Parser *parser)
{
        AstNode *node = node_alloc(parser->ast_arena);
        node->token = parser->token_arr->current;
        node->type = AstNodeType::FUNCTION_DEF;
        AstNodeFunctionDef *function_proper = &node->function_def;

        parser->token_arr->next_token();
        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::IDENTIFIER,
                "\n Error parsing function definition");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;

        ParseResult result = parse_name(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        function_proper->name = result.node;
        SymbolTableValue value = {};
        value.node = node;
        value.static_type.type = TypeInfoType::FUNCTION;

        SymbolTableEntry *entry =
                assert_no_redefinition_and_insert_to_symbol_table(
                        parser->symbol_table_arena, parser->tables->symbol_table, parser->scope,
                        &function_proper->name->token, &value, true);

        entry->value.static_type.function.custom_symbol = entry;
        result = parse_type_params(parser);

        function_proper->type_params = result.node;

        assert_result = assert_token_and_print_debug(
                parser, TokenType::OPEN_PAREN,
                "\n Error parsing function definition");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;
        parser->token_arr->next_token();

        //TODO make parser->scope pushing and popping nicer
        SymbolTableEntry *last_scope = parser->scope;
        parser->scope = parser->tables->symbol_table->lookup(function_proper->name->token.value,
                                             parser->scope);

        result = parse_function_def_arguments(parser, function_proper);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        function_proper->arguments = result.node;

        if (parser->token_arr->current.type == TokenType::ARROW) {
                parser->token_arr->next_token();
                result = parse_expression(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                function_proper->return_type = result.node;
        }

        assert_result = assert_token_and_print_debug(
                parser, TokenType::COLON,
                "Error parsing function definition");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;

        parser->token_arr->next_token();
        result = parse_block(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        function_proper->block = result.node;
        parser->scope = last_scope;

        return ParseResult{.node = node};
}

static ParseResult parse_dotted_name(Parser *parser)
{
        ParseResult result = parse_name(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *left = result.node;
        AstNode *right;
        Token next_token = parser->token_arr->current;
        if (next_token.type == TokenType::DOT) {
                parser->token_arr->next_token();
                result = parse_dotted_name(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                right = result.node;
        } else {
                return ParseResult{.node = left};
        }

        AstNode *binary = node_alloc(parser->ast_arena);
        *binary = AstNode::create_binary(next_token, left, right);
        return ParseResult{.node = binary};
}

static ParseResult parse_dotted_as_name_import(Parser *parser)
{
        AstNode *import_target = node_alloc(parser->ast_arena);
        import_target->type = AstNodeType::IMPORT_TARGET;
        import_target->token = parser->token_arr->current;
        AstNodeImportTarget *target_proper = &import_target->import_target;
        ParseResult result = parse_dotted_name(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        target_proper->dotted_name = result.node;

        parser->tables->import_list->list[parser->tables->import_list->list_index++] =
                import_target;

        SymbolTableValue val = {};
        val.node = import_target;
        if (parser->token_arr->current.type == TokenType::AS) {
                parser->token_arr->next_token();
                result = parse_name(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                target_proper->as = result.node;
                parser->tables->symbol_table->insert(parser->symbol_table_arena,
                                             target_proper->as->token.value,
                                             parser->scope, &val);
        }

        AstNode *names = target_proper->dotted_name;
        while (names->type == AstNodeType::BINARYEXPR) {
                parser->tables->symbol_table->insert(parser->symbol_table_arena,
                                             names->binary.right->token.value,
                                             parser->scope, &val);
                parser->tables->symbol_table->insert(parser->symbol_table_arena,
                                             names->binary.left->token.value,
                                             parser->scope, &val);
                // its a right leaning tree the left nodes will not have any children
                names = target_proper->dotted_name->binary.right;
        }

        return ParseResult{.node = import_target};
}

static ParseResult parse_single_import_from_as_name(Parser *parser)
{
        AstNode *from_target = node_alloc(parser->ast_arena);
        from_target->type = AstNodeType::FROM_TARGET;
        from_target->token = parser->token_arr->current;
        AstNodeFromImportTarget *target_proper = &from_target->from_target;
        ParseResult result = parse_name(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        target_proper->name = result.node;

        if (parser->token_arr->current.type == TokenType::AS) {
                parser->token_arr->next_token();
                result = parse_name(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                target_proper->as = result.node;
        }

        return ParseResult{.node = from_target};
}

static ParseResult parse_import_from_as_names(Parser *parser)
{
        ParseResult result = parse_single_import_from_as_name(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        AstNode *head = result.node;
        AstNode *child = head;
        while (parser->token_arr->current.type == TokenType::COMMA) {
                parser->token_arr->next_token();
                result =
                        parse_single_import_from_as_name(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                child->adjacent_child = result.node;
                child = child->adjacent_child;
        }

        parser->token_arr->next_token();
        return ParseResult{.node = head};
}

static ParseResult parse_with_item(Parser *parser)
{
        AstNode *with_item = node_alloc(parser->ast_arena);
        with_item->type = AstNodeType::WITH_ITEM;
        with_item->token = parser->token_arr->current;
        ParseResult result = parse_expression(parser, 0);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        with_item->with_item.expression = result.node;

        if (parser->token_arr->current.type == TokenType::AS) {
                parser->token_arr->next_token();
                result = parse_single_star_target(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                with_item->with_item.target = result.node;
        }

        return ParseResult{.node = with_item};
}

static ParseResult parse_class_def(Parser *parser)
{
        AstNode *node = node_alloc(parser->ast_arena);
        node->token = parser->token_arr->current;
        node->type = AstNodeType::CLASS_DEF;
        AstNodeClassDef *class_node = &node->class_def;

        parser->token_arr->next_token();
        ParseResult assert_result = assert_token_and_print_debug(
                parser, TokenType::IDENTIFIER,
                "\n Error parsing class_node definition");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;

        ParseResult result = parse_name(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        class_node->name = result.node;
        SymbolTableValue value = {};
        value.static_type.type = TypeInfoType::CLASS;
        value.node = node;

        SymbolTableEntry *entry =
                assert_no_redefinition_and_insert_to_symbol_table(
                        parser->symbol_table_arena, parser->tables->symbol_table, parser->scope,
                        &class_node->name->token, &value, false);

        // cutsom type value referes to its own entry
        // so that when the symbol table is queried for typing it can update other custom types
        // with this pointer value back to the original declaration
        entry->value.static_type.class_type.custom_symbol = entry;

        result = parse_type_params(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        class_node->type_params = result.node;

        if (parser->token_arr->current.type == TokenType::OPEN_PAREN) {
                parser->token_arr->next_token();
                result = parse_function_call_arguments(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                class_node->arguments = result.node;
        }

        assert_result = assert_token_and_print_debug(
                parser, TokenType::COLON,
                "Error parsing class_node definition");

        if (assert_result.error.type != ParseErrorType::NONE)
                return assert_result;
        parser->token_arr->next_token();
        SymbolTableEntry *last_scope = parser->scope;
        parser->scope = parser->tables->symbol_table->lookup(class_node->name->token.value,
                                             parser->scope);
        result = parse_block(parser);

        if (result.error.type != ParseErrorType::NONE)
                return result;

        class_node->block = result.node;
        parser->scope = last_scope;

        return ParseResult{.node = node};
}

static ParseResult parse_statement(Parser *parser)
{
        Token current_token = parser->token_arr->current;

        if (current_token.type == TokenType::ENDFILE) {
                return parser_create_error_from_msg(parser, "Unexpected EOF");
        }

        // some nodes change this some nodes just return their own
        // TODO: unify this maybe??
        AstNode *node = node_alloc(parser->ast_arena);
        node->token = current_token;

        switch (current_token.type) {
        case TokenType::IDENTIFIER: {
                ParseResult result = parse_star_expressions(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *left = result.node;

                result = parse_assignment_or_declaration(parser, left);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *assign = result.node;
                AstNode **target = &left;

                while (parser->token_arr->current.type == TokenType::ASSIGN) {
                        (*target)->adjacent_child =
                                assign->assignment.expression;

                        result = parse_assignment_or_declaration(parser, left);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        assign = result.node;
                }

                return ParseResult{.node = assign};

        } break;

        case TokenType::MULTIPLICATION: {
                ParseResult result = parse_star_targets(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *left = result.node;

                if (parser->token_arr->current.type == TokenType::COLON ||
                    parser->token_arr->current.type == TokenType::ASSIGN) {
                        return parse_assignment_or_declaration(parser, left);
                }

                return parse_increasing_precedence(parser, left, 0);
        }

        case TokenType::IMPORT: {
                node->type = AstNodeType::IMPORT;
                parser->token_arr->next_token();
                // NOTE: must parse at least one
                ParseResult result = parse_comma_seperated_func(parser, parse_dotted_as_name_import);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                node->import.children = result.node;

                if (!node->import.children) {
                        return parser_create_error_from_msg(
                                parser,
                                "Import statement must have a target");
                }

        } break;
                //TODO relative imports
        case TokenType::FROM: {
                node->type = AstNodeType::FROM;
                AstNodeFrom *from = &node->from;
                parser->token_arr->next_token();
                ParseResult result = parse_dotted_name(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                from->dotted_name = result.node;

                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::IMPORT,
                        "Require an import after from");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;
                parser->token_arr->next_token();

                bool paren = false;
                if (parser->token_arr->current.type ==
                    TokenType::MULTIPLICATION) {
                        from->is_wildcard = true;
                        parser->token_arr->next_token();

                        assert_result = assert_token_and_print_debug(
                                parser, TokenType::NEWLINE,
                                "Cannot have a wildcard and other named targets");

                        if (assert_result.error.type != ParseErrorType::NONE)
                                return assert_result;

                        return ParseResult{.node = node};
                }

                if (parser->token_arr->current.type == TokenType::OPEN_PAREN) {
                        parser->token_arr->next_token();
                        paren = true;
                }

                if (paren) {
                        result = parse_single_import_from_as_name(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        AstNode *child = result.node;
                        from->targets = child;
                        while (parser->token_arr->current.type ==
                               TokenType::COMMA) {
                                parser->token_arr->next_token();
                                result =
                                        parse_single_import_from_as_name(parser);

                                if (result.error.type != ParseErrorType::NONE)
                                        return result;

                                child->adjacent_child = result.node;
                                child = child->adjacent_child;
                        }

                        assert_result =
                                assert_token_and_print_debug(
                                        parser,
                                        TokenType::CLOSED_PAREN,
                                        "Mismatched Paren");

                        if (assert_result.error.type != ParseErrorType::NONE)
                                return assert_result;
                        parser->token_arr->next_token();
                } else {
                        result = parse_import_from_as_names(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        from->targets = result.node;
                }

        } break;

        case TokenType::AT: {
                AstNode *head_decorator = nullptr;
                AstNode **decorator = &head_decorator;
                while (parser->token_arr->current.type == TokenType::AT) {
                        parser->token_arr->next_token();
                        ParseResult result = 
                                        parse_single_assignment_expression(
                                                parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        *decorator = result.node;
                        ParseResult assert_result = assert_token_and_print_debug(
                                parser, TokenType::NEWLINE,
                                "expected newline after decarator declaration");

                        if (assert_result.error.type != ParseErrorType::NONE)
                                return assert_result;

                        decorator = &((*decorator)->adjacent_child);
                        parser->token_arr->next_token();
                }

                if (parser->token_arr->current.type == TokenType::CLASS) {
                        ParseResult result =
                                parse_class_def(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        AstNode *class_def = result.node;
                        class_def->class_def.decarators = head_decorator;
                        return ParseResult{.node = class_def};
                }

                if (parser->token_arr->current.type == TokenType::DEF) {
                        ParseResult result =
                                parse_function_def(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        AstNode *funcion_def = result.node;
                        funcion_def->function_def.decarators = head_decorator;
                        return ParseResult{.node = funcion_def};
                }
        }

        case TokenType::IF: {
                parser->token_arr->next_token();
                node->type = AstNodeType::IF;
                AstNodeIf *if_stmt = &node->if_stmt;
                ParseResult result = parse_single_assignment_expression(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                if_stmt->condition = result.node;
                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::COLON,
                        "\n Error Parsing If Satement");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;
                parser->token_arr->next_token();

                result = parse_block(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                if_stmt->block = result.node;

                result = parse_elif(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                if_stmt->or_else = result.node;

        } break;

        case TokenType::WHILE: {
                node->type = AstNodeType::WHILE;
                AstNodeWhile *while_node = &node->while_loop;
                parser->token_arr->next_token();
                ParseResult result = parse_single_assignment_expression(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                while_node->condition = result.node;

                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::COLON,
                        "Error parsing while statment");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;

                parser->token_arr->next_token();
                result = parse_block(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                while_node->block = result.node;

                result = parse_else(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                while_node->or_else = result.node;
        } break;

        //TODO: find out what the hell a star_target is in the python grammar
        case TokenType::FOR: {
                node->type = AstNodeType::FOR_LOOP;
                node->token = parser->token_arr->current;
                AstNodeForLoop *for_node = &node->for_loop;
                parser->token_arr->next_token();
                ParseResult result = parse_star_targets(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                for_node->targets = result.node;

                ParseResult assert_result = assert_token_and_print_debug(
                        parser,
                        TokenType::IN_TOK,
                        "Error parsing for statment");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;

                parser->token_arr->next_token();
                result = parse_star_expression(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                for_node->expression = result.node;

                assert_result = assert_token_and_print_debug(
                        parser, TokenType::COLON,
                        "expected colon after the end of for statement");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;

                parser->token_arr->next_token();
                result = parse_block(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                for_node->block = result.node;

                result = parse_else(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                for_node->or_else = result.node;
        } break;

        //try block
        case TokenType::TRY: {
                node->type = AstNodeType::TRY;
                AstNodeTry *try_node = &node->try_node;

                parser->token_arr->next_token();
                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::COLON,
                        "Expected colon at the end of try statment");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;

                parser->token_arr->next_token();
                ParseResult result = parse_block(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                try_node->block = result.node;

                // Parse except handlers
                AstNode **handler = &try_node->handlers;
                while (parser->token_arr->current.type != TokenType::ENDFILE) {
                        Token except_token = parser->token_arr->current;

                        if (!(except_token.type == TokenType::EXCEPT)) {
                                break;
                        }

                        AstNode *except = node_alloc(parser->ast_arena);
                        except->token = except_token;
                        except->type = AstNodeType::EXCEPT;
                        AstNodeExcept *except_proper = &except->except;

                        if (!(parser->token_arr->next_token().type ==
                              TokenType::COLON)) {
                                result = parse_expression(parser, 0);

                                if (result.error.type != ParseErrorType::NONE)
                                        return result;

                                except_proper->expression = result.node;

                                if (parser->token_arr->current.type ==
                                    TokenType::AS) {
                                        parser->token_arr->next_token();
                                        parse_name(parser);
                                }

                                assert_result = assert_token_and_print_debug(
                                        parser, TokenType::COLON,
                                        "\n Error Parsing except satement");

                                if (assert_result.error.type != ParseErrorType::NONE)
                                        return assert_result;
                        }

                        parser->token_arr->next_token();
                        result = parse_block(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        except_proper->block = result.node;

                        *handler = except;
                        handler = &((*handler)->adjacent_child);
                }

                // parse else
                result = parse_else(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                try_node->or_else = result.node;

                // parse finally statement
                if (parser->token_arr->current.type == TokenType::FINALLY) {
                        parser->token_arr->next_token();
                        assert_result = assert_token_and_print_debug(
                                parser, TokenType::COLON,
                                "\n Error Parsing finally statement");

                        if (assert_result.error.type != ParseErrorType::NONE)
                                return assert_result;

                        parser->token_arr->next_token();
                        result = parse_block(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        try_node->finally = result.node;
                }

        } break;

        case TokenType::WITH: {
                AstNode *with_node = node_alloc(parser->ast_arena);
                with_node->type = AstNodeType::WITH;
                with_node->token = parser->token_arr->current;
                parser->token_arr->next_token();

                ParseResult result = parse_with_item(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                AstNode *child = result.node;

                with_node->with_statement.items = child;

                while (parser->token_arr->current.type == TokenType::COMMA) {
                        result = parse_with_item(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;

                        child->adjacent_child = result.node;
                        child = child->adjacent_child;
                }

                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::COLON,
                        "expected colon after end of with statemnt");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;

                parser->token_arr->next_token();
                result = parse_block(parser);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                with_node->with_statement.block = result.node;

                return ParseResult{.node = with_node};
        }

        case TokenType::LAMBDA: {
                AstNode *lambda = node_alloc(parser->ast_arena);
                lambda->type = AstNodeType::LAMBDA;
                lambda->token = parser->token_arr->current;
                parser->token_arr->next_token();
                ParseResult result = parse_lambda_arguments(parser, 
                                                            &lambda->lambda);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                lambda->lambda.arguments = result.node;

                result =
                        parse_expression(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                lambda->lambda.expression = result.node;

                return ParseResult{.node = lambda};
        }

        case TokenType::DEF: {
                return parse_function_def(parser);
        }

        case TokenType::CLASS: {
                return parse_class_def(parser);
        }

        case TokenType::RETURN: {
                return parse_next_star_expressions_into_children(
                        parser, parser->token_arr->current);
        }

        case TokenType::YIELD: {
                if (parser->token_arr->lookahead.type != TokenType::FROM) {
                        return parse_next_star_expressions_into_children(
                                parser, parser->token_arr->current);
                }

                parser->token_arr->next_token();
                parser->token_arr->next_token();

                node->type = AstNodeType::UNARY;
                ParseResult result = parse_expression(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                node->unary.child = result.node;

                return ParseResult{.node = node};
        }

        case TokenType::RAISE: {
                parser->token_arr->next_token();
                node->type = AstNodeType::RAISE;
                if (parser->token_arr->lookahead.type == TokenType::NEWLINE) {
                        return ParseResult{.node = node};
                }

                parser->token_arr->next_token();
                ParseResult result =
                        parse_expression(parser, 0);

                if (result.error.type != ParseErrorType::NONE)
                        return result;

                node->raise.expression = result.node;

                if (parser->token_arr->current.type == TokenType::FROM) {
                        parser->token_arr->next_token();
                        result =
                                parse_expression(parser, 0);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;


                        node->raise.from_expression = result.node;
                }

                return ParseResult{.node = node};
        }

        case TokenType::DEL: {
                parser->token_arr->next_token();
                return parse_del_targets(parser);
        }

        case TokenType::MATCH: {
                node->type = AstNodeType::MATCH;
                if (parser->token_arr->lookahead.type == TokenType::MULTIPLICATION) {
                        ParseResult result =
                                parse_single_assignment_star_expression(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;


                        node->match.subject = result.node;
                        node->match.subject->type = AstNodeType::NARY;

                        ParseResult assert_result = assert_token_and_print_debug(
                                parser, TokenType::COMMA,
                                "expected comma after star expresssion in match");

                        if (assert_result.error.type != ParseErrorType::NONE)
                                return assert_result;

                        result = parse_assignment_star_expressions(parser, true);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;


                        node->match.subject->adjacent_child = result.node;
                } else {
                        ParseResult result =
                                parse_single_assignment_expression(parser);

                        if (result.error.type != ParseErrorType::NONE)
                                return result;


                        node->match.subject = result.node;
                }

                ParseResult assert_result = assert_token_and_print_debug(
                        parser, TokenType::COLON,
                        "Expected colon in match statement");

                if (assert_result.error.type != ParseErrorType::NONE)
                        return assert_result;
        }

        case TokenType::GLOBAL: {
                return parse_list_of_names_and_return_modifier(
                                parser, current_token);
        }

        case TokenType::NONLOCAL: {
                return parse_list_of_names_and_return_modifier(
                                parser, current_token);
        }

        case TokenType::PASS: {
                return ParseResult{ .node = parse_single_token_into_node(parser) };
        }

        case TokenType::CONTINUE: {
                return ParseResult{ .node = parse_single_token_into_node(parser) };
        }

        case TokenType::BREAK: {
                return ParseResult{ .node = parse_single_token_into_node(parser) };
        }

        default:
                ParseResult result = parse_star_expression(parser);

                if (result.error.type != ParseErrorType::NONE)

                        return result;


                AstNode *expr = result.node;

                if (expr->type == AstNodeType::TUPLE ||
                    expr->type == AstNodeType::LIST) {
                        return parse_assignment_or_declaration(parser, 
                                                               expr);
                }

                return ParseResult{.node = expr};
        }

        return ParseResult{.node = node};
}

static ParseResult parse_statements(Parser *parser)
{
        AstNode *file_node = node_alloc(parser->ast_arena);
        file_node->token = parser->token_arr->current;
        file_node->type = AstNodeType::FILE;
        AstNode **child = &file_node->file.children;

        while (parser->token_arr->current.type != TokenType::ENDFILE) {
                while (parser->token_arr->current.type == TokenType::NEWLINE) {
                        parser->token_arr->next_token();
                }

                if (parser->token_arr->current.type == TokenType::ENDFILE) {
                        break;
                }

                ParseResult result = parse_statement(parser);

                handle_errors_and_assert_end(parser, &result);
                AstNode *statement = result.node;

                *child = statement;
                child = &((*child)->adjacent_child);
        }

        return ParseResult{.node = file_node};
}

bool ast_node_is_simple(AstNode node) {
        switch(node.type) {
        case AstNodeType::FUNCTION_DEF:
                return false;
        case AstNodeType::IF:
                return false;
        case AstNodeType::CLASS_DEF:
                return false;
        case AstNodeType::WITH:
                return false;
        case AstNodeType::FOR_LOOP:
                return false;
        case AstNodeType::TRY:
                return false;
        case AstNodeType::WHILE:
                return false;
        case AstNodeType::MATCH:
                return false;
        default:
                return true;
        }
}

void handle_errors_and_assert_end(Parser *parser, ParseResult *result)
{
        if (result->error.type != ParseErrorType::NONE) {
                Token *error_token = result->error.token;
                printf("File: %s, line: %d, col: %d, Syntax Error At token: '%s': %s\n",
                       parser->token_arr->filename, error_token->line,
                       error_token->column,
                       TOKEN_STRINGS[(int)error_token->type],
                       result->error.msg);

                assert(result->node->type == AstNodeType::INVALID);
                token_array_goto_end_of_statement(parser->token_arr);
                return;
        }

        if (!result->node) {
                return;
        }

        if (ast_node_is_simple(*result->node)) {
                if (parser->token_arr->current.type != TokenType::NEWLINE &&
                    parser->token_arr->current.type != TokenType::SEMICOLON) {
                        printf("File: %s, line: %d, col: %d, Syntax Error: Statements must end in newline or be seperated semicolons\n",
                               parser->token_arr->filename, parser->token_arr->current.line,
                               parser->token_arr->current.column);

                        token_array_goto_end_of_statement(parser->token_arr);

                }
        }
}


void token_array_goto_end_of_statement(TokenArray *token_array) {
        while (true) {
                if (token_array->current.type == TokenType::NEWLINE)
                        break;
                if (token_array->current.type == TokenType::SEMICOLON)
                        break;
                if (token_array->current.type == TokenType::ENDFILE)
                        break;

                token_array->next_token();
        }
}
