#include <cstdint>
#include <assert.h>

#include "parser.h"
#include "tokeniser.h"
#include "typing.h"
#include "tables.h"
#include "debug.h"

// TODO: MATCH STATMENTS
// TODO: tuple assignments and other assignments
// TODO: slash arguments
// TODO: negative numbers unary operator
// TODO: YIELD STATMENTS
// TODO: ASSERT STATMENTS
// TODO: Augmented assign e.g. += -= stuff
// TODO: Test parenthesis and atom parsing
// TODO: enforce declarations to assign= Baby()
//
//NOTE: python grammar reference
//https://docs.python.org/3/reference/grammar.html
//

static inline void parser_exit_with_error_msg(const char *message,
                                              Tokeniser *tokeniser)
{
        fprintf_s(stderr, "Syntax Error: line: %d col %d\n%s",
                  tokeniser->last_returned.line,
                  tokeniser->last_returned.column, message);
        exit(1);
}

static inline AstNode *node_alloc(Arena *ast_arena)
{
        AstNode *allocated_node =
                (AstNode *)ast_arena->alloc(sizeof(*allocated_node));
        // call constructor because AstNode contains std::string .....
        *allocated_node = AstNode();

        return allocated_node;
}

static SymbolTableEntry *assert_no_redefinition_and_insert_to_symbol_table(
        Arena *table_arena, SymbolTable *table, SymbolTableEntry *scope,
        Token *symbol, SymbolTableValue *value)
{
        if (table->lookup(symbol->value, scope)) {
                fprintf_s(
                        stderr,
                        "Parsing Error: line: %d, col: %d redefinition of '%s'",
                        symbol->line, symbol->column, symbol->value.c_str());
                exit(1);
        }

        return table->insert(table_arena, symbol->value, scope, value);
}

