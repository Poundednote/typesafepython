#include <stdio.h>
#include "typing.h"



static void fail_typing_with_debug(AstNode *node) {
    fprintf(stderr, "Invalid Types\n");
    debug_print_parse_tree(node, 0);
    exit(1);
}

static void scope_stack_push(Arena *scope_stack, SymbolTableEntry *value) {
    SymbolTableEntry **new_scope =
        (SymbolTableEntry **)scope_stack->alloc(sizeof(SymbolTableEntry *));
    *new_scope = value;
}

static inline void scope_stack_pop(Arena *scope_stack) {
    scope_stack->offset -= sizeof(SymbolTableEntry *);
}

static bool is_num_type(TypeInfo type_info) {
    switch (type_info.type) {

    case TypeInfoType::FLOAT:
        return true;

    case TypeInfoType::INTEGER:
        return true;

    default:
        return false;
    }
}

//TODO implement for debugging
/*
static bool recreate_binary_expresison_from_root(AstNode *root) {
    recreate_statement_from_root(AstNode *root)
}
*/
static bool find_symbol_definition_and_type(Arena *scope_stack, AstNode *node, SymbolTable *symbol_table) {
    SymbolTableEntry *result = symbol_table->lookup(
        node->token.value, node->scope);

    for (int i = scope_stack->offset; i >= 0; i -= (sizeof(SymbolTableEntry *))) {
        if (result) {
          break;
        }

        result = symbol_table->lookup(
            node->token.value,
            ((SymbolTableEntry **)(scope_stack->memory))[i]);
    }

    if (result) {
        node->static_type = result->value.static_type;
        return true;
    }

    else {
        return false;
    }
}

