#include "debug.h"
#include <string>

// this is metatprogramed the enum passed must be marked for introspection
inline const char *debug_enum_to_string(EnumMemberDefinition *enumMembers, int value_of_enum)
{
    return enumMembers[value_of_enum].name;
}


static void debug_print_indent(uint32_t indent)
{
        for (int i = 0; i < indent; ++i) {
            for (int j = 0; j < 8; ++j) {
                printf(" ");
            }
        }
}

static void debug_print_node(AstNode *node, uint32_t indent)
{
        std::string value = "";

        const char *token = debug_enum_to_string(TokenTypeEnumMembers,
                                                      (int)node->token.type);
        value = node->token.value;

        if (value == "") {
                value = "No value";
        }

        const char *red = "\x1b[31m";
        const char *green = "\x1b[32m";
        const char *blue = "\x1b[34m";
        const char *standard = "\x1b[0m";

        printf("\n");
        debug_print_indent(indent);
        printf("Token: %s%s%s, value: %s, Node: %s%s%s Type: %s%s%s {", red,
               token, standard, value.c_str(), green,
               debug_enum_to_string(AstNodeTypeEnumMembers, (int)node->type),
               standard, blue,
               debug_enum_to_string(TypeInfoTypeEnumMembers,
                                    (int)node->static_type.type),
               standard);
}

static void debug_print_node_struct(StructMemberDefinition *struct_members,
                                    size_t member_count, void *node,
                                    uint32_t indent)
{
        for (int i = 0; i < member_count; ++i) {
                StructMemberDefinition member = struct_members[i];
                switch (member.type) {
                case TYPE_AstNode_PTR: {
                        AstNode *child_node = *((AstNode **)((char *)node + member.offset));
                        while (child_node) {
                                debug_print_parse_tree(child_node, indent);
                                child_node = child_node->adjacent_child;
                        }

                } break;

                default:
                        break;
                }
        }
}