static inline AstNode *parse_single_token_into_node(Tokeniser *tokeniser,
                                                    Arena *ast_arena)
{
        AstNode *node = node_alloc(ast_arena);
        node->token = tokeniser->last_returned;
        tokeniser->next_token();
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

static inline void assert_token_and_print_debug(enum TokenType type,
                                                Token token,
                                                const char *message)
{
        if (token.type != type) {
                fprintf_s(
                        stderr,
                        "Syntax Error: line: %d col %d\nExpected Token '%s' instead of "
                        "'%s'\n",
                        token.line, token.column,
                        debug_enum_to_string(TokenTypeEnumMembers, (int)type),
                        debug_enum_to_string(TokenTypeEnumMembers,
                                             (int)token.type));

                fprintf_s(stderr, "%s\n", message);
                exit(1);
        }
}

static inline void eat_newlines(Tokeniser *tokeniser)
{
        while (tokeniser->last_returned.type == TokenType::NEWLINE)
                tokeniser->next_token();
}

static AstNode *wrap_in_tuple(Tokeniser *tokeniser, Arena *ast_arena,
                              AstNode *head)
{
        AstNode *node = node_alloc(ast_arena);
        node->type = AstNodeType::TUPLE;
        node->token = tokeniser->last_returned;
        node->tuple.children = head;
        return node;
}

static AstNode *parse_single_target(Tokeniser *tokeniser, Arena *ast_arena,
                                    SymbolTableEntry *scope,
                                    CompilerTables *compiler_tables,
                                    Arena *symbol_table_arena)
{
        if (tokeniser->last_returned.type == TokenType::OPEN_PAREN) {
                AstNode *result = parse_single_target(tokeniser, ast_arena,
                                                      scope, compiler_tables,
                                                      symbol_table_arena);
                assert_token_and_print_debug(TokenType::CLOSED_PAREN,
                                             tokeniser->last_returned,
                                             "Mistmatched parenthesis");
                return result;
        }

        return parse_single_subscript_attribute_primary(tokeniser, ast_arena,
                                                        scope, compiler_tables,
                                                        symbol_table_arena);
}

static AstNode *parse_comma_seperated_func(Tokeniser *tokeniser,
                                           Arena *ast_arena,
                                           SymbolTableEntry *scope,
                                           CompilerTables *compiler_tables,
                                           Arena *symbol_table_arena,
                                           ParseSingleFunc func)
{
        AstNode *head = func(tokeniser, ast_arena, scope, compiler_tables,
                             symbol_table_arena);

        AstNode **child = &head->adjacent_child;

        while (tokeniser->last_returned.type == TokenType::COMMA) {
                tokeniser->next_token();
                if (tokeniser->paren_count) {
                        eat_newlines(tokeniser);
                }
                *child = func(tokeniser, ast_arena, scope, compiler_tables,
                              symbol_table_arena);
                child = &((*child)->adjacent_child);
        }

        return head;
}

static AstNode *parse_comma_seperated_in_tuple_func(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena,
        ParseSingleFunc func)
{
        AstNode *head = func(tokeniser, ast_arena, scope, compiler_tables,
                             symbol_table_arena);

        if (tokeniser->last_returned.type != TokenType::COMMA) {
                return head;
        }

        AstNode *tuple = wrap_in_tuple(tokeniser, ast_arena, head);
        AstNode **child = &tuple->tuple.children->adjacent_child;

        while (tokeniser->last_returned.type == TokenType::COMMA) {
                tokeniser->next_token();
                if (tokeniser->paren_count) {
                        eat_newlines(tokeniser);
                }
                *child = func(tokeniser, ast_arena, scope, compiler_tables,
                              symbol_table_arena);
                child = &((*child)->adjacent_child);
        }

        return tuple;
}

static inline AstNode *parse_name(Tokeniser *tokeniser, Arena *ast_arena)
{
        assert_token_and_print_debug(TokenType::IDENTIFIER,
                                     tokeniser->last_returned,
                                     "Failed to parse name");

        AstNode *name = node_alloc(ast_arena);
        name->token = tokeniser->last_returned;
        tokeniser->next_token();
        return name;
}

static AstNode *parse_list_of_names_and_return_modifier(Tokeniser *tokeniser,
                                                        Arena *ast_arena,
                                                        Token token)
{
        AstNode *parent = node_alloc(ast_arena);
        parent->type = AstNodeType::NARY;
        parent->token = token;
        AstNode *current_child =
                parse_name(tokeniser, ast_arena); // parse first child
        AstNode *head_child = current_child;
        while (tokeniser->last_returned.type == TokenType::COMMA) {
                tokeniser->next_token();
                AstNode *name = parse_name(tokeniser, ast_arena);
                current_child->adjacent_child = name;
                current_child = current_child->adjacent_child;
        }

        parent->nary.children = head_child;
        return parent;
}

static AstNode *parse_type_expression(Tokeniser *tokeniser, Arena *ast_arena)
{
        AstNode *node = node_alloc(ast_arena);
        Token current_token = tokeniser->last_returned;
        Token next_token = tokeniser->lookahead;

        node->type = AstNodeType::TYPE_ANNOTATION;
        node->token = current_token;
        node->type_annotation.type = parse_name(tokeniser, ast_arena);

        if (next_token.type == TokenType::SQUARE_OPEN_PAREN) {
                tokeniser->next_token();
                AstNode **child = &node->type_annotation.parameters;
                while (tokeniser->last_returned.type !=
                       TokenType::SQUARE_CLOSED_PAREN) {
                        *child = parse_type_annotation(tokeniser, ast_arena);

                        if (tokeniser->last_returned.type ==
                            TokenType::SQUARE_CLOSED_PAREN) {
                                break;
                        }

                        child = &((*child)->adjacent_child);

                        assert_comma_and_skip_over(tokeniser);
                }

                tokeniser->next_token();
        }

        return node;
}

static AstNode *parse_type_annotation(Tokeniser *tokeniser, Arena *ast_arena)
{
        AstNode *left = parse_type_expression(tokeniser, ast_arena);

        if (tokeniser->last_returned.type == TokenType::BWOR) {
                AstNode *union_type = node_alloc(ast_arena);
                union_type->type = AstNodeType::UNION;
                union_type->token = tokeniser->last_returned;
                union_type->union_type.left = left;
                tokeniser->next_token();
                union_type->union_type.right =
                        parse_type_annotation(tokeniser, ast_arena);

                return union_type;
        } else {
                return left;
        }
}

static AstNode *parse_single_star_target(Tokeniser *tokeniser, Arena *ast_arena,
                                         SymbolTableEntry *scope,
                                         CompilerTables *compiler_tables,
                                         Arena *symbol_table_arena)
{
        if (tokeniser->last_returned.type == TokenType::MULTIPLICATION) {
                AstNode *starred_target = node_alloc(ast_arena);
                starred_target->type = AstNodeType::STARRED;
                starred_target->token = tokeniser->last_returned;
                tokeniser->next_token();
                starred_target->star_expression.expression =
                        parse_single_star_target(tokeniser, ast_arena, scope,
                                                 compiler_tables,
                                                 symbol_table_arena);
                return starred_target;
        } else if (tokeniser->last_returned.type == TokenType::EXPONENTIATION) {
                parser_exit_with_error_msg(
                        "Star targets cannot be double starred", tokeniser);
        }

        return parse_single_subscript_attribute_primary(tokeniser, ast_arena,
                                                        scope, compiler_tables,
                                                        symbol_table_arena);
}

static AstNode *parse_star_targets(Tokeniser *tokeniser, Arena *ast_arena,
                                   SymbolTableEntry *scope,
                                   CompilerTables *compiler_tables,
                                   Arena *symbol_table_arena)
{
        //TODO implement parsing star_atom
        return parse_comma_seperated_in_tuple_func(tokeniser, ast_arena, scope,
                                                   compiler_tables,
                                                   symbol_table_arena,
                                                   parse_single_star_target);
}

static AstNode *parse_function_call_arguments(Tokeniser *tokeniser,
                                              Arena *ast_arena,
                                              SymbolTableEntry *scope,
                                              CompilerTables *compiler_tables,
                                              Arena *symbol_table_arena)
{
        AstNode *args_head = nullptr;
        AstNode **arg = &args_head;
        while (tokeniser->last_returned.type != TokenType::CLOSED_PAREN) {
                if (tokeniser->last_returned.type == TokenType::NEWLINE) {
                        printf("Expected Token ')' before newline\n");
                        exit(1);
                }

                if (tokeniser->lookahead.type == TokenType::ASSIGN) {
                        // check to see if the token before '=' is an identifier
                        assert_token_and_print_debug(
                                TokenType::IDENTIFIER, tokeniser->last_returned,
                                "Keyword arguments must be valid identifiers\n");

                        AstNode *kwarg = node_alloc(ast_arena);
                        kwarg->token = tokeniser->last_returned;
                        kwarg->kwarg.name = parse_name(tokeniser, ast_arena);
                        tokeniser->next_token();
                        kwarg->kwarg.expression = parse_expression(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena, 0);

                        if (!(kwarg->kwarg.expression)) {
                                printf("Failed to parse ufunction argument on line: %d col: %d\n",
                                       tokeniser->last_returned.line,
                                       tokeniser->last_returned.column);
                                break;
                        }

                        *arg = kwarg;
                }

                else {
                        *arg = parse_single_assignment_star_expression(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena);

                        AstNode *maybe_gen = parse_gen_expr_from_first_child(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena, *arg);

                        if (maybe_gen != *arg) {
                                *arg = maybe_gen;
                                assert_token_and_print_debug(
                                        TokenType::CLOSED_PAREN,
                                        tokeniser->last_returned,
                                        "Gen expression without parenthises is only valid for functions with 1 arguemnt");
                                break;
                        }

                        if (!(*arg)) {
                                printf("Failed to parse function argument on line: %d col: %d\n",
                                       tokeniser->last_returned.line,
                                       tokeniser->last_returned.column);

                                break;
                        }
                }

                // dont check for comma if the adjacent_child token is a closing parenthesis
                if (tokeniser->last_returned.type == TokenType::CLOSED_PAREN) {
                        break;
                }

                assert_comma_and_skip_over(tokeniser);
                arg = &((*arg)->adjacent_child);
        }

        tokeniser->next_token();
        return args_head;
}

// NOTE: Python distinguishes between regular primary and those that end
// in just a list slice a and attribute access
// See 'single_subscript_attribute_target'
static AstNode *parse_single_subscript_attribute_primary(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena)
{
        AstNode *prev = nullptr;
        AstNode node = {};
        node.token = tokeniser->last_returned;

        AstNode *left = parse_atom(tokeniser, ast_arena, scope, compiler_tables,
                                   symbol_table_arena);

        Token prev_tok = tokeniser->last_returned;
        while (left != prev) {
                prev = left;
                left = parse_sub_primary(tokeniser, ast_arena, left, scope,
                                         compiler_tables, symbol_table_arena);
        }

        if (left->type == AstNodeType::FUNCTION_CALL) {
                parser_exit_with_error_msg(
                        "Expected either attribute target or subscript",
                        tokeniser);
        }

        return left;
}

static AstNode *parse_targets(Tokeniser *tokeniser, Arena *ast_arena,
                              SymbolTableEntry *scope,
                              CompilerTables *compiler_tables,
                              Arena *symbol_table_arena)
{
        return parse_comma_seperated_in_tuple_func(tokeniser, ast_arena, scope,
                                                   compiler_tables,
                                                   symbol_table_arena,
                                                   parse_single_target);
}

static AstNode *parse_single_del_target(Tokeniser *tokeniser, Arena *ast_arena,
                                        SymbolTableEntry *scope,
                                        CompilerTables *compiler_tables,
                                        Arena *symbol_table_arena)
{
        return parse_single_subscript_attribute_primary(tokeniser, ast_arena,
                                                        scope, compiler_tables,
                                                        symbol_table_arena);
}

static AstNode *parse_del_targets(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena)
{
        return parse_comma_seperated_in_tuple_func(tokeniser, ast_arena, scope,
                                                   compiler_tables,
                                                   symbol_table_arena,
                                                   parse_single_del_target);
}

static AstNode *parse_primary(Tokeniser *tokeniser, Arena *ast_arena,
                              SymbolTableEntry *scope,
                              CompilerTables *compiler_tables,
                              Arena *symbol_table_arena)
{
        AstNode *prev = nullptr;
        AstNode *left = parse_atom(tokeniser, ast_arena, scope, compiler_tables,
                                   symbol_table_arena);

        // parse star primary
        // NOTE: What the fuck does this even mean

        while (left != prev) {
                prev = left;
                left = parse_sub_primary(tokeniser, ast_arena, left, scope,
                                         compiler_tables, symbol_table_arena);
        }

        return left;
}

static AstNode *parse_sub_primary(Tokeniser *tokeniser, Arena *ast_arena,
                                  AstNode *left, SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena)
{
        Token current_token = tokeniser->last_returned;

        if (current_token.type == TokenType::DOT) {
                AstNode *attribute_ref = node_alloc(ast_arena);
                attribute_ref->type = AstNodeType::ATTRIBUTE_REF;
                attribute_ref->token = tokeniser->last_returned;
                tokeniser->next_token();
                attribute_ref->attribute_ref.attribute =
                        parse_name(tokeniser, ast_arena);
                attribute_ref->attribute_ref.name = left;

                return attribute_ref;
        } else if (current_token.type == TokenType::OPEN_PAREN) {
                tokeniser->next_token();

                AstNode *call = node_alloc(ast_arena);
                call->type = AstNodeType::FUNCTION_CALL;
                call->token = current_token;

                call->function_call.args = parse_function_call_arguments(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena);
                call->function_call.expression = left;

                return parse_sub_primary(tokeniser, ast_arena, call, scope,
                                         compiler_tables, symbol_table_arena);
        } else if (current_token.type == TokenType::SQUARE_OPEN_PAREN) {
                tokeniser->next_token();

                AstNode *subscript = node_alloc(ast_arena);
                subscript->token = current_token,
                subscript->type = AstNodeType::SUBSCRIPT;

                AstNodeSubscript *subscript_proper = &subscript->subscript;

                AstNode **child = &subscript_proper->start;
                while (tokeniser->last_returned.type !=
                       TokenType::SQUARE_CLOSED_PAREN) {
                        if (tokeniser->last_returned.type ==
                            TokenType::NEWLINE) {
                                printf("Expected Token ']' before newline\n");
                                exit(1);
                        }

                        *child = parse_expression(tokeniser, ast_arena, scope,
                                                  compiler_tables,
                                                  symbol_table_arena, 0);

                        if (tokeniser->last_returned.type ==
                            TokenType::SQUARE_CLOSED_PAREN) {
                                break;
                        }

                        assert_token_and_print_debug(
                                TokenType::COLON, tokeniser->last_returned,
                                "Subscript expressions must be seperated by a colon");

                        tokeniser->next_token();
                        child = ++child;
                }

                tokeniser->next_token();
                subscript_proper->expression = left;
                return parse_sub_primary(tokeniser, ast_arena, subscript, scope,
                                         compiler_tables, symbol_table_arena);
        } else {
                return left;
        }
}

static AstNode *parse_else(Tokeniser *tokeniser, Arena *ast_arena,
                           SymbolTableEntry *scope,
                           CompilerTables *compiler_tables,
                           Arena *symbol_table_arena)
{
        Token current_token = tokeniser->last_returned;
        switch (current_token.type) {
        case TokenType::ELSE: {
                AstNode *else_node = node_alloc(ast_arena);
                else_node->token = current_token;
                else_node->type = AstNodeType::ELSE;
                AstNodeElse *else_node_proper = &else_node->else_stmt;
                assert_token_and_print_debug(TokenType::COLON,
                                             tokeniser->next_token(),
                                             "\n Error Parsing else satement");

                tokeniser->next_token(); // skip the colon

                else_node_proper->block = parse_block(tokeniser, ast_arena,
                                                      scope, compiler_tables,
                                                      symbol_table_arena);
                return else_node;
        }

        default:
                return nullptr;
        }
}

static AstNode *parse_elif(Tokeniser *tokeniser, Arena *ast_arena,
                           SymbolTableEntry *scope,
                           CompilerTables *compiler_tables,
                           Arena *symbol_table_arena)
{
        Token current_token = tokeniser->last_returned;
        switch (current_token.type) {
        case TokenType::ELIF: {
                tokeniser->next_token();
                AstNode *elif = node_alloc(ast_arena);
                elif->token = current_token;
                AstNodeIf *elif_proper = &elif->if_stmt;

                elif_proper->condition = parse_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, 0);

                assert_token_and_print_debug(TokenType::COLON,
                                             tokeniser->last_returned,
                                             "\n Error Parsing elif satement");

                tokeniser->next_token(); // skip the colon

                elif_proper->block = parse_block(tokeniser, ast_arena, scope,
                                                 compiler_tables,
                                                 symbol_table_arena);
                elif_proper->or_else = parse_elif(tokeniser, ast_arena, scope,
                                                  compiler_tables,
                                                  symbol_table_arena);
                return elif;
        }

        default:
                return parse_else(tokeniser, ast_arena, scope, compiler_tables,
                                  symbol_table_arena);
        }
}

