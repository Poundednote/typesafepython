#ifndef PARSER_H_
#define PARSER_H_

#include <stdint.h>

#include "tpython.h"
#include "tokeniser.h"
#include "utils.h"
#include "typing.h"


struct AstNode;
struct SymbolTableEntry;

typedef AstNode *(*ParseSingleFunc)(Tokeniser *, Arena *, SymbolTableEntry *,
                                    CompilerTables *, Arena *);

introspect enum class AstNodeType {
        FILE = 0,
        BINARYEXPR = 1,
        UNARY = 2,
        NARY = 3,
        TUPLE = 4,
        DICT = 5,
        DICTCOMP = 6,
        LIST = 7,
        LISTCOMP = 8,
        TERMINAL = 9,
        ASSIGNMENT = 10,
        BLOCK = 11,
        DECLARATION = 12,
        TYPE_ANNOTATION = 13,
        IF = 14,
        ELSE = 15,
        WHILE = 16,
        FOR_LOOP = 17,
        FOR_IF = 18,
        FUNCTION_DEF = 19,
        CLASS_DEF = 20,
        FUNCTION_CALL = 21,
        SUBSCRIPT = 22,
        ATTRIBUTE_REF = 23,
        TRY = 24,
        WITH = 25,
        WITH_ITEM = 26,
        EXCEPT = 27,
        STARRED = 28,
        KVPAIR = 29,
        IMPORT = 30,
        IMPORT_TARGET = 31,
        FROM = 32,
        FROM_TARGET = 33,
        UNION = 34,
        MATCH = 35,
        RAISE = 36,
        IF_EXPR = 37,
        GEN_EXPR = 38,
};

introspect struct AstNodeUnary {
        AstNode *child;
};

introspect struct AstNodeNary {
        AstNode *children;
};

introspect struct AstNodeBinaryExpr {
        AstNode *left;
        AstNode *right;
};

introspect struct AstNodeAssignment {
        AstNode *name;
        AstNode *expression;
};

introspect struct AstNodeDeclaration {
        AstNode *name;
        AstNode *expression;
        AstNode *annotation;
};

introspect struct AstNodeTypeAnnot {
        AstNode *type;
        AstNode *parameters;
};

introspect struct AstNodeUnion {
        AstNode *left;
        AstNode *right;
};

introspect struct AstNodeIf {
        AstNode *condition;
        AstNode *block;
        AstNode *or_else;
};

introspect struct AstNodeIfExpr {
        AstNode *true_expression;
        AstNode *condition;
        AstNode *false_expression;
};

introspect struct AstNodeElse {
        AstNode *block;
};

introspect struct AstNodeForLoop {
        AstNode *targets;
        AstNode *expression;
        AstNode *block;
        AstNode *or_else;
};

introspect struct AstNodeForIfClause {
        AstNode *targets;
        AstNode *expression;
        AstNode *if_clause;
};

introspect struct AstNodeWhile {
        AstNode *condition;
        AstNode *block;
        AstNode *or_else;
};

introspect struct AstNodeClassDef {
        AstNode *decarators;
        AstNode *name;
        AstNode *arguments;
        AstNode *block;
};

introspect struct AstNodeFunctionDef {
        AstNode *decarators;
        AstNode *name;
        AstNode *arguments;
        AstNode *block;
        AstNode *star;
        AstNode *double_star;
        AstNode *return_type;
        bool has_star;
};

introspect struct AstNodeFunctionCall {
        AstNode *expression;
        AstNode *args;
};

introspect struct AstNodeKwarg {
        AstNode *name;
        AstNode *expression;
};

introspect struct AstNodeKvPair {
        AstNode *key;
        AstNode *value;
};

introspect struct AstNodeSubscript {
        AstNode *expression;
        AstNode *start;
        AstNode *end;
        AstNode *step;

};

introspect struct AstNodeTry {
        AstNode *block;
        AstNode *handlers;
        AstNode *or_else;
        AstNode *finally;
};

introspect struct AstNodeWithItem {
        AstNode *expression;
        AstNode *target;
};

introspect struct AstNodeWith {
        AstNode *items;
        AstNode *block;
};

introspect struct AstNodeExcept {
        AstNode *expression;
        AstNode *block;
};

introspect struct AstNodeStarExpression {
        AstNode *expression;
};

// import targets can have dotted names
// from targets cannot
introspect struct AstNodeImportTarget {
        AstNode *dotted_name;
        AstNode *as;
};

introspect struct AstNodeFromImportTarget {
        AstNode *name;
        AstNode *as;
};

introspect struct AstNodeRaise {
        AstNode *expression;
        AstNode *from_expression;
};

introspect struct AstNodeFrom {
        AstNode *dotted_name;
        AstNode *targets;
        bool is_wildcard;
};

introspect struct AstNodeMatch {
        AstNode *subject;
        AstNode *case_block;
};

introspect struct AstNodeAttributeRef {
        AstNode *name;
        AstNode *attribute;
};

introspect struct AstNodeFile {
        AstNode *children;
};
introspect struct AstNodeBlock {
        AstNode *children;
};
introspect struct AstNodeTuple {
        AstNode *children;
};

introspect struct AstNodeGenExpr {
        AstNode *expression;
        AstNode *for_if_clauses;
};

introspect struct AstNodeImport {
        AstNode *children;
};

introspect struct AstNodeList {
        AstNode *children;
};

introspect struct AstNodeDict {
        AstNode *children;
};

