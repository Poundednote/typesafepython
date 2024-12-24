#ifndef PARSER_H_
#define PARSER_H_

#include <stdint.h>

#include "main.h"
#include "tokeniser.h"
#include "utils.h"
#include "typing.h"


struct AstNode;
struct SymbolTableEntry;
struct ParseResult;
struct Parser;

typedef ParseResult (*ParseSingleFunc)(Parser *parser);

introspect enum class AstNodeType {
        FILE = 1,
        BINARYEXPR = 2,
        UNARY = 3,
        NARY = 4,
        TUPLE = 5,
        DICT = 6,
        DICTCOMP = 7,
        LIST = 8,
        LISTCOMP = 9,
        TERMINAL = 10,
        IDENTIFIER = 11,
        ASSIGNMENT = 12,
        BLOCK = 13,
        DECLARATION = 14,
        TYPE_ANNOTATION = 15,
        IF = 16,
        ELSE = 17,
        WHILE = 18,
        FOR_LOOP = 19,
        FOR_IF = 20,
        FUNCTION_DEF = 21,
        CLASS_DEF = 22,
        FUNCTION_CALL = 23,
        SUBSCRIPT = 24,
        SLICE = 25,
        ATTRIBUTE_REF = 26,
        TRY = 27,
        WITH = 28,
        WITH_ITEM = 29,
        EXCEPT = 30,
        STARRED = 31,
        KVPAIR = 32,
        IMPORT = 33,
        IMPORT_TARGET = 34,
        FROM = 35,
        FROM_TARGET = 36,
        UNION = 37,
        MATCH = 38,
        RAISE = 39,
        IF_EXPR = 40,
        GEN_EXPR = 41,
        LAMBDA = 42,
        TYPE_PARAM = 43,
        INVALID = 44,
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
        AstNode *left;
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

introspect struct AstNodeTypeParam {
        AstNode *name;
        AstNode *bound;
        bool star;
        bool double_star;
};

introspect struct AstNodeClassDef {
        AstNode *decarators;
        AstNode *name;
        AstNode *type_params;
        AstNode *arguments;
        AstNode *block;
};

introspect struct AstNodeFunctionDef {
        AstNode *decarators;
        AstNode *name;
        AstNode *type_params;
        AstNode *arguments;
        AstNode *block;
        AstNode *star;
        AstNode *double_star;
        AstNode *return_type;
        int star_pos;
        int slash_pos;
};

introspect struct AstNodeLambdaDef {
        AstNode *arguments;
        AstNode *expression;
        AstNode *star;
        AstNode *double_star;
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
        AstNode *slices;

};

introspect struct AstNodeSlice {
        AstNode *start;
        AstNode *end;
        AstNode *step;
        AstNode *named_expr;
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
                AstNodeSlice slice;
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
                AstNodeLambdaDef lambda;
                AstNodeTypeParam type_param;
        };

        AstNode *adjacent_child = nullptr;

        static AstNode create_terminal(Token token);
        static AstNode create_unary(Token token, AstNode *child);
        static AstNode create_binary(Token token, AstNode *left,
                                     AstNode *right);
};

introspect enum class ParseErrorType {
        NONE = 0,
        INVALID_SYNTAX = 2,
        GENERAL = 3,
};

struct ParseError {
        ParseErrorType type;
        Token *token;
        const char *msg;
};

struct ParseResult {
        AstNode *node; 
        ParseError error;
};

struct Parser {
        TokenArray *token_arr;
        Arena *ast_arena;
        SymbolTableEntry *scope;
        Tables *tables;
        Arena *symbol_table_arena;
};

static AstNode *node_alloc(Arena *ast_arena);
static ParseResult parse_atom(Parser *parser, bool add_to_symbol_table);
static ParseResult parse_next_expression_into_child(Parser *parser);
static ParseResult
parse_next_star_expression_into_child(Parser *parser);

static ParseResult parse_class_def(Parser *parser);
static ParseResult parse_function_def(Parser *parser);
static inline ParseResult parse_left(Parser *parser);
static ParseResult parse_dotted_name(Parser *parser);
static ParseResult parse_block(Parser *parser);
static ParseResult parse_statements(Parser *parser);
static ParseResult parse_statement(Parser *parser);
static ParseResult parse_star_expression(Parser *parser);

static ParseResult parse_inc_precedence_minimum_bitwise_or_precedence(
Parser *parser);

static ParseResult parse_bitwise_or_minimum_precedence(Parser *parser, 
                                                       int min_precedence);

static ParseResult parse_expression(Parser *parser, int min_precedence);
static ParseResult parse_disjunction(Parser *parser, int min_precedence);
static ParseResult parse_increasing_precedence(Parser *parser);
static ParseResult 
parse_assignment_or_declaration(Parser *parser, AstNode *left);
static ParseResult parse_single_assignment_expression(Parser *parser);

static ParseResult 
parse_single_assignment_star_expression(Parser *parser);

static ParseResult parse_single_double_starred_kvpair(Parser *parser);
static ParseResult parse_double_starred_kvpairs(Parser *parser);
static ParseResult parse_assignment_star_expressions(Parser *parser, 
                                                     bool wrap_in_tuple);
static ParseResult parse_primary(Parser *parser);

static ParseResult parse_sub_primary(Parser *parser, AstNode *left, bool add_to_symbol_table);
static ParseResult parse_elif(Parser *parser);
static ParseResult parse_else(Parser *parser);
static ParseResult parse_star_target(Parser *parser);

static ParseResult assert_comma_and_skip_over(Parser *parser);
static ParseResult assert_token_and_print_debug(Parser *parser);
static ParseResult parse_function_def_arguments(Parser *parser, AstNodeFunctionDef *function);
static ParseResult parse_function_call_arguments(Parser *parser);
static ParseResult parse_gen_expr_from_first_child(Parser *parser, AstNode *first_child);

static ParseResult parse_single_star_target(Parser *parser,
                                            bool add_to_symbol_table = false);

static ParseResult parse_type_expression(Parser *parser);
static ParseResult parse_type_annotation(Parser *parser);
static ParseResult parse_single_subscript_attribute_primary(Parser *parser, 
                                                            bool add_to_symbol_table = false);
static ParseResult parse_name(Parser *parser, bool add_to_symbol_table = false);
void handle_errors_and_assert_end(Parser *parser, ParseResult *result);
void token_array_goto_end_of_statement(TokenArray *token_array);

static inline bool expect_token(enum TokenType type, Token *token,
                                char *message);
#endif // PARSER_H_