static inline void assert_comma_and_skip_over(Tokeniser *tokeniser)
{
        assert_token_and_print_debug(TokenType::COMMA, tokeniser->last_returned,
                                     "parameters must be seperated by commas");

        tokeniser->next_token();
        eat_newlines(tokeniser);
}
static AstNode *
parse_function_default_assignment(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena, AstNode *left)
{
        AstNode *node = node_alloc(ast_arena);
        node->token = tokeniser->last_returned;

        if (tokeniser->last_returned.type == TokenType::ASSIGN) {
                node->type = AstNodeType::ASSIGNMENT;
                AstNodeAssignment *assignment = &node->assignment;
                assignment->name = left;

                if (!compiler_tables->variable_table->lookup(
                            tokeniser->last_returned.value, scope)) {
                        SymbolTableValue value = {};
                        value.node = node;
                        value.static_type.type = TypeInfoType::ANY;
                        compiler_tables->variable_table->insert(
                                symbol_table_arena, left->token.value, scope,
                                &value);
                }

                assert_token_and_print_debug(
                        TokenType::ASSIGN, tokeniser->last_returned,
                        "\nError in parsing assignment on line: ");
                tokeniser->next_token();

                assignment->expression = parse_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, 0);

                return node;
        } else if (tokeniser->last_returned.type == TokenType::COLON) {
                node->type = AstNodeType::DECLARATION;
                AstNodeDeclaration *declaration = &node->declaration;
                declaration->name = left;

                SymbolTableValue value = {};
                value.node = node;
                assert_no_redefinition_and_insert_to_symbol_table(
                        symbol_table_arena, compiler_tables->variable_table,
                        scope, &declaration->name->token, &value);

                tokeniser->next_token();
                declaration->annotation =
                        parse_type_annotation(tokeniser, ast_arena);

                if (tokeniser->last_returned.type == TokenType::ASSIGN) {
                        tokeniser->next_token();
                        declaration->expression = parse_expression(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena, 0);
                }

                return node;
        } else {
                return left;
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

static AstNode *parse_function_def_arguments(Tokeniser *tokeniser,
                                             Arena *ast_arena,
                                             AstNodeFunctionDef *function,
                                             SymbolTableEntry *scope,
                                             CompilerTables *compiler_tables,
                                             Arena *symbol_table_arena)
{
        AstNode *arg_head = nullptr;
        AstNode **arg = &arg_head;
        bool defaults_only = false;

        while (tokeniser->last_returned.type != TokenType::CLOSED_PAREN) {
                eat_newlines(tokeniser);
                Token current_token = tokeniser->last_returned;
                AstNode argument_node = {};
                argument_node.token = current_token;

                // *args
                if (current_token.type == TokenType::MULTIPLICATION) {
                        tokeniser->next_token();
                        if (function->star) {
                                parser_exit_with_error_msg(
                                        "Function can only contain one * argument",
                                        tokeniser);
                        }

                        function->has_star = true;
                        if (tokeniser->last_returned.type ==
                            TokenType::CLOSED_PAREN) {
                                break;
                        }
                        if (tokeniser->last_returned.type != TokenType::COMMA) {
                                function->star =
                                        parse_name(tokeniser, ast_arena);
                        }

                        if (tokeniser->last_returned.type ==
                            TokenType::CLOSED_PAREN) {
                                break;
                        }

                        assert_comma_and_skip_over(tokeniser);
                }

                // **kwargs
                if (tokeniser->last_returned.type ==
                    TokenType::EXPONENTIATION) {
                        tokeniser->next_token();

                        function->double_star = parse_assignment_or_declaration(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena,
                                parse_name(tokeniser, ast_arena));

                        assert_token_and_print_debug(
                                TokenType::CLOSED_PAREN,
                                tokeniser->last_returned,
                                "** arguments cannot be followed by any additional arguments");
                        break;
                }

                AstNode *left = parse_name(tokeniser, ast_arena);

                // parse_optional default assignment
                *arg = parse_function_default_assignment(tokeniser, ast_arena,
                                                         scope, compiler_tables,
                                                         symbol_table_arena,
                                                         left);

                if (!is_default_arg(*arg) && defaults_only) {
                        parser_exit_with_error_msg(
                                "Non default arguments cannot follow default arguments",
                                tokeniser);
                }

                if (is_default_arg(*arg))
                        defaults_only = true;

                arg = &((*arg)->adjacent_child);

                if (tokeniser->last_returned.type == TokenType::CLOSED_PAREN) {
                        break;
                }

                eat_newlines(tokeniser);
                assert_comma_and_skip_over(tokeniser);
        }

        tokeniser->next_token();
        return arg_head;
}

static AstNode *parse_block(Tokeniser *tokeniser, Arena *ast_arena,
                            SymbolTableEntry *scope,
                            CompilerTables *compiler_tables,
                            Arena *symbol_table_arena)
{
        AstNode *block = node_alloc(ast_arena);
        block->token = tokeniser->last_returned;
        block->type = AstNodeType::BLOCK;
        AstNode **child = &block->block.children;

        assert_token_and_print_debug(TokenType::NEWLINE,
                                     tokeniser->last_returned,
                                     "Error in parsing block");
        uint32_t old_indent = tokeniser->current_indent_level;
        tokeniser->next_token();
        if (tokeniser->current_indent_level != old_indent + 1) {
                parser_exit_with_error_msg("Expected indent in block",
                                           tokeniser);
        }

        while (tokeniser->current_indent_level > old_indent) {
                if (tokeniser->last_returned.type == TokenType::NEWLINE) {
                        tokeniser->next_token();
                        continue;
                }

                AstNode *statement = parse_statement(tokeniser, ast_arena,
                                                     scope, compiler_tables,
                                                     symbol_table_arena);

                if (!statement) {
                        printf("error parsing statment in block\n");
                        //debug_print_parse_tree(&block, 0);
                        return nullptr;
                }

                *child = statement;
                child = &(*child)->adjacent_child;
        }

        return block;
}

// TODO Figure out the reason for the Python grammars distinction between
// assignment expressions and named expressions
static AstNode *parse_single_assignment_expression(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena)
{
        AstNode *node = node_alloc(ast_arena);
        node->token = tokeniser->last_returned;

        if (tokeniser->last_returned.type == TokenType::IDENTIFIER &&
            tokeniser->lookahead.type == TokenType::COLON_EQUAL) {
                node->type = AstNodeType::ASSIGNMENT;
                AstNodeAssignment *assignment = &node->assignment;
                // if not already declared then create with any type
                // used for reading python library files
                if (!compiler_tables->variable_table->lookup(
                            tokeniser->last_returned.value, scope)) {
                        SymbolTableValue value{};
                        value.node = node;
                        value.static_type.type = TypeInfoType::ANY;
                        compiler_tables->variable_table->insert(
                                symbol_table_arena,
                                tokeniser->last_returned.value, scope, &value);
                }

                assignment->name = parse_name(tokeniser, ast_arena);
                assert_token_and_print_debug(
                        TokenType::ASSIGN, tokeniser->next_token(),
                        "\nError in parsing assignment on line: ");
                tokeniser->next_token();
                assignment->expression = parse_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, 0);
                return node;
        }

        else {
                return parse_expression(tokeniser, ast_arena, scope,
                                        compiler_tables, symbol_table_arena, 0);
        }
}

static AstNode *parse_single_assignment_star_expression(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena)
{
        AstNode node = {};
        node.token = tokeniser->last_returned;

        if (tokeniser->last_returned.type == TokenType::MULTIPLICATION) {
                return parse_star_expression(tokeniser, ast_arena, scope,
                                             compiler_tables,
                                             symbol_table_arena);
        }

        return parse_single_assignment_expression(tokeniser, ast_arena, scope,
                                                  compiler_tables,
                                                  symbol_table_arena);
}

