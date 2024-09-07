#include <stdint.h>
#include <stdio.h>

#include "tpython.h"
#include "typing.h"
#include "parser.h"
#include "tables.h"
#include "debug.h"


// TODO: GENERICS
// TODO: COMMENTS IN PYTHON
// TODO: inheritence
// TODO: abstract classes / interfaces
// TODO strict initilisation e.g. classes must be initilised before use variables too
static void fail_typing_with_debug(AstNode *node)
{
        fprintf(stderr, "Invalid Types\n");
        debug_print_parse_tree(node, 0);
        exit(1);
}

static inline void scope_stack_push(SubArena *scope_stack,
                                    SymbolTableEntry *value)
{
        SymbolTableEntry **new_scope = (SymbolTableEntry **)scope_stack->alloc(
                sizeof(SymbolTableEntry *));
        *new_scope = value;
}

static inline void scope_stack_pop(Arena *scope_stack)
{
        scope_stack->offset -= sizeof(SymbolTableEntry *);
}

static inline SymbolTableEntry *scope_stack_peek(Arena *scope_stack)
{
        return *((SymbolTableEntry **)(((char *)scope_stack->memory) +
                                       scope_stack->offset -
                                       sizeof(SymbolTableEntry *)));
}

static bool static_type_is_num(TypeInfo *type_info)
{
        switch (type_info->type) {
        case TypeInfoType::FLOAT:
                return true;
        case TypeInfoType::INTEGER:
                return true;
        default:
                return false;
        }
}

static bool static_type_is_parameterised(TypeInfo *type_info)
{
        if (type_info->type == TypeInfoType::CLASS)
                return false;
        if ((int)type_info->type >= (int)(TypeInfoType::LIST))
                return true;
        else
                return false;
}

// The order of arguments matters for union types
// an annotation of [int | str] shouldn't match a list of [1, 1.0, "hello"] but
// if the list is type is compared against the list the type will match the lists union type
static inline bool static_types_is_rhs_equal_lhs(TypeInfo *lhs, TypeInfo *rhs)
{
        if (lhs->type == TypeInfoType::ANY || rhs->type == TypeInfoType::ANY)
                return true;
        if (lhs->type != rhs->type)
                return false;

        if (lhs->type == TypeInfoType::CLASS) {
                if (lhs->custom_symbol == rhs->custom_symbol) {
                        return true;
                }

                AstNode *lhs_class_node = lhs->custom_symbol->value.node;
                AstNode *rhs_class_node = rhs->custom_symbol->value.node;
                assert(lhs_class_node->type == AstNodeType::CLASS_DEF);
                assert(rhs_class_node->type == AstNodeType::CLASS_DEF);

                // check the rhs inherits from the left or implements the left

                AstNode *rhs_arg = rhs_class_node->class_def.arguments;

                while (rhs_arg) {
                        assert(rhs_arg->static_type.type != TypeInfoType::UNKNOWN);
                        if (rhs_arg->static_type.type == TypeInfoType::CLASS &&
                            rhs_arg->static_type.custom_symbol ==
                                    lhs->custom_symbol)
                                return true;
                }

                return false;
        }

        if (lhs->type == TypeInfoType::UNION) {
                TypeInfo visited_types_in_union[100] = {};
                assert(lhs->union_type.left);
                assert(lhs->union_type.right);
                assert(rhs->union_type.left);
                assert(rhs->union_type.right);
                return unions_are_equal(lhs, rhs,
                                        visited_types_in_union, 100);
        }

        else if (lhs->type == TypeInfoType::LIST) {
                return static_types_is_rhs_equal_lhs(
                        lhs->list.item_type, rhs->list.item_type);
        }

        else if (lhs->type == TypeInfoType::DICT) {
                bool are_keys_same_type = static_types_is_rhs_equal_lhs(
                        lhs->dict.key_type, rhs->dict.key_type);
                bool are_vals_same_type = static_types_is_rhs_equal_lhs(
                        lhs->dict.val_type, rhs->dict.val_type);

                return are_keys_same_type && are_vals_same_type;
        }

        else {
                return true;
        }
}