//
// ASTNode are tree nodes each node contains a linked list of its children
// each child node has a next pointer to the adjacent child of the same level
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
                AstNodeImportTarget import_target;
                AstNodeFrom from;
                AstNodeFromImportTarget from_target;
                AstNodeTuple tuple;
                AstNodeList list;
                AstNodeDict dict;
                AstNodeUnion union_type;
                AstNodeMatch match;
                AstNodeRaise raise;
                AstNodeIfExpr if_expr;
                AstNodeGenExpr gen_expr;
        };

        AstNode *adjacent_child = nullptr;

        static AstNode create_terminal(Token token);
        static AstNode create_unary(Token token, AstNode *child);
        static AstNode create_binary(Token token, AstNode *left,
                                     AstNode *right);
};

static inline AstNode *parse_atom(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena);
static inline AstNode *parse_next_expression_into_child(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena);
static inline AstNode *parse_next_star_expression_into_child(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena);

static AstNode *parse_class_def(Tokeniser *tokeniser, Arena *ast_arena,
                                SymbolTableEntry *scope,
                                CompilerTables *compiler_tables,
                                Arena *symbol_table_arena);
static AstNode *parse_function_def(Tokeniser *tokeniser, Arena *ast_arena,
                                   SymbolTableEntry *scope,
                                   CompilerTables *compiler_tables,
                                   Arena *symbol_table_arena);
static inline AstNode *parse_left(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena);
static AstNode *parse_dotted_name(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena);
static AstNode *parse_block(Tokeniser *tokeniser, Arena *ast_arena,
                            SymbolTableEntry *scope,
                            CompilerTables *compiler_tables,
                            Arena *symbol_table_arena);
static AstNode *parse_statements(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope,
                                 CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena);
static AstNode *parse_statement(Tokeniser *tokeniser, Arena *ast_arena,
                                SymbolTableEntry *scope,
                                CompilerTables *compiler_tables,
                                Arena *symbol_table_arena);
static AstNode *parse_star_expression(Tokeniser *tokeniser, Arena *ast_arena,
                                      SymbolTableEntry *scope,
                                      CompilerTables *compiler_tables,
                                      Arena *symbol_table_arena);

static AstNode *parse_inc_precedence_minimum_bitwise_or_precedence(
        Tokeniser *tokeniser, Arena *ast_arena, AstNode *left,
        SymbolTableEntry *scope, CompilerTables *compiler_tables,
        Arena *symbol_table_arena, int min_precedence);

static AstNode *parse_bitwise_or_minimum_precedence(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena,
        int min_precedence);

static AstNode *parse_expression(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope,
                                 CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena, int min_precedence);
static AstNode *parse_disjunction(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope,
                                 CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena, int min_precedence);
static AstNode *parse_increasing_precedence(Tokeniser *tokeniser,
                                            Arena *ast_arena, AstNode *left,
                                            SymbolTableEntry *scope,
                                            CompilerTables *compiler_tables,
                                            Arena *symbol_table_arena,
                                            int min_precedence);
static AstNode *parse_assignment_or_declaration(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope,
                                 CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena, AstNode *left);
static AstNode *parse_single_assignment_expression(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena);

static AstNode *parse_single_assignment_star_expression(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena);

static AstNode *parse_single_double_starred_kvpair(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena);
static AstNode *parse_double_starred_kvpairs(Tokeniser *tokeniser,
                                             Arena *ast_arena,
                                             SymbolTableEntry *scope,
                                             CompilerTables *compiler_tables,
                                             Arena *symbol_table_arena);
static AstNode *parse_assignment_star_expressions(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena,
        bool wrap_in_tuple);
static AstNode *parse_primary(Tokeniser *tokeniser, Arena *ast_arena,
                              SymbolTableEntry *scope,
                              CompilerTables *compiler_tables,
                              Arena *symbol_table_arena);
static AstNode *parse_sub_primary(Tokeniser *tokeniser, Arena *ast_arena,
                                  AstNode *left, SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena);
static AstNode *parse_elif(Tokeniser *tokeniser, Arena *ast_arena,
                           SymbolTableEntry *scope,
                           CompilerTables *compiler_tables,
                           Arena *symbol_table_arena);
static AstNode *parse_else(Tokeniser *tokeniser, Arena *ast_arena,
                           SymbolTableEntry *scope,
                           CompilerTables *compiler_tables,
                           Arena *symbol_table_arena);
static AstNode *parse_star_target(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena);

static inline void assert_comma_and_skip_over(Tokeniser *tokeniser);
static AstNode *parse_function_def_arguments(Tokeniser *tokeniser,
                                             Arena *ast_arena,
                                             AstNodeFunctionDef *function,
                                             SymbolTableEntry *scope,
                                             CompilerTables *compiler_tables,
                                             Arena *symbol_table_arena);
static AstNode *parse_function_call_arguments(Tokeniser *tokeniser,
                                              Arena *ast_arena,
                                              SymbolTableEntry *scope,
                                              CompilerTables *compiler_tables,
                                              Arena *symbol_table_arena);
static AstNode *parse_gen_expr_from_first_child(Tokeniser *tokeniser,
                                                Arena *ast_arena,
                                                SymbolTableEntry *scope,
                                                CompilerTables *compiler_tables,
                                                Arena *symbol_table_arena,
                                                AstNode *first_child);

static AstNode *parse_type_expression(Tokeniser *tokeniser, Arena *ast_arena);
static AstNode *parse_type_annotation(Tokeniser *tokeniser, Arena *ast_arena);
static AstNode *parse_single_subscript_attribute_primary(
        Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope,
        CompilerTables *compiler_tables, Arena *symbol_table_arena);

static inline bool expect_token(enum TokenType type, Token *token,
                                char *message);
#endif // PARSER_H_