static AstNode *
parse_assignment_star_expressions(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena, bool wrap_in_tuple)
{
        if (wrap_in_tuple) {
                return parse_comma_seperated_in_tuple_func(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena,
                        parse_single_assignment_star_expression);
        }

        return parse_comma_seperated_func(
                tokeniser, ast_arena, scope, compiler_tables,
                symbol_table_arena, parse_single_assignment_star_expression);
}

static AstNode *parse_single_double_starred_kvpair(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena)
{
        if (tokeniser->last_returned.type == TokenType::EXPONENTIATION) {
                AstNode *double_starred = node_alloc(ast_arena);
                double_starred->type = AstNodeType::STARRED;
                double_starred->token = tokeniser->last_returned;
                AstNodeStarExpression *doule_starred_proper =
                        &double_starred->star_expression;
                tokeniser->next_token();
                doule_starred_proper->expression =
                        parse_bitwise_or_minimum_precedence(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena, 0);

                return double_starred;
        }

        else {
                AstNode *kvpair = node_alloc(ast_arena);
                kvpair->type = AstNodeType::KVPAIR;
                kvpair->token = tokeniser->last_returned;
                AstNodeKvPair *kvpair_proper = &kvpair->kvpair;
                kvpair_proper->key = parse_expression(tokeniser, ast_arena,
                                                      scope, compiler_tables,
                                                      symbol_table_arena, 0);
                assert_token_and_print_debug(
                        TokenType::COLON, tokeniser->last_returned,
                        "Key Value pairs must be seperated by ':'");

                tokeniser->next_token();
                kvpair_proper->value = parse_expression(tokeniser, ast_arena,
                                                        scope, compiler_tables,
                                                        symbol_table_arena, 0);

                return kvpair;
        }
}

static AstNode *parse_double_starred_kvpairs(Tokeniser *tokeniser,
                                             Arena *ast_arena,
                                             SymbolTableEntry *scope,
                                             CompilerTables *compiler_tables,
                                             Arena *symbol_table_arena)
{
        AstNode *current_kvpair = parse_single_double_starred_kvpair(
                tokeniser, ast_arena, scope, compiler_tables,
                symbol_table_arena);
        AstNode *head_kvpair = current_kvpair;
        while (tokeniser->last_returned.type == TokenType::COMMA) {
                tokeniser->next_token();
                current_kvpair->adjacent_child =
                        parse_single_double_starred_kvpair(tokeniser, ast_arena,
                                                           scope,
                                                           compiler_tables,
                                                           symbol_table_arena);
                current_kvpair = current_kvpair->adjacent_child;
        }

        return head_kvpair;
}

static AstNode *parse_single_for_if_clause(Tokeniser *tokeniser,
                                           Arena *ast_arena,
                                           SymbolTableEntry *scope,
                                           CompilerTables *compiler_tables,
                                           Arena *symbol_table_arena)
{
        AstNode *for_if = node_alloc(ast_arena);
        for_if->type = AstNodeType::FOR_IF;
        for_if->token = tokeniser->last_returned;
        AstNodeForIfClause *for_if_proper = &for_if->for_if;
        for_if_proper->targets = parse_star_targets(tokeniser, ast_arena, scope,
                                                    compiler_tables,
                                                    symbol_table_arena);
        assert_token_and_print_debug(TokenType::IN_TOK,
                                     tokeniser->last_returned,
                                     "expected in token in for if clause");

        tokeniser->next_token();
        for_if_proper->expression = parse_disjunction(tokeniser, ast_arena,
                                                      scope, compiler_tables,
                                                      symbol_table_arena, 0);

        if (tokeniser->last_returned.type == TokenType::IF) {
                tokeniser->next_token();
                for_if_proper->if_clause = parse_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, 0);
        }

        return for_if;
}

static AstNode *parse_for_if_clauses(Tokeniser *tokeniser, Arena *ast_arena,
                                     SymbolTableEntry *scope,
                                     CompilerTables *compiler_tables,
                                     Arena *symbol_table_arena)
{
        AstNode *head = parse_single_for_if_clause(tokeniser, ast_arena, scope,
                                                   compiler_tables,
                                                   symbol_table_arena);
        AstNode **child = &head->adjacent_child;
        while (tokeniser->last_returned.type == TokenType::FOR) {
                *child = parse_single_for_if_clause(tokeniser, ast_arena, scope,
                                                    compiler_tables,
                                                    symbol_table_arena);
        }

        return head;
}
static AstNode *parse_gen_expr_from_first_child(Tokeniser *tokeniser,
                                              Arena *ast_arena,
                                              SymbolTableEntry *scope,
                                              CompilerTables *compiler_tables,
                                              Arena *symbol_table_arena,
                                              AstNode *first_child)
{
        if (tokeniser->last_returned.type == TokenType::FOR) {
                AstNode *node = node_alloc(ast_arena);
                node->token = tokeniser->last_returned;

                if (first_child->type == AstNodeType::STARRED) {
                        parser_exit_with_error_msg(
                                "Generator expressions cannot contain star expressions",
                                tokeniser);
                }
                tokeniser->next_token();

                node->type = AstNodeType::GEN_EXPR;
                node->gen_expr.expression = first_child;
                node->gen_expr.for_if_clauses = parse_for_if_clauses(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena);

                return node;
        }

        return first_child;
}

static AstNode *parse_tuple_or_genxpr_from_first_child(Tokeniser *tokeniser,
                                              Arena *ast_arena,
                                              SymbolTableEntry *scope,
                                              CompilerTables *compiler_tables,
                                              Arena *symbol_table_arena,
                                              AstNode *first_child)
{
        AstNode *maybe_gen = parse_gen_expr_from_first_child(tokeniser, ast_arena, scope,
                                        compiler_tables, symbol_table_arena,
                                        first_child);

        if (maybe_gen != first_child) {
                return maybe_gen;
        }

        AstNode *node = node_alloc(ast_arena);
        node->type = AstNodeType::TUPLE;
        node->tuple.children = first_child;

        if (tokeniser->last_returned.type == TokenType::COMMA) {
                AstNode *node = node_alloc(ast_arena);
                node->token = tokeniser->last_returned;

                tokeniser->next_token();
                first_child->adjacent_child = parse_assignment_star_expressions(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, false);
        }

        assert_token_and_print_debug(
                TokenType::CLOSED_PAREN, tokeniser->last_returned,
                "Mismatched parenthesis in tuple declaration");
        tokeniser->next_token();
        return node;
}