// TODO bounds checks on lists
static inline bool find_types_in_list(TypeInfo *list, size_t list_size,
                                      TypeInfo *type)
{
        for (int i = 0; i < list_size; ++i) {
                if (static_types_is_rhs_equal_lhs(&list[i], type))
                        return true;
        }

        return false;
}

static void traverse_union_and_populate_list(TypeInfo *union_type,
                                             TypeInfo *visited_list,
                                             uint32_t *list_index)
{
        if (union_type->type != TypeInfoType::UNION) {
                visited_list[(*list_index)++] = *union_type;
                return;
        }

        if (union_type->union_type.left)
                traverse_union_and_populate_list(union_type->union_type.left,
                                                 visited_list, list_index);

        if (union_type->union_type.right)
                traverse_union_and_populate_list(union_type->union_type.right,
                                                 visited_list, list_index);
}

static bool compare_union_with_visited_list(TypeInfo *union_type,
                                            TypeInfo *visited_list,
                                            uint32_t list_size)
{
        if (union_type->type != TypeInfoType::UNION) {
                if (!find_types_in_list(visited_list, list_size, union_type)) {
                        return false;
                } else {
                        return true;
                }
        }

        if (!union_type->union_type.left) {
                return compare_union_with_visited_list(
                        union_type->union_type.right, visited_list, list_size);
        }

        if (!union_type->union_type.right) {
                return compare_union_with_visited_list(
                        union_type->union_type.left, visited_list, list_size);
        }

        return compare_union_with_visited_list(union_type->union_type.right,
                                               visited_list, list_size) &&
               compare_union_with_visited_list(union_type->union_type.left,
                                               visited_list, list_size);
}

// traverse the union on the lhs and make sure that there are no types on the
// rhs union that do not appear on the left
static bool unions_are_equal(TypeInfo *lhs, TypeInfo *rhs,
                             TypeInfo *visited_list, size_t list_size)
{
        uint32_t list_index = 0;
        traverse_union_and_populate_list(lhs, visited_list, &list_index);
        assert(list_index < list_size);
        return compare_union_with_visited_list(rhs, visited_list, list_index);
}

// Used get pointers to types that need to persist such as for the type inside a list
static TypeInfo *type_info_make_persistent(Arena *arena, CompilerTables *tables,
                                           TypeInfo *type)
{
        if (type->type == TypeInfoType::CLASS) {
                return &type->custom_symbol->value.static_type;
        } else if (static_type_is_parameterised(type)) {
                TypeInfo *result =
                        (TypeInfo *)arena->alloc(sizeof(*result));
                *result = *type;
                return result;
        } else {
                return &tables->builtin_types[(int)type->type];
        }
}

        //TODO implement for debugging
        /*
static bool recreate_binary_expresison_from_root(AstNode *root) {
    recreate_statement_from_root(AstNode *root)
}
*/

        // compares the two types and if they aren't equal it will overite the type_to_unionise ino a union type
        // and update the type_to_unionise to the right branch of the union
        // this function
static TypeInfo **generate_union_and_update_type_to_unionise(
        Arena *arena, CompilerTables *compiler_tables, TypeInfo *prev_type,
        TypeInfo *current_type, TypeInfo **type_to_unionise)
{
        if (!static_types_is_rhs_equal_lhs(prev_type, current_type)) {
                // alloc union
                TypeInfo *prev_unionised_type = *type_to_unionise;
                *type_to_unionise = (TypeInfo *)arena->alloc(sizeof(TypeInfo));
                (*type_to_unionise)->type = TypeInfoType::UNION;

                // if the old type_to_unionise is already a union then just update with old ptr
                // so we don't alloc parameterised types and other things twice
                if (prev_unionised_type &&
                    prev_unionised_type->type == TypeInfoType::UNION) {
                        (*type_to_unionise)->union_type.left =
                                prev_unionised_type->union_type.right;
                } else {
                        (*type_to_unionise)->union_type.left =
                                type_info_make_persistent(
                                        arena, compiler_tables, prev_type);
                }

                (*type_to_unionise)->union_type.right =
                        type_info_make_persistent(arena, compiler_tables,
                                                  current_type);

                // update the type_to_unionse_ptr
                type_to_unionise = &(*type_to_unionise)->union_type.right;
        }

        return type_to_unionise;
}

