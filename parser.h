#ifndef PARSER_H_
#define PARSER_H_

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "tokeniser.h"
#include "utils.h"

enum class AstNodeType {
    FILE = 0,
    BINARYEXPR = 1,
    UNARY = 3,
    NARY = 4,
    TUPLE = 5,
    DICT = 6,
    DICTCOMP = 7,
    LIST = 8,
    LISTCOMP = 9,
    TERMINAL = 10,
    ASSIGNMENT = 11,
    BLOCK = 12,
    DECLARATION = 13,
    TYPE_ANNOTATION =14,
    IF = 15,
    ELSE = 16,
    WHILE = 17,
    FOR_LOOP = 18,
    FOR_IF = 19,
    FUNCTION_DEF = 20,
    CLASS_DEF = 21,
    FUNCTION_CALL = 22,
    SUBSCRIPT = 23,
    ATTRIBUTE_REF = 24,
    TRY = 25,
    WITH = 26,
    WITH_ITEM = 27,
    EXCEPT = 28,
    STARRED = 29,
    KVPAIR = 30,
    IMPORT = 31,
};

struct AstNode;

struct AstNodeUnary {
    AstNode *child;
};

struct AstNodeNary {
    AstNode *children;
};

struct AstNodeBinaryExpr {
    AstNode *left;
    AstNode *right;
};

struct AstNodeAssignment {
    AstNode *name;
    AstNode *expression;
};

struct AstNodeDeclaration: public AstNodeAssignment {
    AstNode *annotation;
};

struct AstNodeTypeAnnot {
    AstNode *name;
    AstNode *parameters;
};

struct AstNodeIf {
    AstNode *condition;
    AstNode *block;
    AstNode *or_else;
};

struct AstNodeElse {
    AstNode *block;
};

struct AstNodeForLoop {
    AstNode *targets;
    AstNode *expression;
    AstNode *block;
    AstNode *or_else;
};

struct AstNodeForIfClause {
    AstNode *targets;
    AstNode *expression;
    AstNode *if_clause;
};

struct AstNodeWhile {
    AstNode *condition;
    AstNode *block;
    AstNode *or_else;
};

struct AstNodeClassDef {
    AstNode *decarators;
    AstNode *name;
    AstNode *arguments;
    AstNode *block;
};

struct AstNodeFunctionDef: public AstNodeClassDef {
    AstNode *star;
    AstNode *double_star;
    AstNode *return_type;
};

struct AstNodeFunctionCall {
    AstNode *expression;
    AstNode *args;
};

struct AstNodeKwarg {
    AstNode *name;
    AstNode *expression;
};

struct AstNodeKvPair {
    AstNode *key;
    AstNode *value;
};

struct AstNodeSubscript {
    AstNode *expression;
    union {
        struct {
            AstNode *start;
            AstNode *end;
            AstNode *step;
        };

        AstNode *children[3];
    };
};

struct AstNodeTry {
    AstNode *block;
    AstNode *handlers;
    AstNode *or_else;
    AstNode *finally;
};

struct AstNodeWithItem {
    AstNode *expression;
    AstNode *target;
};

struct AstNodeWith {
    AstNode *items;
    AstNode *block;
};


struct AstNodeExcept {
    AstNode *expression;
    AstNode *block;
};

struct AstNodeStarExpression {
    AstNode *expression;
};

struct AstNodeImport {
    AstNode *target;
    AstNode *as;
};

struct AstNodeAttributeRef {
    AstNode *name;
    AstNode *attribute;
};

struct AstNodeFile: public AstNodeNary {};
struct AstNodeBlock: public AstNodeNary {};
struct AstNodeTuple: public AstNodeNary {};

//
// ASTNode are tree nodes each node contains a linked list of its children
// each child node has a next pointer to the adjacent child of the same level
//
// REVIEW: Not too sure if i should have a specific variant for every single statement
// since some can be simplified into binary or nary however the union uses the space regardless so
// might aswell make it easier to work with for now
//

struct AstNode {
    Token token;
    AstNodeType type = AstNodeType::TERMINAL;
    TypeInfo static_type;

    union {
        AstNodeNary nary;
        AstNodeFile file;
        AstNodeBlock block;
        AstNodeBinaryExpr binary;
        AstNodeUnary unary;
        AstNodeDeclaration declaration;
        AstNodeAssignment assignment;
        AstNodeTypeAnnot type_annotation;
        AstNodeIf if_stmt;
        AstNodeElse else_stmt;
        AstNodeWhile while_loop;
        AstNodeForLoop for_loop;
        AstNodeForIfClause for_if;
        AstNodeFunctionDef function_def;
        AstNodeFunctionCall function_call;
        AstNodeClassDef class_def;
        AstNodeSubscript subscript;
        AstNodeAttributeRef attribute_ref;
        AstNodeTry try_node;
        AstNodeWithItem with_item;
        AstNodeWith with_statement;
        AstNodeExcept except;
        AstNodeStarExpression star_expression;
        AstNodeKwarg kwarg;
        AstNodeKvPair kvpair;
        AstNodeImport import;
        AstNodeTuple tuple;
    };