static void debug_print_parse_tree(AstNode *node, uint32_t indent)
{
        if (!node) {
                return;
        }

        debug_print_node(node, indent);

        switch (node->type) {
        case AstNodeType::FILE:
                debug_print_node_struct(AstNodeFileStructMembers,
                                        array_count(AstNodeFileStructMembers),
                                        (void *)(&node->file), indent + 1);
                break;

        case AstNodeType::BLOCK:
                debug_print_node_struct(AstNodeBlockStructMembers,
                                        array_count(AstNodeBlockStructMembers),
                                        (void *)(&node->block), indent + 1);

                break;

        case AstNodeType::IMPORT:
                debug_print_node_struct(AstNodeImportStructMembers,
                                        array_count(AstNodeImportStructMembers),
                                        (void *)(&node->import), indent + 1);
                break;

        case AstNodeType::IMPORT_TARGET:
                debug_print_node_struct(
                        AstNodeImportTargetStructMembers,
                        array_count(AstNodeImportTargetStructMembers),
                        (void *)(&node->import_target), indent + 1);
                break;

        case AstNodeType::FROM:
                debug_print_node_struct(AstNodeFromStructMembers,
                                        array_count(AstNodeFromStructMembers),
                                        (void *)(&node->from), indent + 1);
                break;

        case AstNodeType::FROM_TARGET:
                debug_print_node_struct(
                        AstNodeFromImportTargetStructMembers,
                        array_count(AstNodeFromImportTargetStructMembers),
                        (void *)(&node->from_target), indent + 1);
                break;

        case AstNodeType::FOR_IF:
                debug_print_node_struct(AstNodeForIfClauseStructMembers,
                                        array_count(AstNodeForIfClauseStructMembers),
                                        (void *)(&node->for_if), indent + 1);

                break;

        case AstNodeType::KVPAIR:
                debug_print_node_struct(AstNodeKvPairStructMembers,
                                        array_count(AstNodeKvPairStructMembers),
                                        (void *)(&node->for_if), indent + 1);
                break;

        case AstNodeType::ELSE:
                debug_print_node_struct(AstNodeElseStructMembers,
                                        array_count(AstNodeElseStructMembers),
                                        (void *)(&node->else_stmt), indent + 1);
                break;

        case AstNodeType::IF:
                debug_print_node_struct(AstNodeIfStructMembers,
                                        array_count(AstNodeIfStructMembers),
                                        (void *)(&node->if_stmt), indent + 1);
                break;

        case AstNodeType::WHILE:
                debug_print_node_struct(AstNodeWhileStructMembers,
                                        array_count(AstNodeWhileStructMembers),
                                        (void *)(&node->while_loop),
                                        indent + 1);
                break;

        case AstNodeType::DECLARATION:
                debug_print_node_struct(AstNodeDeclarationStructMembers,
                                        array_count(AstNodeDeclarationStructMembers),
                                        (void *)(&node->declaration),
                                        indent + 1);
                break;

        case AstNodeType::ASSIGNMENT:
                debug_print_node_struct(AstNodeAssignmentStructMembers,
                                        array_count(AstNodeAssignmentStructMembers),
                                        (void *)(&node->assignment),
                                        indent + 1);
                break;

        case AstNodeType::SUBSCRIPT:
                debug_print_node_struct(AstNodeSubscriptStructMembers,
                                        array_count(AstNodeSubscriptStructMembers),
                                        (void *)(&node->subscript), indent + 1);
                break;

        case AstNodeType::FUNCTION_CALL:
                debug_print_node_struct(
                        AstNodeFunctionCallStructMembers,
                        array_count(AstNodeFunctionCallStructMembers),
                        (void *)(&node->function_call), indent + 1);
                break;

        case AstNodeType::STARRED:
                debug_print_node_struct(
                        AstNodeStarExpressionStructMembers,
                        array_count(AstNodeStarExpressionStructMembers),
                        (void *)(&node->star_expression), indent + 1);
                break;

        case AstNodeType::FUNCTION_DEF:
                debug_print_node_struct(AstNodeFunctionDefStructMembers,
                                        array_count(AstNodeFunctionDefStructMembers),
                                        (void *)(&node->function_def),
                                        indent + 1);
                break;

        case AstNodeType::CLASS_DEF:
                debug_print_node_struct(AstNodeClassDefStructMembers,
                                        array_count(AstNodeClassDefStructMembers),
                                        (void *)(&node->class_def), indent + 1);
                break;

        case AstNodeType::FOR_LOOP:
                debug_print_node_struct(AstNodeForLoopStructMembers,
                                        array_count(AstNodeForLoopStructMembers),
                                        (void *)(&node->for_loop), indent + 1);
                break;

        case AstNodeType::TRY:
                debug_print_node_struct(AstNodeSubscriptStructMembers,
                                        array_count(AstNodeTryStructMembers),
                                        (void *)(&node->try_node), indent + 1);

                break;

        case AstNodeType::WITH_ITEM:
                debug_print_node_struct(AstNodeWithItemStructMembers,
                                        array_count(AstNodeWithItemStructMembers),
                                        (void *)(&node->with_item), indent + 1);

                break;

        case AstNodeType::WITH:
                debug_print_node_struct(AstNodeWithStructMembers,
                                        array_count(AstNodeWithStructMembers),
                                        (void *)(&node->with_statement),
                                        indent + 1);

                break;

        case AstNodeType::EXCEPT:
                debug_print_node_struct(AstNodeExceptStructMembers,
                                        array_count(AstNodeExceptStructMembers),
                                        (void *)(&node->except), indent + 1);
                break;

        case AstNodeType::NARY:
                debug_print_node_struct(AstNodeNaryStructMembers,
                                        array_count(AstNodeNaryStructMembers),
                                        (void *)(&node->nary), indent + 1);

                break;

        case AstNodeType::BINARYEXPR:
                debug_print_node_struct(AstNodeBinaryExprStructMembers,
                                        array_count(AstNodeBinaryExprStructMembers),
                                        (void *)(&node->binary), indent + 1);
                break;

        case AstNodeType::ATTRIBUTE_REF:
                debug_print_node_struct(
                        AstNodeAttributeRefStructMembers,
                        array_count(AstNodeAttributeRefStructMembers),
                        (void *)(&node->attribute_ref), indent + 1);
                break;

        case AstNodeType::UNARY:
                debug_print_node_struct(AstNodeUnaryStructMembers,
                                        array_count(AstNodeUnaryStructMembers),
                                        (void *)(&node->unary), indent + 1);
                break;

        case AstNodeType::TUPLE:
                debug_print_node_struct(AstNodeTupleStructMembers,
                                        array_count(AstNodeTupleStructMembers),
                                        (void *)(&node->tuple), indent + 1);
                break;

        case AstNodeType::DICT:
                debug_print_node_struct(AstNodeDictStructMembers,
                                        array_count(AstNodeDictStructMembers),
                                        (void *)(&node->dict), indent + 1);
                break;

        case AstNodeType::DICTCOMP:
                debug_print_node_struct(AstNodeDictStructMembers,
                                        array_count(AstNodeDictStructMembers),
                                        (void *)(&node->dict), indent + 1);
                break;

        case AstNodeType::LIST:
                debug_print_node_struct(AstNodeListStructMembers,
                                        array_count(AstNodeListStructMembers),
                                        (void *)(&node->list), indent + 1);
                break;

        case AstNodeType::LISTCOMP:
                debug_print_node_struct(AstNodeListStructMembers,
                                        array_count(AstNodeListStructMembers),
                                        (void *)(&node->list), indent + 1);
                break;

        case AstNodeType::UNION:
                debug_print_node_struct(AstNodeUnionStructMembers,
                                        array_count(AstNodeUnionStructMembers),
                                        (void *)(&node->union_type),
                                        indent + 1);
                break;

        case AstNodeType::MATCH:
                debug_print_node_struct(AstNodeMatchStructMembers,
                                        array_count(AstNodeMatchStructMembers),
                                        (void *)(&node->match), indent + 1);
                break;

        case AstNodeType::TYPE_ANNOTATION:
                debug_print_node_struct(AstNodeTypeAnnotStructMembers,
                                        array_count(AstNodeTypeAnnotStructMembers),
                                        (void *)(&node->type_annotation),
                                        indent + 1);
                break;

        case AstNodeType::RAISE:
                debug_print_node_struct(AstNodeRaiseStructMembers,
                                        array_count(AstNodeRaiseStructMembers),
                                        (void *)(&node->raise), indent + 1);
                break;

        case AstNodeType::IF_EXPR:
                debug_print_node_struct(AstNodeIfExprStructMembers,
                                        array_count(AstNodeIfExprStructMembers),
                                        (void *)(&node->if_expr), indent + 1);

        case AstNodeType::GEN_EXPR:
                debug_print_node_struct(AstNodeGenExprStructMembers,
                                        array_count(AstNodeGenExprStructMembers),
                                        (void *)(&node->gen_expr), indent + 1);

        case AstNodeType::TERMINAL:
                printf("}");
                return;
        }

       printf("\n");
       debug_print_indent(indent);
       printf("}\n");
}