static AstNode *parse_atom(Tokeniser *tokeniser, Arena *ast_arena,
                           SymbolTableEntry *scope,
                           CompilerTables *compiler_tables,
                           Arena *symbol_table_arena)
{
        if (tokeniser->last_returned.type == TokenType::SQUARE_OPEN_PAREN) {
                //parse list
                AstNode *node = node_alloc(ast_arena);
                node->token = tokeniser->last_returned;
                node->type = AstNodeType::LIST;

                tokeniser->next_token();

                if (tokeniser->last_returned.type ==
                    TokenType::SQUARE_CLOSED_PAREN) {
                        tokeniser->next_token();
                        return node;
                }

                node->list.children = parse_single_assignment_star_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena);

                if (tokeniser->last_returned.type == TokenType::FOR) {
                        tokeniser->next_token();
                        node->type = AstNodeType::LISTCOMP;
                        node->list.children = parse_for_if_clauses(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena);
                }

                if (tokeniser->last_returned.type == TokenType::COMMA) {
                        tokeniser->next_token();
                        node->list.children->adjacent_child =
                                parse_assignment_star_expressions(
                                        tokeniser, ast_arena, scope,
                                        compiler_tables, symbol_table_arena,
                                        false);
                }

                assert_token_and_print_debug(TokenType::SQUARE_CLOSED_PAREN,
                                             tokeniser->last_returned,
                                             "Mismatched parenthesis in list");
                tokeniser->next_token();
                return node;
        } else if (tokeniser->last_returned.type == TokenType::OPEN_PAREN) {
                // parse tuple
                Token token = tokeniser->last_returned;
                tokeniser->next_token();

                if (tokeniser->last_returned.type == TokenType::CLOSED_PAREN) {
                        tokeniser->next_token();
                        AstNode *node = node_alloc(ast_arena);
                        node->type = AstNodeType::TUPLE;
                        node->token = token;
                        return node;
                }

                AstNode *first_child = parse_single_assignment_star_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena);

                // parse genexpr
                return parse_tuple_or_genxpr_from_first_child(tokeniser, ast_arena,
                                                       scope, compiler_tables,
                                                       symbol_table_arena,
                                                       first_child);


        } else if (tokeniser->last_returned.type == TokenType::CURLY_OPEN_PAREN) {
                AstNode *node = node_alloc(ast_arena);
                node->token = tokeniser->last_returned;
                tokeniser->next_token();

                if (tokeniser->last_returned.type ==
                    TokenType::CURLY_CLOSED_PAREN) {
                        tokeniser->next_token();
                        node->type = AstNodeType::DICT;
                        return node;
                }

                AstNode maybe_first_child = {};
                maybe_first_child.token = tokeniser->last_returned;
                AstNode *expression = parse_expression(tokeniser, ast_arena,
                                                       scope, compiler_tables,
                                                       symbol_table_arena, 0);


                node->type = AstNodeType::DICT;
                if (tokeniser->last_returned.type ==
                    TokenType::EXPONENTIATION) {
                        node->dict.children = parse_double_starred_kvpairs(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena);
                        return node;
                }

                if (tokeniser->last_returned.type == TokenType::COLON_EQUAL) {
                        node->dict.children = parse_assignment_or_declaration(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena, expression);
                        return node;
                }

                AstNode *first_child = node_alloc(ast_arena);
                first_child->token = maybe_first_child.token;

                if (tokeniser->last_returned.type == TokenType::COLON) {
                        node->dict.children = first_child;
                        tokeniser->next_token();
                        first_child->type = AstNodeType::KVPAIR;
                        AstNodeKvPair *kvpair = &first_child->kvpair;

                        kvpair->key = expression;
                        kvpair->value = parse_expression(tokeniser, ast_arena,
                                                         scope, compiler_tables,
                                                         symbol_table_arena, 0);

                        if (tokeniser->last_returned.type == TokenType::FOR) {
                                tokeniser->next_token();
                                first_child->adjacent_child =
                                        parse_single_for_if_clause(
                                                tokeniser, ast_arena, scope,
                                                compiler_tables,
                                                symbol_table_arena);
                        }

                        if (tokeniser->last_returned.type == TokenType::COMMA) {
                                tokeniser->next_token();
                                first_child->adjacent_child =
                                        parse_double_starred_kvpairs(
                                                tokeniser, ast_arena, scope,
                                                compiler_tables,
                                                symbol_table_arena);
                        }

                } else {
                        // parse set definition
                        if (tokeniser->last_returned.type == TokenType::FOR) {
                                tokeniser->next_token();
                                first_child->adjacent_child =
                                        parse_single_for_if_clause(
                                                tokeniser, ast_arena, scope,
                                                compiler_tables,
                                                symbol_table_arena);
                        }

                        else {
                                tokeniser->next_token();
                                first_child->adjacent_child =
                                        parse_assignment_star_expressions(
                                                tokeniser, ast_arena, scope,
                                                compiler_tables,
                                                symbol_table_arena, false);
                        }
                }

                assert_token_and_print_debug(
                        TokenType::CURLY_CLOSED_PAREN, tokeniser->last_returned,
                        "Mismatched '}' in set definition");
                tokeniser->next_token();

                return node;
        } else if (tokeniser->last_returned.is_literal()) {
                AstNode *node = node_alloc(ast_arena);
                node->token = tokeniser->last_returned;
                tokeniser->next_token();
                return node;
        }

        else {
                return nullptr;
        }
}

static inline AstNode *parse_left(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena)
{
        AstNode *left = nullptr;
        if (tokeniser->last_returned.type == TokenType::OPEN_PAREN) {
                Token token = tokeniser->last_returned;
                // parse tuple or genxpr
                tokeniser->next_token();

                if (tokeniser->last_returned.type == TokenType::CLOSED_PAREN) {
                        tokeniser->next_token();
                        AstNode *node = node_alloc(ast_arena);
                        node->type = AstNodeType::TUPLE;
                        node->token = token;
                        return node;
                }

                left = parse_star_expression(tokeniser, ast_arena, scope,
                                             compiler_tables,
                                             symbol_table_arena);

                if (tokeniser->last_returned.type == TokenType::CLOSED_PAREN &&
                    tokeniser->lookahead.is_binary_op()) {
                        tokeniser->next_token();
                        return left;
                }

                left = parse_assignment_or_declaration(tokeniser, ast_arena,
                                                       scope, compiler_tables,
                                                       symbol_table_arena,
                                                       left);

                return parse_tuple_or_genxpr_from_first_child(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, left);

        } else if (tokeniser->last_returned.is_unary_op()) {
                left = node_alloc(ast_arena);
                left->type = AstNodeType::UNARY;
                left->token = tokeniser->last_returned;
                tokeniser->next_token();
                left->unary.child = parse_left(tokeniser, ast_arena, scope,
                                               compiler_tables,
                                               symbol_table_arena);
        } else {
                left = parse_primary(tokeniser, ast_arena, scope,
                                     compiler_tables, symbol_table_arena);
        }

        return left;
}

// NOTE: this is exactly the same as parse_inc_precedence except the precedence cannot be less
// than bitwise_or expressions this is used for python star expressions to disallow boolean operations
// inside star expressions
static AstNode *parse_inc_precedence_minimum_bitwise_or_precedence(
        Tokeniser *tokeniser, Arena *ast_arena, AstNode *left,
        SymbolTableEntry *scope, CompilerTables *compiler_tables,
        Arena *symbol_table_arena, int min_precedence)
{
        Token current_token = tokeniser->last_returned;

        if (current_token.type == TokenType::ENDFILE) {
                printf("Unexpected EOF while parsing expression\n");
                return nullptr;
        }

        if (!current_token.is_binary_op()) {
                return left;
        }

        int current_precedence = current_token.precedence();

        if (current_precedence >= (int)TokenType::BWOR &&
            current_precedence > min_precedence) {
                tokeniser->next_token();
                AstNode *right = parse_expression(tokeniser, ast_arena, scope,
                                                  compiler_tables,
                                                  symbol_table_arena,
                                                  current_precedence);

                if (!right) {
                        printf("UNEXPECTED END OF TOKEN STREAM\n");
                        return nullptr;
                }

                AstNode *binary_op_node = node_alloc(ast_arena);
                *binary_op_node =
                        AstNode::create_binary(current_token, left, right);
                return binary_op_node;
        }

        else {
                return left;
        }
}

static AstNode *parse_bitwise_or_minimum_precedence(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena,
        int min_precedence)
{
        AstNode *left = parse_left(tokeniser, ast_arena, scope, compiler_tables,
                                   symbol_table_arena);
        // parse star primary

        while (true) {
                AstNode *node =
                        parse_inc_precedence_minimum_bitwise_or_precedence(
                                tokeniser, ast_arena, left, scope,
                                compiler_tables, symbol_table_arena,
                                min_precedence);

                if (node == left) {
                        break;
                }

                left = node;
        }

        return left;
}

static AstNode *parse_star_expression(Tokeniser *tokeniser, Arena *ast_arena,
                                      SymbolTableEntry *scope,
                                      CompilerTables *compiler_tables,
                                      Arena *symbol_table_arena)
{
        if (tokeniser->last_returned.type != TokenType::MULTIPLICATION) {
                return parse_expression(tokeniser, ast_arena, scope,
                                        compiler_tables, symbol_table_arena, 0);
        }

        return parse_bitwise_or_minimum_precedence(tokeniser, ast_arena, scope,
                                                   compiler_tables,
                                                   symbol_table_arena, 0);
}

static AstNode *parse_star_expressions(Tokeniser *tokeniser, Arena *ast_arena,
                                       SymbolTableEntry *scope,
                                       CompilerTables *compiler_tables,
                                       Arena *symbol_table_arena)
{
        return parse_comma_seperated_in_tuple_func(tokeniser, ast_arena, scope,
                                                   compiler_tables,
                                                   symbol_table_arena,
                                                   parse_star_expression);
}

static AstNode *parse_increasing_precedence(Tokeniser *tokeniser,
                                            Arena *ast_arena, AstNode *left,
                                            SymbolTableEntry *scope,
                                            CompilerTables *compiler_tables,
                                            Arena *symbol_table_arena,
                                            int min_precedence)
{
        Token current_token = tokeniser->last_returned;

        if (current_token.type == TokenType::ENDFILE) {
                return left;
        }

        if (current_token.type == TokenType::OPEN_PAREN) {
                tokeniser->next_token();
                parse_expression(tokeniser, ast_arena, scope, compiler_tables,
                                 symbol_table_arena, 0);
        }

        if (!current_token.is_binary_op()) {
                return left;
        }

        int current_precedence = current_token.precedence();
        if (current_precedence > min_precedence) {
                tokeniser->next_token();
                AstNode *right = parse_expression(tokeniser, ast_arena, scope,
                                                  compiler_tables,
                                                  symbol_table_arena,
                                                  current_precedence);

                if (!right) {
                        printf("UNEXPECTED END OF TOKEN STREAM\n");
                        return nullptr;
                }

                AstNode *binary_op_node = node_alloc(ast_arena);
                *binary_op_node =
                        AstNode::create_binary(current_token, left, right);
                return binary_op_node;
        }

        else {
                return left;
        }
}

static AstNode *parse_disjunction(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena, int min_precedence)
{
        AstNode *left = parse_left(tokeniser, ast_arena, scope, compiler_tables,
                                   symbol_table_arena);
        // parse star primary

        while (true) {
                AstNode *node = parse_increasing_precedence(
                        tokeniser, ast_arena, left, scope, compiler_tables,
                        symbol_table_arena, min_precedence);
                if (node == left) {
                        break;
                }

                left = node;
        }

        return left;
}