    AstNode *adjacent_child = nullptr;

    static AstNode create_terminal(Token token);
    static AstNode create_unary(Token token, AstNode *child);
    static AstNode create_binary(Token token, AstNode *left, AstNode *right);
};

static inline AstNode *parse_atom(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);
static inline AstNode *
parse_next_expression_into_child(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);
static inline AstNode *
parse_next_star_expression_into_child(Tokeniser *tokeniser, Arena *ast_arena,
                                      SymbolTableEntry *scope,
                                      CompilerTables *compiler_tables, Arena *symbol_table_arena);

static AstNode *parse_class_def(Tokeniser *tokeniser, Arena *ast_arena,
                                SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);
static AstNode *parse_function_def(Tokeniser *tokeniser, Arena *ast_arena,
                                   SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);
static inline AstNode *parse_left(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);
static AstNode *parse_dotted_name(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);
static AstNode *parse_block(Tokeniser *tokeniser, Arena *ast_arena,
                            SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);
static AstNode *parse_statements(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);
static AstNode *parse_statement(Tokeniser *tokeniser, Arena *ast_arena,
                                SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);
static AstNode *parse_star_expression(Tokeniser *tokeniser, Arena *ast_arena,
                                      SymbolTableEntry *scope,
                                      CompilerTables *compiler_tables, Arena *symbol_table_arena);

static AstNode *parse_inc_precedence_minimum_bitwise_or_precedence(
    Tokeniser *tokeniser, Arena *ast_arena, AstNode *left, SymbolTableEntry *scope,
    CompilerTables *compiler_tables, Arena *symbol_table_arena, int min_precedence);

static AstNode *parse_bitwise_or_minimum_precedence(
    Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
    CompilerTables *compiler_tables, Arena *symbol_table_arena, int min_precedence);

static AstNode *parse_expression(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope, CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena, int min_precedence);
static AstNode *parse_increasing_precedence(Tokeniser *tokeniser,
                                            Arena *ast_arena, AstNode *left,
                                            SymbolTableEntry *scope,
                                            CompilerTables *compiler_tables,
                                            Arena *symbol_table_arena,
                                            int min_precedence);
static AstNode *parse_assignment(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena, AstNode *left);
static AstNode *parse_assignment_expression(Tokeniser *tokeniser, Arena *ast_arena,
                                            SymbolTableEntry *scope,
                                            CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static AstNode *
parse_single_assignment_star_expression(Tokeniser *tokeniser, Arena *ast_arena,
                                        SymbolTableEntry *scope,
                                        CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static AstNode *parse_single_double_starred_kvpair(Tokeniser *tokeniser,
                                                   Arena *ast_arena,
                                                   SymbolTableEntry *scope,
                                                   CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static AstNode *parse_double_starred_kvpairs(Tokeniser *tokeniser, Arena *ast_arena,
                                             SymbolTableEntry *scope,
                                             CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static AstNode *parse_assignment_star_expressions(Tokeniser *tokeniser,
                                                  Arena *ast_arena,
                                                  SymbolTableEntry *scope,
                                                  CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static AstNode *parse_primary(Tokeniser *tokeniser, Arena *ast_arena,
                              SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static AstNode *parse_sub_primary(Tokeniser *tokeniser, Arena *ast_arena,
                                  AstNode *left, SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static AstNode *parse_elif(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
                           CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static AstNode *parse_else(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
                           CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static AstNode *parse_star_target(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static inline void assert_comma_and_skip_over(Tokeniser *tokeniser);
static AstNode *parse_function_def_arguments(Tokeniser *tokeniser, Arena *ast_arena,
                                             AstNodeFunctionDef *function,
                                             SymbolTableEntry *scope,
                                             CompilerTables *compiler_tables, Arena *symbol_table_arena);;
static AstNode *parse_function_call_arguments(Tokeniser *tokeniser,
                                              Arena *ast_arena, SymbolTableEntry *scope,
                                              CompilerTables *compiler_tables, Arena *symbol_table_arena);;

static inline bool expect_token(enum TokenType type, Token *token,
                                char *message);
#endif // PARSER_H_