static int type_parse_tree(AstNode *node, Arena *scope_stack,
                           CompilerTables *compiler_tables) {
    if (!node) {
        return 0;
    }

    switch (node->type) {
    case AstNodeType::TERMINAL: {
        if (node->token.type == TokenType::INT_LIT) {
          node->static_type.type= TypeInfoType::INTEGER;
        }

        else if (node->token.type == TokenType::FLOAT_LIT) {
          node->static_type.type= TypeInfoType::FLOAT;
        }

        else if (node->token.type == TokenType::STRING_LIT) {
          node->static_type.type= TypeInfoType::STRING;
        }

        else if (node->token.type == TokenType::BOOL_TRUE ||
            node->token.type == TokenType::BOOL_FALSE)
          node->static_type.type = TypeInfoType::BOOLEAN;

        else if (node->token.type == TokenType::NONE) {
            node->static_type.type = TypeInfoType::NONE;
        }

        else if (node->token.type == TokenType::IDENTIFIER) {
            if (node->token.value == "int") {
                node->static_type.type = TypeInfoType::INTEGER;
            }

            else if (node->token.value == "float") {
                node->static_type.type = TypeInfoType::FLOAT;
            }

            else if (node->token.value == "str") {
                node->static_type.type = TypeInfoType::STRING;
            }

            else if (node->token.value == "bool") {
                node->static_type.type = TypeInfoType::BOOLEAN;
            }

            else if (node->token.value == "complex") {
                node->static_type.type = TypeInfoType::COMPLEX;
            }

            else if (node->token.value == "NoneType") {
                node->static_type.type = TypeInfoType::NONE;
            }

            else {
                // search for a typename first of all
                if (find_symbol_definition_and_type(
                        scope_stack, node, compiler_tables->class_table)) {
                    break;
                }

                find_symbol_definition_and_type(
                    scope_stack, node, compiler_tables->variable_table);
            }
        }
    } break;

    case AstNodeType::UNARY: {
        AstNode *child = node->unary.child;
        type_parse_tree(child, scope_stack, compiler_tables);
        node->static_type = child->static_type;

        if (node->token.type == TokenType::RETURN) {
            return 1;
        }

    } break;

    case AstNodeType::BINARYEXPR: {
        AstNode *left = node->binary.left;
        AstNode *right = node->binary.right;
        type_parse_tree(left, scope_stack, compiler_tables);
        type_parse_tree(right, scope_stack, compiler_tables);

        node->static_type.type = TypeInfoType::BOOLEAN;

        if (node->token.is_comparrison_op()) {
            if (left->static_type.type == right->static_type.type &&
                left->static_type.custom_symbol == right->static_type.custom_symbol) {

                return 0;
            }

            else if (is_num_type(left->static_type) &&
                     is_num_type(right->static_type)) {

                return 0;
            }

            fail_typing_with_debug(node);
        }

        switch (node->token.type) {

        case TokenType::FLOOR_DIV:
            if (left->static_type.type != TypeInfoType::INTEGER &&
                left->static_type.type != TypeInfoType::FLOAT) {

                fail_typing_with_debug(node);
            }

            else if (right->static_type.type != TypeInfoType::INTEGER &&
                     right->static_type.type != TypeInfoType::FLOAT) {

                fail_typing_with_debug(node);
            }

            node->static_type.type = TypeInfoType::INTEGER;
            break;

        case TokenType::DIVISION:
            if (left->static_type.type != TypeInfoType::INTEGER &&
                left->static_type.type != TypeInfoType::FLOAT) {

                fail_typing_with_debug(node);
            }

            else if (right->static_type.type != TypeInfoType::INTEGER &&
                     right->static_type.type != TypeInfoType::FLOAT) {

              fail_typing_with_debug(node);
            }

            node->static_type.type = TypeInfoType::FLOAT;
            break;

        default:
            if (left->static_type.type == right->static_type.type) {
                node->static_type.type = left->static_type.type;
            }

            else if (left->static_type.type == TypeInfoType::INTEGER) {
                if (right->static_type.type != TypeInfoType::FLOAT) {
                    fprintf(stderr, "Invalid types\n");
                    fail_typing_with_debug(node);
                }

                else {
                    node->static_type.type = TypeInfoType::FLOAT;
                }
            }

            else if (left->static_type.type == TypeInfoType::FLOAT) {
                if (right->static_type.type != TypeInfoType::INTEGER) {
                    fail_typing_with_debug(node);
                }

                else {
                    node->static_type.type = TypeInfoType::FLOAT;
                }
            }

            else {
                fprintf(stderr, "Parse Error: \n");
                fail_typing_with_debug(node);
            }
        }

    } break;

    case AstNodeType::NARY: {
        AstNode *child = node->nary.children;
        while (child) {
            type_parse_tree(child, scope_stack, compiler_tables);
            child = child->adjacent_child;
        }

        break;
    }

    case AstNodeType::FILE: {
        AstNode *child = node->file.children;
        while (child) {
            type_parse_tree(child, scope_stack, compiler_tables);
            child = child->adjacent_child;
        }
    } break;

    case AstNodeType::FUNCTION_DEF: {
        type_parse_tree(node->function_def.return_type, scope_stack, compiler_tables);

        scope_stack_push(scope_stack, node->function_def.block->scope);
        type_parse_tree(node->function_def.block, scope_stack, compiler_tables);
        node->static_type = node->function_def.return_type->static_type;

        if (node->function_def.block->static_type.type != node->static_type.type) {

            fprintf_s(stderr,
                      "TypeError: line: %d, col: %d\nFunction definition block "
                      "must match "
                      "annotated return type in all paths\n",
                      node->token.line, node->token.column);
            return 0;
        }

        SymbolTableEntry *entry = compiler_tables->function_table->lookup(node->function_def.name->token.value,
                             node->scope);
        entry->value.static_type = node->function_def.return_type->static_type;

        scope_stack_pop(scope_stack);

    } break;

    case AstNodeType::TUPLE:
        break;
    case AstNodeType::DICT:
        break;
    case AstNodeType::DICTCOMP:
        break;
    case AstNodeType::LIST:
        break;
    case AstNodeType::LISTCOMP:
        break;
    case AstNodeType::ASSIGNMENT:
        break;

    case AstNodeType::BLOCK: {
    // Type is return statement if no return statemnt the type is None
        AstNode *child = node->file.children;
        node->static_type.type = TypeInfoType::UNKNOWN;
        int return_flag = 0;
        while (child) {
            return_flag = type_parse_tree(child, scope_stack, compiler_tables);
            if (return_flag == 1) {
                if (node->static_type.type == TypeInfoType::UNKNOWN) {
                    node->static_type = child->static_type;
                }
                else {
                    if (node->static_type.type != child->static_type.type ||
                        node->static_type.custom_symbol != child->static_type.custom_symbol) {
                      fprintf(stderr,
                              "Must return the same type in all paths\n");
                    }
                }
            }

            child = child->adjacent_child;
        }

        return return_flag;

    } break;

    case AstNodeType::DECLARATION: {

        type_parse_tree(node->declaration.annotation, scope_stack, compiler_tables);
        type_parse_tree(node->declaration.expression, scope_stack, compiler_tables);

        if (node->declaration.expression) {
            if (node->declaration.annotation->static_type.type !=
                    node->declaration.expression->static_type.type ||

                node->declaration.annotation->static_type.custom_symbol !=
                    node->declaration.expression->static_type.custom_symbol) {

                fprintf(
                    stderr,
                    "Declaration expression must match annotated return type");
                exit(1);
            }
        }

        SymbolTableEntry *entry = compiler_tables->variable_table->lookup(node->declaration.name->token.value,
                             node->scope);
        entry->value.static_type = node->declaration.annotation->static_type;
    } break;

    case AstNodeType::IF: {
        // NOTE: conditions can be any type
        // all objects types can be considered either truthy or falsy if their length evaluates to zero
        // or their __bool__ method returns false
        type_parse_tree(node->if_stmt.condition, scope_stack, compiler_tables);
        int return_flag = type_parse_tree(node->if_stmt.block, scope_stack, compiler_tables);

        node->static_type = node->if_stmt.block->static_type;
        type_parse_tree(node->if_stmt.or_else, scope_stack, compiler_tables);
        if (node->if_stmt.or_else)  {
            if (node->static_type.type != node->if_stmt.or_else->static_type.type ||
                node->static_type.custom_symbol != node->if_stmt.or_else->static_type.custom_symbol) {

                fprintf(stderr,
                        "TypeError: line: %d col: %d in if statement branch"
                        ", all branches must have the same return type\n"
                        "'%s' on line: %d col %d has a mismatched type\n",
                        node->if_stmt.block->token.line,
                        node->if_stmt.block->token.column,
                        debug_token_type_to_string(
                            node->if_stmt.or_else->token.type)
                            .c_str(),
                        node->if_stmt.or_else->token.line,
                        node->if_stmt.or_else->token.column);

                exit(1);
            }
        }

        return return_flag;

    } break;

    case AstNodeType::ELSE: {
        int return_flag = type_parse_tree(node->else_stmt.block, scope_stack, compiler_tables);
        node->static_type = node->else_stmt.block->static_type;

        return return_flag;
    } break;

    case AstNodeType::WHILE: {

        type_parse_tree(node->while_loop.condition, scope_stack, compiler_tables);

        int return_flag = type_parse_tree(node->while_loop.block, scope_stack, compiler_tables);
        type_parse_tree(node->while_loop.or_else, scope_stack, compiler_tables);

        if (node->while_loop.or_else) {
            if (node->static_type.type != node->while_loop.or_else->static_type.type ||
                node->static_type.custom_symbol != node->while_loop.or_else->static_type.custom_symbol) {

                fprintf(
                    stderr,
                    "TypeError: line: %d col: %d in if statement branch"
                    ", all branches must have the same return type\n"
                    "'%s' on line: %d col %d has a mismatched type\n",
                    node->while_loop.block->token.line,
                    node->while_loop.block->token.column,
                    debug_token_type_to_string(node->while_loop.or_else->token.type).c_str(),
                    node->while_loop.or_else->token.line,
                    node->while_loop.or_else->token.column);

                exit(1);
            }
        }

        return return_flag;
    } break;

    case AstNodeType::FOR_LOOP:
        break;
    case AstNodeType::FOR_IF:
        break;
    case AstNodeType::CLASS_DEF:
        break;
    case AstNodeType::FUNCTION_CALL:
        break;
    case AstNodeType::SUBSCRIPT:
        break;
    case AstNodeType::ATTRIBUTE_REF: {
        type_parse_tree(node->attribute_ref.name, scope_stack, compiler_tables);
        //find symbol in class

        SymbolTableEntry *result = compiler_tables->class_table->lookup(node->attribute_ref.attribute->token.value,
                                             node->attribute_ref.name->static_type.custom_symbol);

        if (!result) {
            fprintf(
                stderr,
                "Cannot resolve name %s in attribute reference for custom type",
                node->attribute_ref.attribute->token.value.c_str());
            exit(1);
        }

        node->static_type = node->attribute_ref.attribute->static_type;
    } break;
    case AstNodeType::TRY:
        break;
    case AstNodeType::EXCEPT:
        break;
    case AstNodeType::STARRED:
        break;
    case AstNodeType::KVPAIR:
        break;
    case AstNodeType::IMPORT:
        break;

    }
     return 0;
}