static AstNode *parse_expression(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope,
                                 CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena, int min_precedence)
{
        AstNode *expr = parse_disjunction(tokeniser, ast_arena, scope,
                                          compiler_tables, symbol_table_arena,
                                          min_precedence);
        if (tokeniser->last_returned.type != TokenType::IF) {
                return expr;
        }

        AstNode *if_expr = node_alloc(ast_arena);
        if_expr->type = AstNodeType::IF_EXPR;
        if_expr->token = tokeniser->last_returned;
        if_expr->if_expr.true_expression = expr;

        tokeniser->next_token();
        if_expr->if_expr.condition = parse_disjunction(tokeniser, ast_arena,
                                                       scope, compiler_tables,
                                                       symbol_table_arena, 0);

        assert_token_and_print_debug(TokenType::ELSE, tokeniser->last_returned,
                                     "In if expressions else is required");
        tokeniser->next_token();

        if_expr->if_expr.false_expression =
                parse_expression(tokeniser, ast_arena, scope, compiler_tables,
                                 symbol_table_arena, min_precedence);

        return if_expr;
}

static void assert_single_subscript_attribute(AstNode *node,
                                              Tokeniser *tokeniser)
{
        if (node->type == AstNodeType::FUNCTION_CALL) {
                parser_exit_with_error_msg("Cannot assign to function",
                                           tokeniser);
        }
}
static AstNode *parse_declaration(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope,
                                 CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena, AstNode *left)
{
        AstNode *node = node_alloc(ast_arena);
        assert_single_subscript_attribute(left, tokeniser);
        node->type = AstNodeType::DECLARATION;
        node->token = tokeniser->last_returned;
        AstNodeDeclaration *declaration = &node->declaration;
        declaration->name = left;

        SymbolTableValue value = {};
        value.node = node;
        assert_no_redefinition_and_insert_to_symbol_table(
                symbol_table_arena, compiler_tables->variable_table, scope,
                &declaration->name->token, &value);

        tokeniser->next_token();
        declaration->annotation = parse_type_annotation(tokeniser, ast_arena);

        if (tokeniser->last_returned.type == TokenType::ASSIGN) {
                tokeniser->next_token();
                declaration->expression = parse_star_expressions(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena);
        }

        return node;
}

static AstNode *parse_assignment(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope,
                                 CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena, AstNode *left)
{
        AstNode *node = node_alloc(ast_arena);
        node->token = tokeniser->last_returned;
        assert_single_subscript_attribute(left, tokeniser);
        node->type = AstNodeType::ASSIGNMENT;
        AstNodeAssignment *assignment = &node->assignment;
        assignment->name = left;
        if (!compiler_tables->variable_table->lookup(
                    tokeniser->last_returned.value, scope)) {
                SymbolTableValue value = {};
                value.node = node;
                value.static_type.type = TypeInfoType::ANY;
                compiler_tables->variable_table->insert(
                        symbol_table_arena, left->token.value, scope, &value);
        }

        assert_token_and_print_debug(TokenType::ASSIGN,
                                     tokeniser->last_returned,
                                     "\nError in parsing assignment on line: ");
        tokeniser->next_token();

        assignment->expression = parse_star_expressions(tokeniser, ast_arena,
                                                        scope, compiler_tables,
                                                        symbol_table_arena);

        return node;
}

static AstNode *parse_assignment_or_declaration(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope,
                                 CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena, AstNode *left)
{
        if (tokeniser->last_returned.type == TokenType::ASSIGN) {
                return parse_assignment(tokeniser, ast_arena, scope,
                                         compiler_tables, symbol_table_arena,
                                         left);
        } else if (tokeniser->last_returned.type == TokenType::COLON) {
                return parse_declaration(tokeniser, ast_arena, scope,
                                         compiler_tables, symbol_table_arena,
                                         left);
        } else if (tokeniser->last_returned.type == TokenType::COLON_EQUAL) {
                return parse_assignment(tokeniser, ast_arena, scope,
                                        compiler_tables, symbol_table_arena,
                                        left);
        }

        return left;
}

static AstNode *parse_next_star_expressions_into_children(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena, Token token)
{
        AstNode *node = node_alloc(ast_arena);
        node->token = token;
        node->type = AstNodeType::UNARY;
        tokeniser->next_token();
        AstNode *child = parse_star_expression(tokeniser, ast_arena, scope,
                                               compiler_tables,
                                               symbol_table_arena);
        node->unary.child = child;

        if (tokeniser->last_returned.type == TokenType::COMMA &&
            tokeniser->lookahead.type != TokenType::NEWLINE) {
                AstNode *tuple = node_alloc(ast_arena);
                tuple->type = AstNodeType::TUPLE;
                tuple->tuple.children = child;
                AstNode *next_child = tuple->tuple.children->adjacent_child;

                while (tokeniser->last_returned.type == TokenType::COMMA) {
                        eat_newlines(tokeniser);
                        // if there is more than one return value then make it a tuple
                        tokeniser->next_token();
                        next_child = parse_star_expression(tokeniser, ast_arena,
                                                           scope,
                                                           compiler_tables,
                                                           symbol_table_arena);
                        next_child = next_child->adjacent_child;
                        eat_newlines(tokeniser);
                }

                node->unary.child = tuple;
        }

        return node;
}

static AstNode *parse_function_def(Tokeniser *tokeniser, Arena *ast_arena,
                                   SymbolTableEntry *scope,
                                   CompilerTables *compiler_tables,
                                   Arena *symbol_table_arena)
{
        AstNode *node = node_alloc(ast_arena);
        node->token = tokeniser->last_returned;
        node->type = AstNodeType::FUNCTION_DEF;
        AstNodeFunctionDef *function_proper = &node->function_def;

        assert_token_and_print_debug(TokenType::IDENTIFIER,
                                     tokeniser->next_token(),
                                     "\n Error parsing function definition");

        function_proper->name = parse_name(tokeniser, ast_arena);
        SymbolTableValue value = {};
        value.node = node;

        assert_no_redefinition_and_insert_to_symbol_table(
                symbol_table_arena, compiler_tables->function_table, scope,
                &function_proper->name->token, &value);

        assert_token_and_print_debug(TokenType::OPEN_PAREN,
                                     tokeniser->last_returned,
                                     "\n Error parsing function definition");
        tokeniser->next_token();

        //TODO make scope pushing and popping nicer
        SymbolTableEntry *last_scope = scope;
        scope = compiler_tables->function_table->lookup(
                function_proper->name->token.value, scope);

        function_proper->arguments = parse_function_def_arguments(
                tokeniser, ast_arena, function_proper, scope, compiler_tables,
                symbol_table_arena);

        if (tokeniser->last_returned.type == TokenType::ARROW) {
                tokeniser->next_token();
                function_proper->return_type = parse_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, 0);
        }

        assert_token_and_print_debug(TokenType::COLON, tokeniser->last_returned,
                                     "Error parsing function definition");

        tokeniser->next_token();
        function_proper->block = parse_block(tokeniser, ast_arena, scope,
                                             compiler_tables,
                                             symbol_table_arena);
        scope = last_scope;

        return node;
}

static AstNode *parse_dotted_name(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena)
{
        AstNode *left = parse_name(tokeniser, ast_arena);
        AstNode *right;
        Token next_token = tokeniser->last_returned;
        if (next_token.type == TokenType::DOT) {
                tokeniser->next_token();
                right = parse_dotted_name(tokeniser, ast_arena, scope,
                                          compiler_tables, symbol_table_arena);
        } else {
                return left;
        }

        AstNode *binary = node_alloc(ast_arena);
        *binary = AstNode::create_binary(next_token, left, right);
        return binary;
}

static AstNode *parse_dotted_as_name_import(Tokeniser *tokeniser,
                                            Arena *ast_arena,
                                            SymbolTableEntry *scope,
                                            CompilerTables *compiler_tables,
                                            Arena *symbol_table_arena)
{
        AstNode *import_target = node_alloc(ast_arena);
        import_target->type = AstNodeType::IMPORT_TARGET;
        import_target->token = tokeniser->last_returned;
        AstNodeImportTarget *target_proper = &import_target->import_target;
        target_proper->dotted_name = parse_dotted_name(tokeniser, ast_arena,
                                                       scope, compiler_tables,
                                                       symbol_table_arena);

        if (tokeniser->last_returned.type == TokenType::AS) {
                tokeniser->next_token();
                target_proper->as = parse_name(tokeniser, ast_arena);
        }

        return import_target;
}

static AstNode *parse_single_import_from_as_name(Tokeniser *tokeniser,
                                                 Arena *ast_arena)
{
        AstNode *from_target = node_alloc(ast_arena);
        from_target->type = AstNodeType::FROM_TARGET;
        from_target->token = tokeniser->last_returned;
        AstNodeFromImportTarget *target_proper = &from_target->from_target;
        target_proper->name = parse_name(tokeniser, ast_arena);

        if (tokeniser->last_returned.type == TokenType::AS) {
                tokeniser->next_token();
                target_proper->as = parse_name(tokeniser, ast_arena);
        }

        return from_target;
}