static bool find_symbol_definition_and_type(SubArena *scope_stack,
                                            AstNode *node,
                                            SymbolTable *symbol_table)
{
        SymbolTableEntry *result = nullptr;

        for (int i = (scope_stack->offset / sizeof(SymbolTableEntry *)) - 1;
             i >= 0; --i) {
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
        } else {
                return false;
        }
}

static int type_parse_tree(AstNode *node, Arena *parse_arena,
                           SubArena *scope_stack,
                           CompilerTables *compiler_tables,
                           SymbolTable *table_to_look)
{
        if (!node) {
                return 0;
        }

        switch (node->type) {
        case AstNodeType::TERMINAL: {
                if (node->token.type == TokenType::INT_LIT) {
                        node->static_type.type = TypeInfoType::INTEGER;
                } else if (node->token.type == TokenType::FLOAT_LIT) {
                        node->static_type.type = TypeInfoType::FLOAT;
                } else if (node->token.type == TokenType::STRING_LIT) {
                        node->static_type.type = TypeInfoType::STRING;
                }
                else if (node->token.type == TokenType::BOOL_TRUE ||
                         node->token.type == TokenType::BOOL_FALSE) {
                        node->static_type.type = TypeInfoType::BOOLEAN;
                }
                else if (node->token.type == TokenType::NONE) {
                        node->static_type.type = TypeInfoType::NONE;
                }
                else if (node->token.type == TokenType::IDENTIFIER) {
                        if (!find_symbol_definition_and_type(scope_stack, node,
                                                             table_to_look)) {
                                fprintf_s(stderr, "No valid identifier %s",
                                          node->token.value.c_str());
                                exit(1);
                        }
                }

        } break;

        case AstNodeType::TYPE_ANNOTATION: {
                type_parse_tree(node->type_annotation.type, parse_arena,
                                scope_stack, compiler_tables,
                                compiler_tables->class_table);

                AstNode *parameter = node->type_annotation.parameters;
                node->static_type = node->type_annotation.type->static_type;

                if (node->static_type.type == TypeInfoType::LIST) {
                        if (!parameter || parameter->adjacent_child) {
                                fprintf(stderr,
                                        "TypeError: List Type annotation takes exactly 1 type parameter");
                                exit(1);
                        }

                        type_parse_tree(node->type_annotation.parameters,
                                        parse_arena, scope_stack,
                                        compiler_tables,
                                        compiler_tables->class_table);

                        node->static_type.list.item_type =
                                &parameter->static_type;
                } else if (node->static_type.type == TypeInfoType::DICT) {
                        if (!parameter || !parameter->adjacent_child) {
                                fprintf(stderr,
                                        "TypeError: Dict Type annotation takes exactly 2 type parameters");
                                exit(1);
                        }

                        if (parameter->adjacent_child->adjacent_child) {
                                fprintf(stderr,
                                        "TypeError: Dict Type annotation takes exactly 2 type parameters");
                                exit(1);
                        }

                        AstNode *key_param = node->type_annotation.parameters;
                        AstNode *val_param =
                                node->type_annotation.parameters->adjacent_child;
                        type_parse_tree(key_param, parse_arena, scope_stack,
                                        compiler_tables,
                                        compiler_tables->class_table);
                        type_parse_tree(val_param, parse_arena, scope_stack,
                                        compiler_tables,
                                        compiler_tables->class_table);

                        node->static_type.dict.key_type =
                                &key_param->static_type;
                        node->static_type.dict.val_type =
                                &val_param->static_type;
                } else {
                        while (parameter) {
                                type_parse_tree(
                                        node->type_annotation.parameters,
                                        parse_arena, scope_stack,
                                        compiler_tables,
                                        compiler_tables->class_table);
                                parameter = parameter->adjacent_child;
                        }
                }

        } break;

        case AstNodeType::UNARY: {
                AstNode *child = node->unary.child;
                type_parse_tree(child, parse_arena, scope_stack,
                                compiler_tables, table_to_look);

                if (child)
                        node->static_type = child->static_type;
                else
                        node->static_type.type = TypeInfoType::NONE;

                if (node->token.type == TokenType::RETURN)
                        return 1;

        } break;

        case AstNodeType::BINARYEXPR: {
                AstNode *left = node->binary.left;
                AstNode *right = node->binary.right;
                type_parse_tree(left, parse_arena, scope_stack, compiler_tables,
                                table_to_look);
                type_parse_tree(right, parse_arena, scope_stack,
                                compiler_tables, table_to_look);

                node->static_type.type = TypeInfoType::BOOLEAN;

                if (node->token.is_comparrison_op()) {
                        if (static_types_is_rhs_equal_lhs(
                                    &left->static_type, &right->static_type)) {
                                return 0;
                        }

                        else if (static_type_is_num(&left->static_type) &&
                                 static_type_is_num(&right->static_type)) {
                                return 0;
                        }

                        fail_typing_with_debug(node);
                }

                switch (node->token.type) {
                case TokenType::FLOOR_DIV:
                        if (!static_type_is_num(&left->static_type) ||
                            !static_type_is_num(&right->static_type))
                                fail_typing_with_debug(node);

                        node->static_type.type = TypeInfoType::INTEGER;
                        break;

                case TokenType::DIVISION:
                        if (!static_type_is_num(&left->static_type) ||
                            !static_type_is_num(&right->static_type))
                                fail_typing_with_debug(node);

                        node->static_type.type = TypeInfoType::FLOAT;
                        break;

                default:
                        if (left->static_type.type == right->static_type.type) {
                                node->static_type = right->static_type;
                        } else if (static_type_is_num(&left->static_type) &&
                                   static_type_is_num(&right->static_type)) {
                                if (left->static_type.type ==
                                            TypeInfoType::FLOAT ||
                                    right->static_type.type ==
                                            TypeInfoType::FLOAT) {
                                        node->static_type.type =
                                                TypeInfoType::FLOAT;
                                } else {
                                        node->static_type.type =
                                                TypeInfoType::INTEGER;
                                }
                        }
                }

        } break;

        case AstNodeType::NARY: {
                AstNode *child = node->nary.children;
                while (child) {
                        type_parse_tree(child, parse_arena, scope_stack,
                                        compiler_tables, table_to_look);
                        child = child->adjacent_child;
                }

                break;
        }

        case AstNodeType::FILE: {
                AstNode *child = node->file.children;
                while (child) {
                        type_parse_tree(child, parse_arena, scope_stack,
                                        compiler_tables, table_to_look);
                        child = child->adjacent_child;
                }
        } break;

        case AstNodeType::FUNCTION_DEF: {
                SymbolTableEntry *function_symbol =
                        compiler_tables->function_table->lookup(
                                node->function_def.name->token.value,
                                scope_stack_peek(scope_stack));

                if (node->function_def.return_type) {
                        type_parse_tree(node->function_def.return_type, parse_arena,
                                        scope_stack, compiler_tables,
                                        compiler_tables->class_table);

                } else {
                        function_symbol->value.static_type.type = TypeInfoType::ANY;
                        function_symbol->value.static_type.custom_symbol = function_symbol;
                        return 0;
                }

                scope_stack_push(scope_stack, function_symbol);
                type_parse_tree(node->function_def.arguments, parse_arena,
                                scope_stack, compiler_tables, table_to_look);

                int return_flag = type_parse_tree(node->function_def.block, parse_arena,
                                scope_stack, compiler_tables, table_to_look);
                node->static_type.type = TypeInfoType::CLASS;
                node->static_type.custom_symbol = function_symbol;

                if (!static_types_is_rhs_equal_lhs(
                            &node->function_def.return_type->static_type,
                            &node->function_def.block->static_type)) {
                        fprintf_s(
                                stderr,
                                "TypeError: line: %d, col: %d\nFunction definition block "
                                "must match "
                                "annotated return type in all paths",
                                node->token.line, node->token.column);
                        exit(1);
                }

                function_symbol->value.static_type = node->function_def.return_type->static_type;
                function_symbol->value.static_type.custom_symbol = function_symbol;


                scope_stack_pop(scope_stack);
                return return_flag;

        } break;

        case AstNodeType::TUPLE:
                break;

        case AstNodeType::DICT: {
                node->static_type.type = TypeInfoType::DICT;
                AstNode *child = node->nary.children;
                TypeInfo **key_type_to_modify =
                        &node->static_type.dict.key_type;
                TypeInfo **val_type_to_modify =
                        &node->static_type.dict.val_type;
                // find the correct type for the collection
                type_parse_tree(child, parse_arena, scope_stack,
                                compiler_tables, table_to_look);

                TypeInfo *prev_key_type = child->static_type.kvpair.key_type;
                TypeInfo *prev_val_type = child->static_type.kvpair.val_type;

                *key_type_to_modify = child->static_type.kvpair.key_type;
                *val_type_to_modify = child->static_type.kvpair.val_type;
                child = child->adjacent_child;

                while (child) {
                        type_parse_tree(child, parse_arena, scope_stack,
                                        compiler_tables, table_to_look);

                        TypeInfo *child_key_type =
                                child->static_type.kvpair.key_type;
                        TypeInfo *child_val_type =
                                child->static_type.kvpair.val_type;

                        key_type_to_modify =
                                generate_union_and_update_type_to_unionise(
                                        parse_arena, compiler_tables,
                                        prev_key_type, child_key_type,
                                        key_type_to_modify);

                        val_type_to_modify =
                                generate_union_and_update_type_to_unionise(
                                        parse_arena, compiler_tables,
                                        prev_val_type, child_val_type,
                                        val_type_to_modify);


                        prev_key_type = child->static_type.kvpair.key_type;
                        prev_val_type = child->static_type.kvpair.val_type;
                        child = child->adjacent_child;
                }

                child = node->nary.children;
                while (child) {
                        child->static_type.kvpair.key_type =
                                node->static_type.dict.key_type;
                        child->static_type.kvpair.val_type =
                                node->static_type.dict.val_type;
                        child = child->adjacent_child;
                }
                assert(node->static_type.dict.key_type != nullptr);
                assert(node->static_type.dict.val_type != nullptr);

        } break;

        case AstNodeType::DICTCOMP:
                break;
        case AstNodeType::LIST: {
                node->static_type.type = TypeInfoType::LIST;
                AstNode *child = node->nary.children;
                TypeInfo **type_to_modify = &node->static_type.list.item_type;
                type_parse_tree(child, parse_arena, scope_stack,
                                compiler_tables, table_to_look);
                TypeInfo *prev_type = &child->static_type;
                *type_to_modify = &child->static_type;
                child = child->adjacent_child;
                // essentially an iterative implementation of reccursively generating a union
                // tree like structure based on wether or not the last type is
                // equal to the current one if they are not then create union and
                // move the ptr to the right branch
                //
                // TODO i dont like this it makes me upset i think there is a better way to do it
                while (child) {
                        type_parse_tree(child, parse_arena, scope_stack,
                                        compiler_tables, table_to_look);
                        type_to_modify =
                                generate_union_and_update_type_to_unionise(
                                        parse_arena, compiler_tables, prev_type,
                                        &child->static_type, type_to_modify);

                        prev_type = &child->static_type;
                        child = child->adjacent_child;
                }

                child = node->nary.children;
                while (child) {
                        child->static_type = *node->static_type.list.item_type;
                        child = child->adjacent_child;
                }

                assert(node->static_type.list.item_type != nullptr);

                /*
        TypeInfo types_in_union[100] = {};
        child = node->nary.children;
        while (child) {
            child->static_type =
        }

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

        */
                // fix children into correct union types according to the mask
        } break;

        case AstNodeType::LISTCOMP:
                break;
        case AstNodeType::ASSIGNMENT: {
                type_parse_tree(node->assignment.name, parse_arena, scope_stack,
                                compiler_tables,
                                compiler_tables->variable_table);

                type_parse_tree(node->assignment.expression, parse_arena,
                                scope_stack, compiler_tables, table_to_look);

                if (!static_types_is_rhs_equal_lhs(
                            &node->assignment.expression->static_type,
                            &node->assignment.name->static_type)) {
                        fprintf(stderr, "Assignment value miscmatched types\n");
                        exit(1);
                }

                node->static_type = node->assignment.name->static_type;

        } break;

        case AstNodeType::BLOCK: {
                // Type is return statement if no return statemnt the type is None
                AstNode *child = node->file.children;
                node->static_type.type = TypeInfoType::UNKNOWN;
                int return_flag = 0;
                while (child) {
                        return_flag =
                                type_parse_tree(child, parse_arena, scope_stack,
                                                compiler_tables, table_to_look);
                        if (return_flag == 1) {
                                if (node->static_type.type == TypeInfoType::UNKNOWN) {
                                        node->static_type = child->static_type;
                                }

                                if (!static_types_is_rhs_equal_lhs(
                                            &node->static_type,
                                            &child->static_type)) {
                                        fprintf(stderr,
                                                "Must return the same type in all paths\n");
                                        exit(1);
                                }
                        }

                        child = child->adjacent_child;
                }

                return return_flag;

        } break;

        case AstNodeType::DECLARATION: {
                type_parse_tree(node->declaration.annotation, parse_arena,
                                scope_stack, compiler_tables,
                                compiler_tables->class_table);

                type_parse_tree(node->declaration.expression, parse_arena,
                                scope_stack, compiler_tables, table_to_look);

                if (node->declaration.expression) {
                        // order matters always compare the expression against the annotation
                        //
                        //TODO put this into function call
                        if (!static_types_is_rhs_equal_lhs(
                                    &node->declaration.annotation->static_type,
                                    &node->declaration.expression->static_type)) {
                                fprintf(stderr,
                                        "TypeError: line: %d, col: %d, Declaration "
                                        "expression must match annotated return type",
                                        node->token.line,
                                        node->declaration.expression->token
                                                .column);
                                exit(1);
                        }
                }

                SymbolTableEntry *entry =
                        compiler_tables->variable_table->lookup(
                                node->declaration.name->token.value,
                                scope_stack_peek(scope_stack));

                entry->value.static_type = node->declaration.annotation->static_type;
                node->static_type = node->declaration.annotation->static_type;

        } break;

        case AstNodeType::IF: {
                // NOTE: conditions evaluate to booloeans but
                // all objects types can be considered either truthy or falsy if their length evaluates to zero
                // or their __bool__ method returns false
                type_parse_tree(node->if_stmt.condition, parse_arena,
                                scope_stack, compiler_tables, table_to_look);
                int return_flag = type_parse_tree(node->if_stmt.block,
                                                  parse_arena, scope_stack,
                                                  compiler_tables,
                                                  table_to_look);

                node->static_type = node->if_stmt.block->static_type;
                type_parse_tree(node->if_stmt.or_else, parse_arena, scope_stack,
                                compiler_tables, table_to_look);

                if (!node->if_stmt.or_else) {
                        return return_flag;
                }

                if (!static_types_is_rhs_equal_lhs(
                            &node->static_type,
                            &node->if_stmt.or_else->static_type)) {
                        fprintf(stderr,
                                "TypeError: line: %d col: %d in if statement branch"
                                ", all branches must have the same return type\n"
                                "'%s' on line: %d col %d has a mismatched type\n",
                                node->if_stmt.block->token.line,
                                node->if_stmt.block->token.column,
                                debug_enum_to_string(TokenTypeEnumMembers,
                                        (int)node->if_stmt.or_else->token.type),
                                node->if_stmt.or_else->token.line,
                                node->if_stmt.or_else->token.column);

                        exit(1);
                }

                return return_flag;

        } break;

        case AstNodeType::ELSE: {
                int return_flag = type_parse_tree(node->else_stmt.block,
                                                  parse_arena, scope_stack,
                                                  compiler_tables,
                                                  table_to_look);
                node->static_type = node->else_stmt.block->static_type;

                return return_flag;
        } break;

        case AstNodeType::WHILE: {
                type_parse_tree(node->while_loop.condition, parse_arena,
                                scope_stack, compiler_tables, table_to_look);

                int return_flag = type_parse_tree(node->while_loop.block,
                                                  parse_arena, scope_stack,
                                                  compiler_tables,
                                                  table_to_look);
                type_parse_tree(node->while_loop.or_else, parse_arena,
                                scope_stack, compiler_tables, table_to_look);

                if (!node->while_loop.or_else) {
                        return return_flag;
                }

                if (!static_types_is_rhs_equal_lhs(
                            &node->static_type,
                            &node->while_loop.or_else->static_type)) {
                        fprintf(stderr,
                                "TypeError: line: %d col: %d in if statement branch"
                                ", all branches must have the same return type\n"
                                "'%s' on line: %d col %d has a mismatched type\n",
                                node->while_loop.block->token.line,
                                node->while_loop.block->token.column,
                                debug_enum_to_string(TokenTypeEnumMembers,
                                        (int)node->while_loop.or_else->token.type),
                                node->while_loop.or_else->token.line,
                                node->while_loop.or_else->token.column);

                        exit(1);
                }

                return return_flag;
        } break;

        //TODO find a way to make for loops sane in python
        case AstNodeType::FOR_LOOP: {
                type_parse_tree(node->for_loop.targets, parse_arena,
                                scope_stack,
                                compiler_tables,
                                table_to_look);
                type_parse_tree(node->for_loop.expression, parse_arena,
                                scope_stack,
                                compiler_tables,
                                table_to_look);

                //entry->value.static_type = node->static_type;
                //AstNode *target = node->for_loop.targets;
        } break;
        case AstNodeType::FOR_IF:
                break;
        case AstNodeType::CLASS_DEF: {
                SymbolTableEntry *class_scope =
                        compiler_tables->class_table->lookup(
                                node->class_def.name->token.value,
                                scope_stack_peek(scope_stack));

                scope_stack_push(scope_stack, class_scope);

                type_parse_tree(node->class_def.arguments, parse_arena,
                                scope_stack, compiler_tables, compiler_tables->class_table);
                type_parse_tree(node->class_def.block, parse_arena, scope_stack,
                                compiler_tables, table_to_look);

                scope_stack_pop(scope_stack);

        } break;

        case AstNodeType::FUNCTION_CALL: {
                type_parse_tree(node->function_call.expression, parse_arena,
                                scope_stack, compiler_tables,
                                compiler_tables->function_table);

                AstNode *call_arg = node->function_call.args;

                // lookup the function definition
                SymbolTableEntry *function_symbol =
                        node->function_call.expression->static_type
                                .custom_symbol;
                AstNode *function_node = function_symbol->value.node;
                AstNode *definition_arg = function_node->function_def.arguments;

                int i = 1;
                while (call_arg && definition_arg) {
                        type_parse_tree(call_arg, parse_arena, scope_stack,
                                        compiler_tables, table_to_look);

                        if (!static_types_is_rhs_equal_lhs(
                                    &definition_arg->static_type,
                                    &call_arg->static_type)) {
                                fprintf_s(
                                        stderr,
                                        "TypeError: line: %d, col: %d in function call arguments\n"
                                        "argument at position %d doesnt match type in function definition",
                                        call_arg->token.line, call_arg->token.column, i);
                                exit(1);
                        }

                        call_arg = call_arg->adjacent_child;
                        definition_arg = definition_arg->adjacent_child;
                        ++i;
                }

                if (definition_arg || call_arg) {
                        fprintf_s(
                                stderr,
                                "number of positional arguments don't match in call");
                        exit(1);
                }

                node->static_type = node->function_call.expression->static_type;

        } break;
        case AstNodeType::SUBSCRIPT:
                type_parse_tree(node->subscript.expression, parse_arena,
                                scope_stack, compiler_tables,
                                compiler_tables->variable_table);

                if (node->subscript.expression->static_type.type ==
                    TypeInfoType::LIST) {
                        node->static_type =
                                *node->subscript.expression->static_type.list
                                         .item_type;
                } else if (node->subscript.expression->static_type.type ==
                    TypeInfoType::DICT) {
                        node->static_type =
                                *node->subscript.expression->static_type.dict
                                         .val_type;
                }
                break;
        case AstNodeType::ATTRIBUTE_REF: {
                type_parse_tree(node->attribute_ref.name, parse_arena,
                                scope_stack, compiler_tables,
                                compiler_tables->variable_table);

                // find symbol in class
                SymbolTableEntry *result = table_to_look->lookup(
                        node->attribute_ref.attribute->token.value,
                        node->attribute_ref.name->static_type.custom_symbol);

                if (!result) {
                        fprintf(stderr,
                                "Cannot resolve name %s in attribute reference for custom type %s",
                                node->attribute_ref.attribute->token.value
                                        .c_str(),
                                node->attribute_ref.name->static_type
                                        .custom_symbol->key.identifier.c_str());
                        exit(1);
                }

                node->attribute_ref.attribute->static_type =
                        result->value.static_type;
                node->static_type = node->attribute_ref.attribute->static_type;

        } break;
        case AstNodeType::TRY:
                break;
        case AstNodeType::EXCEPT:
                break;
        case AstNodeType::STARRED:
                break;
        case AstNodeType::KVPAIR: {
                node->static_type.type = TypeInfoType::KVPAIR;
                type_parse_tree(node->kvpair.key, parse_arena, scope_stack,
                                compiler_tables, table_to_look);
                type_parse_tree(node->kvpair.value, parse_arena, scope_stack,
                                compiler_tables, table_to_look);

                node->static_type.kvpair.key_type =
                        &node->kvpair.key->static_type;
                node->static_type.kvpair.val_type =
                        &node->kvpair.value->static_type;
        } break;
        case AstNodeType::IMPORT:
                break;
        case AstNodeType::IMPORT_TARGET:
                break;
        case AstNodeType::FROM:
                break;
        case AstNodeType::FROM_TARGET:
                break;
        case AstNodeType::WITH:
                break;
        case AstNodeType::WITH_ITEM:
                break;
        case AstNodeType::MATCH:
                break;

        case AstNodeType::UNION: {
                type_parse_tree(node->union_type.left, parse_arena, scope_stack,
                                compiler_tables, table_to_look);
                type_parse_tree(node->union_type.right, parse_arena,
                                scope_stack, compiler_tables, table_to_look);

                node->static_type.type = TypeInfoType::UNION;
                node->static_type.union_type.left =
                        &node->union_type.left->static_type;
                node->static_type.union_type.right =
                        &node->union_type.right->static_type;

        } break;

        }

        return 0;
}
