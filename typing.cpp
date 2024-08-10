#include <stdio.h>
#include "typing.h"


// TODO: GENERICS
// TODO: Figure out how to store uninon types
// wether or not to cap them to a maximun number of types or to allocate dynamically

static void fail_typing_with_debug(AstNode *node) {
    fprintf(stderr, "Invalid Types\n");
    debug_print_parse_tree(node, 0);
    exit(1);
}

static inline void scope_stack_push(SubArena *scope_stack, SymbolTableEntry *value) {
    SymbolTableEntry **new_scope =
        (SymbolTableEntry **)scope_stack->alloc(sizeof(SymbolTableEntry *));
    *new_scope = value;
}

static inline void scope_stack_pop(Arena *scope_stack) {
    scope_stack->offset -= sizeof(SymbolTableEntry *);
}

static inline SymbolTableEntry *scope_stack_peek(Arena *scope_stack) {
    return *((SymbolTableEntry **)(((char *)scope_stack->memory) +
                                    scope_stack->offset -
                                   sizeof(SymbolTableEntry *)));
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
static bool find_symbol_definition_and_type(SubArena *scope_stack, AstNode *node, SymbolTable *symbol_table) {
    SymbolTableEntry *result = nullptr;

    for (int i = (scope_stack->offset / sizeof(SymbolTableEntry *)) - 1;
         i >= 0;
         --i) {

        assert(scope_stack->offset % sizeof(SymbolTableEntry *) == 0);
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

static int type_parse_tree(AstNode *node, Arena *parse_arena, SubArena *scope_stack,
                           CompilerTables *compiler_tables,
                           SymbolTable *table_to_look) {
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
            find_symbol_definition_and_type(scope_stack, node, table_to_look);
        }

    } break;

    case AstNodeType::TYPE_ANNOTATION: {
        type_parse_tree(node->type_annotation.name, parse_arena, scope_stack, compiler_tables,
                        compiler_tables->class_table);

        AstNode *parameter = node->type_annotation.parameters;
        while(parameter) {
            type_parse_tree(node->type_annotation.parameters, parse_arena, scope_stack,
                            compiler_tables, compiler_tables->class_table);
            parameter = parameter->adjacent_child;
        }

        node->static_type = node->type_annotation.name->static_type;
    } break;

    case AstNodeType::UNARY: {
        AstNode *child = node->unary.child;
        type_parse_tree(child, parse_arena, scope_stack, compiler_tables, table_to_look);
        node->static_type = child->static_type;

        if (node->token.type == TokenType::RETURN) {
            return 1;
        }

    } break;

    case AstNodeType::BINARYEXPR: {
        AstNode *left = node->binary.left;
        AstNode *right = node->binary.right;
        type_parse_tree(left, parse_arena, scope_stack, compiler_tables, table_to_look);
        type_parse_tree(right, parse_arena, scope_stack, compiler_tables, table_to_look);

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
            type_parse_tree(child, parse_arena, scope_stack, compiler_tables, table_to_look);
            child = child->adjacent_child;
        }

        break;
    }

    case AstNodeType::FILE: {
        AstNode *child = node->file.children;
        while (child) {
            type_parse_tree(child, parse_arena, scope_stack, compiler_tables, table_to_look);
            child = child->adjacent_child;
        }
    } break;

    case AstNodeType::FUNCTION_DEF: {
        type_parse_tree(node->function_def.return_type, parse_arena, scope_stack,
                        compiler_tables, compiler_tables->class_table);

        SymbolTableEntry *new_scope = compiler_tables->function_table->lookup(
            node->function_def.name->token.value, scope_stack_peek(scope_stack));
        scope_stack_push(scope_stack, new_scope);

        type_parse_tree(node->function_def.block, parse_arena, scope_stack, compiler_tables, table_to_look);
        node->static_type = node->function_def.return_type->static_type;

        if (node->function_def.block->static_type.type != node->static_type.type) {

            fprintf_s(stderr,
                      "TypeError: line: %d, col: %d\nFunction definition block "
                      "must match "
                      "annotated return type in all paths\n",
                      node->token.line, node->token.column);
            return 0;
        }

        new_scope->value.static_type = node->function_def.return_type->static_type;

        scope_stack_pop(scope_stack);

    } break;

    case AstNodeType::TUPLE:
        break;
    case AstNodeType::DICT:
        break;
    case AstNodeType::DICTCOMP:
        break;

    case AstNodeType::LIST: {
        node->static_type.type = TypeInfoType::LIST;
        uint32_t previous_type_mask = 0x00000000;
        uint32_t union_type_mask = 0x00000000;
        AstNode *child = node->nary.children;

        TypeInfo list_item_type = {};
        TypeInfo **types_in_list = (TypeInfo **)parse_arena->alloc(sizeof(TypeInfo *[1000]));
        uint32_t custom_index = (uint32_t)TypeInfoType::CUSTOM;
        while (child) {
            type_parse_tree(child, parse_arena, scope_stack, compiler_tables,
                            table_to_look);

            if (child->static_type.type == TypeInfoType::CUSTOM) {
                types_in_list[custom_index++] = &child->static_type;
            }

            else if (child->static_type.type == TypeInfoType::UNION) {
                types_in_list[custom_index++] = &child->static_type;
            }

            else {
                types_in_list[(int)child->static_type.type] = &child->static_type;
                child = child->adjacent_child;
            }
        }

        for (int i = 0; i < custom_index)

        // free the space
        parse_arena->offset -= sizeof(types_in_list);

        child = node->nary.children;

        if (previous_type_mask) {
            list_item_tye.type = UNION;
            while(child) {
                uint32_t type_bit (0x00000000 | (1 << (int)child->static_type.type))
                uint32_t type = union_type_mask & type_bit;
                union_type_mask = union_type_mask ^ type_bit;
                if (type_bit) {
                    chid->static_type;
                }
            }
        }

        // fix children into correct union types according to the mask
    } break;

    case AstNodeType::LISTCOMP:
        break;
    case AstNodeType::ASSIGNMENT: {
        type_parse_tree(node->assignment.name, parse_arena, scope_stack, compiler_tables,
                        compiler_tables->variable_table);

        type_parse_tree(node->assignment.expression, parse_arena, scope_stack,
                        compiler_tables, table_to_look);

        if (node->assignment.expression->static_type.type !=
                node->assignment.name->static_type.type ||
            node->assignment.expression->static_type.custom_symbol !=
                node->assignment.name->static_type.custom_symbol) {

            fprintf(stderr, "Assignment value miscmatched types\n");
        }

        node->static_type = node->assignment.name->static_type;

    } break;

    case AstNodeType::BLOCK: {
    // Type is return statement if no return statemnt the type is None
        AstNode *child = node->file.children;
        node->static_type.type = TypeInfoType::UNKNOWN;
        int return_flag = 0;
        while (child) {
            return_flag = type_parse_tree(child, parse_arena, scope_stack, compiler_tables, table_to_look);
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
        type_parse_tree(node->declaration.annotation,
                        scope_stack,
                        compiler_tables, compiler_tables->class_table);

        type_parse_tree(node->declaration.expression, parse_arena, scope_stack, compiler_tables, table_to_look);

        if (node->declaration.expression) {
            if (node->declaration.annotation->static_type.type !=
                    node->declaration.expression->static_type.type ||

                node->declaration.annotation->static_type.custom_symbol !=
                    node->declaration.expression->static_type.custom_symbol) {

                fprintf(stderr, "TypeError: line: %d, col: %d, Declaration "
                                "expression must match annotated return type",
                        node->token.line, node->declaration.expression->token.column);
                exit(1);
            }
        }

        SymbolTableEntry *entry = compiler_tables->variable_table->lookup(
            node->declaration.name->token.value, scope_stack_peek(scope_stack));
        node->static_type = node->declaration.annotation->static_type;
        entry->value.static_type = node->static_type;
    } break;

    case AstNodeType::IF: {
        // NOTE: conditions can be any type
        // all objects types can be considered either truthy or falsy if their length evaluates to zero
        // or their __bool__ method returns false
        type_parse_tree(node->if_stmt.condition, parse_arena, scope_stack, compiler_tables, table_to_look);
        int return_flag = type_parse_tree(node->if_stmt.block, parse_arena, scope_stack, compiler_tables, table_to_look);

        node->static_type = node->if_stmt.block->static_type;
        type_parse_tree(node->if_stmt.or_else, parse_arena, scope_stack, compiler_tables, table_to_look);
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
        int return_flag = type_parse_tree(node->else_stmt.block, parse_arena, scope_stack, compiler_tables, table_to_look);
        node->static_type = node->else_stmt.block->static_type;

        return return_flag;
    } break;

    case AstNodeType::WHILE: {

        type_parse_tree(node->while_loop.condition, parse_arena, scope_stack, compiler_tables, table_to_look);

        int return_flag = type_parse_tree(node->while_loop.block, parse_arena, scope_stack, compiler_tables, table_to_look);
        type_parse_tree(node->while_loop.or_else, parse_arena, scope_stack, compiler_tables, table_to_look);

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
    case AstNodeType::CLASS_DEF: {
        SymbolTableEntry *new_scope = compiler_tables->class_table->lookup(
            node->class_def.name->token.value, scope_stack_peek(scope_stack));

        scope_stack_push(scope_stack, new_scope);

        type_parse_tree(node->class_def.arguments, parse_arena, scope_stack, compiler_tables, table_to_look);
        type_parse_tree(node->class_def.block, parse_arena, scope_stack, compiler_tables, table_to_look);

        scope_stack_pop(scope_stack);

    } break;

    case AstNodeType::FUNCTION_CALL: {
        // Find function in table
        type_parse_tree(node->function_call.expression, parse_arena, scope_stack,
                        compiler_tables, compiler_tables->function_table);

        type_parse_tree(node->function_call.args, parse_arena, scope_stack, compiler_tables, table_to_look);

        node->static_type = node->function_call.expression->static_type;

    } break;
    case AstNodeType::SUBSCRIPT:
        break;
    case AstNodeType::ATTRIBUTE_REF: {
        type_parse_tree(node->attribute_ref.name, parse_arena, scope_stack, compiler_tables,
                        compiler_tables->variable_table);

        // find symbol in class
        SymbolTableEntry *result = table_to_look->lookup(
            node->attribute_ref.attribute->token.value,
            node->attribute_ref.name->static_type.custom_symbol);

        if (!result) {
            fprintf(
                stderr,
                "Cannot resolve name %s in attribute reference for custom type",
                node->attribute_ref.attribute->token.value.c_str());
            exit(1);
        }

        node->attribute_ref.attribute->static_type = result->value.static_type;
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
    case AstNodeType::WITH:
        break;
    case AstNodeType::WITH_ITEM:
        break;
    }
     return 0;
}