static AstNode *parse_import_from_as_names(Tokeniser *tokeniser,
                                           Arena *ast_arena)
{
        AstNode *head = parse_single_import_from_as_name(tokeniser, ast_arena);
        AstNode *child = head;
        while (tokeniser->last_returned.type == TokenType::COMMA) {
                tokeniser->next_token();
                child->adjacent_child =
                        parse_single_import_from_as_name(tokeniser, ast_arena);
                child = child->adjacent_child;
        }

        tokeniser->next_token();
        return head;
}

static AstNode *parse_with_item(Tokeniser *tokeniser, Arena *ast_arena,
                                SymbolTableEntry *scope,
                                CompilerTables *compiler_tables,
                                Arena *symbol_table_arena)
{
        AstNode *with_item = node_alloc(ast_arena);
        with_item->type = AstNodeType::WITH_ITEM;
        with_item->token = tokeniser->last_returned;
        with_item->with_item.expression =
                parse_expression(tokeniser, ast_arena, scope, compiler_tables,
                                 symbol_table_arena, 0);

        if (tokeniser->last_returned.type == TokenType::AS) {
                tokeniser->next_token();
                with_item->with_item.target = parse_single_star_target(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena);
        }

        return with_item;
}

static AstNode *parse_class_def(Tokeniser *tokeniser, Arena *ast_arena,
                                SymbolTableEntry *scope,
                                CompilerTables *compiler_tables,
                                Arena *symbol_table_arena)
{
        AstNode *node = node_alloc(ast_arena);
        node->token = tokeniser->last_returned;
        node->type = AstNodeType::CLASS_DEF;
        AstNodeClassDef *class_node = &node->class_def;

        assert_token_and_print_debug(TokenType::IDENTIFIER,
                                     tokeniser->next_token(),
                                     "\n Error parsing class_node definition");

        class_node->name = parse_name(tokeniser, ast_arena);
        SymbolTableValue value = {};
        value.static_type.type = TypeInfoType::CLASS;
        value.node = node;

        SymbolTableEntry *entry =
                assert_no_redefinition_and_insert_to_symbol_table(
                        symbol_table_arena, compiler_tables->class_table, scope,
                        &class_node->name->token, &value);

        // cutsom type value referes to its own entry
        // so that when the symbol table is queried for typing it can update other custom types
        // with this pointer value back to the original declaration
        entry->value.static_type.custom_symbol = entry;

        if (tokeniser->last_returned.type == TokenType::OPEN_PAREN) {
                tokeniser->next_token();
                class_node->arguments = parse_function_call_arguments(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena);
        }

        assert_token_and_print_debug(TokenType::COLON, tokeniser->last_returned,
                                     "Error parsing class_node definition");
        tokeniser->next_token();
        SymbolTableEntry *last_scope = scope;
        scope = compiler_tables->class_table->lookup(
                class_node->name->token.value, scope);
        class_node->block = parse_block(tokeniser, ast_arena, scope,
                                        compiler_tables, symbol_table_arena);
        scope = last_scope;

        return node;
}

static AstNode *parse_statement(Tokeniser *tokeniser, Arena *ast_arena,
                                SymbolTableEntry *scope,
                                CompilerTables *compiler_tables,
                                Arena *symbol_table_arena)
{
        Token current_token = tokeniser->last_returned;

        if (current_token.type == TokenType::ENDFILE) {
                return nullptr;
        }

        // some nodes change this some nodes just return their own
        // TODO: unify this maybe??
        AstNode *node = node_alloc(ast_arena);
        node->token = current_token;

        switch (current_token.type) {
        case TokenType::IDENTIFIER: {
                AstNode *left = parse_star_expressions(tokeniser, ast_arena,
                                                       scope, compiler_tables,
                                                       symbol_table_arena);

                AstNode *assign = parse_assignment_or_declaration(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, left);
                AstNode **target = &left;

                while (tokeniser->last_returned.type == TokenType::ASSIGN) {
                        (*target)->adjacent_child = assign->assignment.expression;

                        assign = parse_assignment_or_declaration(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena, left);
                }

                return assign;

        } break;

        case TokenType::MULTIPLICATION: {
                AstNode *left = parse_star_targets(tokeniser, ast_arena, scope,
                                                   compiler_tables,
                                                   symbol_table_arena);

                if (tokeniser->last_returned.type == TokenType::COLON ||
                    tokeniser->last_returned.type == TokenType::ASSIGN) {
                        return parse_assignment_or_declaration(tokeniser, ast_arena, scope,
                                                compiler_tables,
                                                symbol_table_arena, left);
                }

                return parse_increasing_precedence(tokeniser, ast_arena, left,
                                                   scope, compiler_tables,
                                                   symbol_table_arena, 0);
        }

        case TokenType::IMPORT: {
                node->type = AstNodeType::IMPORT;
                tokeniser->next_token();
                // NOTE: must parse at least one
                node->import.children = parse_comma_seperated_func(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, parse_dotted_as_name_import);

                if (!node->import.children) {
                        parser_exit_with_error_msg(
                                "Import statement must have a target",
                                tokeniser);
                }

        } break;
                //TODO relative imports
        case TokenType::FROM: {
                node->type = AstNodeType::FROM;
                AstNodeFrom *from = &node->from;
                tokeniser->next_token();
                from->dotted_name = parse_dotted_name(tokeniser, ast_arena,
                                                      scope, compiler_tables,
                                                      symbol_table_arena);

                assert_token_and_print_debug(TokenType::IMPORT,
                                             tokeniser->last_returned,
                                             "Require an import after from");
                tokeniser->next_token();

                bool paren = false;
                if (tokeniser->last_returned.type ==
                    TokenType::MULTIPLICATION) {
                        from->is_wildcard = true;
                        tokeniser->next_token();

                        assert_token_and_print_debug(
                                TokenType::NEWLINE, tokeniser->last_returned,
                                "Cannot have a wildcard and other named targets");

                        return node;
                }

                if (tokeniser->last_returned.type == TokenType::OPEN_PAREN) {
                        tokeniser->next_token();
                        paren = true;
                }

                if (paren) {
                        AstNode *child = parse_single_import_from_as_name(
                                tokeniser, ast_arena);
                        from->targets = child;
                        while (tokeniser->last_returned.type ==
                               TokenType::COMMA) {
                                tokeniser->next_token();
                                eat_newlines(tokeniser);
                                child->adjacent_child =
                                        parse_single_import_from_as_name(
                                                tokeniser, ast_arena);
                                child = child->adjacent_child;
                                eat_newlines(tokeniser);
                        }

                        assert_token_and_print_debug(TokenType::CLOSED_PAREN,
                                                     tokeniser->last_returned,
                                                     "Mismatched Paren");
                        tokeniser->next_token();
                } else {
                        from->targets = parse_import_from_as_names(tokeniser,
                                                                   ast_arena);
                }

        } break;

        case TokenType::AT: {
                AstNode *head_decorator = nullptr;
                AstNode **decorator = &head_decorator;
                while (tokeniser->last_returned.type == TokenType::AT) {
                        tokeniser->next_token();
                        *decorator = parse_single_assignment_expression(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena);
                        assert_token_and_print_debug(
                                TokenType::NEWLINE, tokeniser->last_returned,
                                "expected newline after decarator declaration");

                        decorator = &((*decorator)->adjacent_child);
                        tokeniser->next_token();
                }

                if (tokeniser->last_returned.type == TokenType::CLASS) {
                        AstNode *class_def = parse_class_def(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena);
                        class_def->class_def.decarators = head_decorator;
                        return class_def;
                }

                if (tokeniser->last_returned.type == TokenType::DEF) {
                        AstNode *funcion_def = parse_function_def(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena);
                        funcion_def->function_def.decarators = head_decorator;
                        return funcion_def;
                }
        }

        case TokenType::IF: {
                tokeniser->next_token();
                node->type = AstNodeType::IF;
                AstNodeIf *if_stmt = &node->if_stmt;
                if_stmt->condition = parse_single_assignment_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena);
                assert_token_and_print_debug(TokenType::COLON,
                                             tokeniser->last_returned,
                                             "\n Error Parsing If Satement");
                tokeniser->next_token();

                if_stmt->block = parse_block(tokeniser, ast_arena, scope,
                                             compiler_tables,
                                             symbol_table_arena);
                if (!if_stmt->block) {
                        printf("Failed to parse block inside if on line: %d\n",
                               current_token.line);
                        exit(1);
                }

                if_stmt->or_else = parse_elif(tokeniser, ast_arena, scope,
                                              compiler_tables,
                                              symbol_table_arena);

        } break;

        case TokenType::WHILE: {
                node->type = AstNodeType::WHILE;
                AstNodeWhile *while_node = &node->while_loop;
                tokeniser->next_token();
                while_node->condition = parse_single_assignment_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena);

                assert_token_and_print_debug(TokenType::COLON,
                                             tokeniser->last_returned,
                                             "Error parsing while statment");

                tokeniser->next_token();
                while_node->block = parse_block(tokeniser, ast_arena, scope,
                                                compiler_tables,
                                                symbol_table_arena);
                while_node->or_else = parse_else(tokeniser, ast_arena, scope,
                                                 compiler_tables,
                                                 symbol_table_arena);
        } break;

        //TODO: find out what the hell a star_target is in the python grammar
        case TokenType::FOR: {
                node->type = AstNodeType::FOR_LOOP;
                node->token = tokeniser->last_returned;
                AstNodeForLoop *for_node = &node->for_loop;
                tokeniser->next_token();

                for_node->targets = parse_star_targets(tokeniser, ast_arena,
                                                       scope, compiler_tables,
                                                       symbol_table_arena);

                assert_token_and_print_debug(TokenType::IN_TOK,
                                             tokeniser->last_returned,
                                             "Error parsing for statment");

                tokeniser->next_token();
                for_node->expression = parse_star_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena);

                assert_token_and_print_debug(
                        TokenType::COLON, tokeniser->last_returned,
                        "expected colon after the end of for statement");

                tokeniser->next_token();
                for_node->block = parse_block(tokeniser, ast_arena, scope,
                                              compiler_tables,
                                              symbol_table_arena);
                for_node->or_else = parse_else(tokeniser, ast_arena, scope,
                                               compiler_tables,
                                               symbol_table_arena);
        } break;

        case TokenType::TRY: {
                node->type = AstNodeType::TRY;
                AstNodeTry *try_node = &node->try_node;

                assert_token_and_print_debug(
                        TokenType::COLON, tokeniser->next_token(),
                        "Expected colon at the end of try statment");

                tokeniser->next_token();
                try_node->block = parse_block(tokeniser, ast_arena, scope,
                                              compiler_tables,
                                              symbol_table_arena);

                // Parse except handlers
                AstNode **handler = &try_node->handlers;
                while (tokeniser->last_returned.type != TokenType::ENDFILE) {
                        Token except_token = tokeniser->last_returned;

                        if (!(except_token.type == TokenType::EXCEPT)) {
                                break;
                        }

                        AstNode *except = node_alloc(ast_arena);
                        except->token = except_token;
                        except->type = AstNodeType::EXCEPT;
                        AstNodeExcept *except_proper = &except->except;

                        if (!(tokeniser->next_token().type ==
                              TokenType::COLON)) {
                                except_proper->expression = parse_expression(
                                        tokeniser, ast_arena, scope,
                                        compiler_tables, symbol_table_arena, 0);

                                if (tokeniser->last_returned.type ==
                                    TokenType::AS) {
                                        tokeniser->next_token();
                                        parse_name(tokeniser, ast_arena);
                                }

                                assert_token_and_print_debug(
                                        TokenType::COLON,
                                        tokeniser->last_returned,
                                        "\n Error Parsing except satement");
                        }

                        tokeniser->next_token();
                        except_proper->block = parse_block(tokeniser, ast_arena,
                                                           scope,
                                                           compiler_tables,
                                                           symbol_table_arena);

                        *handler = except;
                        handler = &((*handler)->adjacent_child);
                }

                // parse else
                try_node->or_else = parse_else(tokeniser, ast_arena, scope,
                                               compiler_tables,
                                               symbol_table_arena);

                // parse finally statement
                if (tokeniser->last_returned.type == TokenType::FINALLY) {
                        assert_token_and_print_debug(
                                TokenType::COLON, tokeniser->next_token(),
                                "\n Error Parsing finally statement");

                        tokeniser->next_token();
                        try_node->finally = parse_block(tokeniser, ast_arena,
                                                        scope, compiler_tables,
                                                        symbol_table_arena);
                }

        } break;

        case TokenType::WITH: {
                AstNode *with_node = node_alloc(ast_arena);
                with_node->type = AstNodeType::WITH;
                with_node->token = tokeniser->last_returned;
                tokeniser->next_token();

                AstNode *child = parse_with_item(tokeniser, ast_arena, scope,
                                                 compiler_tables,
                                                 symbol_table_arena);

                with_node->with_statement.items = child;

                while (tokeniser->last_returned.type == TokenType::COMMA) {
                        eat_newlines(tokeniser);
                        child->adjacent_child = parse_with_item(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena);
                        child = child->adjacent_child;
                        eat_newlines(tokeniser);
                }
                assert_token_and_print_debug(
                        TokenType::COLON, tokeniser->last_returned,
                        "expected colon after end of with statemnt");

                tokeniser->next_token();
                with_node->with_statement.block =
                        parse_block(tokeniser, ast_arena, scope,
                                    compiler_tables, symbol_table_arena);

                return with_node;
        }

        case TokenType::DEF: {
                return parse_function_def(tokeniser, ast_arena, scope,
                                          compiler_tables, symbol_table_arena);
        }

        case TokenType::CLASS: {
                return parse_class_def(tokeniser, ast_arena, scope,
                                       compiler_tables, symbol_table_arena);
        }

        case TokenType::RETURN: {
                return parse_next_star_expressions_into_children(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, tokeniser->last_returned);
        }

        case TokenType::YIELD: {
                if (tokeniser->lookahead.type != TokenType::FROM) {
                        return parse_next_star_expressions_into_children(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena, tokeniser->last_returned);
                }

                tokeniser->next_token();
                tokeniser->next_token();

                node->type = AstNodeType::UNARY;
                node->unary.child = parse_expression(tokeniser, ast_arena,
                                                     scope, compiler_tables,
                                                     symbol_table_arena, 0);

                return node;
        }

        case TokenType::RAISE: {
                tokeniser->next_token();
                node->type = AstNodeType::RAISE;
                if (tokeniser->lookahead.type == TokenType::NEWLINE) {
                        return node;
                }

                tokeniser->next_token();
                node->raise.expression = parse_expression(
                        tokeniser, ast_arena, scope, compiler_tables,
                        symbol_table_arena, 0);

                if (tokeniser->last_returned.type == TokenType::FROM) {
                        tokeniser->next_token();
                        node->raise.from_expression = parse_expression(
                                tokeniser, ast_arena, scope, compiler_tables,
                                symbol_table_arena, 0);
                }

                return node;
        }

        case TokenType::DEL: {
                tokeniser->next_token();
                return parse_del_targets(tokeniser, ast_arena, scope,
                                         compiler_tables, symbol_table_arena);
        }

        case TokenType::MATCH: {
                node->type = AstNodeType::MATCH;
                if (tokeniser->lookahead.type == TokenType::MULTIPLICATION) {
                        node->match.subject =
                                parse_single_assignment_star_expression(
                                        tokeniser, ast_arena, scope,
                                        compiler_tables, symbol_table_arena);
                        node->match.subject->type = AstNodeType::NARY;

                        assert_token_and_print_debug(
                                TokenType::COMMA, tokeniser->last_returned,
                                "expected comma after star expresssion in match");

                        node->match.subject->adjacent_child =
                                parse_assignment_star_expressions(
                                        tokeniser, ast_arena, scope,
                                        compiler_tables, symbol_table_arena,
                                        true);
                } else {
                        node->match.subject =
                                parse_single_assignment_expression(
                                        tokeniser, ast_arena, scope,
                                        compiler_tables, symbol_table_arena);
                }

                assert_token_and_print_debug(
                        TokenType::COLON, tokeniser->last_returned,
                        "Expected colon in match statement");
        }

        case TokenType::GLOBAL: {
                return parse_list_of_names_and_return_modifier(
                        tokeniser, ast_arena, current_token);
        }

        case TokenType::NONLOCAL: {
                return parse_list_of_names_and_return_modifier(
                        tokeniser, ast_arena, current_token);
        }

        case TokenType::PASS: {
                return parse_single_token_into_node(tokeniser, ast_arena);
        }

        case TokenType::CONTINUE: {
                return parse_single_token_into_node(tokeniser, ast_arena);
        }

        case TokenType::BREAK: {
                return parse_single_token_into_node(tokeniser, ast_arena);
        }

        default:
                return parse_star_expression(tokeniser, ast_arena, scope,
                                             compiler_tables,
                                             symbol_table_arena);
        }

        return node;
}

static AstNode *parse_statements(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope,
                                 CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena)
{
        AstNode *file_node = node_alloc(ast_arena);
        file_node->token = tokeniser->last_returned;
        file_node->type = AstNodeType::FILE;
        AstNode **child = &file_node->file.children;

        while (tokeniser->last_returned.type != TokenType::ENDFILE) {
                eat_newlines(tokeniser);
                if (tokeniser->last_returned.type == TokenType::ENDFILE) {
                        break;
                }
                AstNode *statement = parse_statement(tokeniser, ast_arena,
                                                     scope, compiler_tables,
                                                     symbol_table_arena);
                if (!statement) {
                        printf("PARSING ERROR\n");
                        //debug_print_parse_tree(&file_node, 0);
                        break;
                }

                *child = statement;
                child = &((*child)->adjacent_child);
        }

        return file_node;
}
