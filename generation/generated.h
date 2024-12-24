EnumMemberDefinition  AstNodeTypeEnumMembers[] =
{
{"FILE", 1},
{"BINARYEXPR", 2},
{"UNARY", 3},
{"NARY", 4},
{"TUPLE", 5},
{"DICT", 6},
{"DICTCOMP", 7},
{"LIST", 8},
{"LISTCOMP", 9},
{"TERMINAL", 10},
{"IDENTIFIER", 11},
{"ASSIGNMENT", 12},
{"BLOCK", 13},
{"DECLARATION", 14},
{"TYPE_ANNOTATION", 15},
{"IF", 16},
{"ELSE", 17},
{"WHILE", 18},
{"FOR_LOOP", 19},
{"FOR_IF", 20},
{"FUNCTION_DEF", 21},
{"CLASS_DEF", 22},
{"FUNCTION_CALL", 23},
{"SUBSCRIPT", 24},
{"SLICE", 25},
{"ATTRIBUTE_REF", 26},
{"TRY", 27},
{"WITH", 28},
{"WITH_ITEM", 29},
{"EXCEPT", 30},
{"STARRED", 31},
{"KVPAIR", 32},
{"IMPORT", 33},
{"IMPORT_TARGET", 34},
{"FROM", 35},
{"FROM_TARGET", 36},
{"UNION", 37},
{"MATCH", 38},
{"RAISE", 39},
{"IF_EXPR", 40},
{"GEN_EXPR", 41},
{"LAMBDA", 42},
{"TYPE_PARAM", 43},
{"INVALID", 44},
};
StructMemberDefinition AstNodeUnaryStructMembers[] = 
{
{TYPE_AstNode_PTR, "child", (uint64_t)&((AstNodeUnary *)0)->child},
};
StructMemberDefinition AstNodeNaryStructMembers[] = 
{
{TYPE_AstNode_PTR, "children", (uint64_t)&((AstNodeNary *)0)->children},
};
StructMemberDefinition AstNodeBinaryExprStructMembers[] = 
{
{TYPE_AstNode_PTR, "left", (uint64_t)&((AstNodeBinaryExpr *)0)->left},
{TYPE_AstNode_PTR, "right", (uint64_t)&((AstNodeBinaryExpr *)0)->right},
};
StructMemberDefinition AstNodeAssignmentStructMembers[] = 
{
{TYPE_AstNode_PTR, "left", (uint64_t)&((AstNodeAssignment *)0)->left},
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeAssignment *)0)->expression},
};
StructMemberDefinition AstNodeDeclarationStructMembers[] = 
{
{TYPE_AstNode_PTR, "name", (uint64_t)&((AstNodeDeclaration *)0)->name},
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeDeclaration *)0)->expression},
{TYPE_AstNode_PTR, "annotation", (uint64_t)&((AstNodeDeclaration *)0)->annotation},
};
StructMemberDefinition AstNodeTypeAnnotStructMembers[] = 
{
{TYPE_AstNode_PTR, "type", (uint64_t)&((AstNodeTypeAnnot *)0)->type},
{TYPE_AstNode_PTR, "parameters", (uint64_t)&((AstNodeTypeAnnot *)0)->parameters},
};
StructMemberDefinition AstNodeUnionStructMembers[] = 
{
{TYPE_AstNode_PTR, "left", (uint64_t)&((AstNodeUnion *)0)->left},
{TYPE_AstNode_PTR, "right", (uint64_t)&((AstNodeUnion *)0)->right},
};
StructMemberDefinition AstNodeIfStructMembers[] = 
{
{TYPE_AstNode_PTR, "condition", (uint64_t)&((AstNodeIf *)0)->condition},
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeIf *)0)->block},
{TYPE_AstNode_PTR, "or_else", (uint64_t)&((AstNodeIf *)0)->or_else},
};
StructMemberDefinition AstNodeIfExprStructMembers[] = 
{
{TYPE_AstNode_PTR, "true_expression", (uint64_t)&((AstNodeIfExpr *)0)->true_expression},
{TYPE_AstNode_PTR, "condition", (uint64_t)&((AstNodeIfExpr *)0)->condition},
{TYPE_AstNode_PTR, "false_expression", (uint64_t)&((AstNodeIfExpr *)0)->false_expression},
};
StructMemberDefinition AstNodeElseStructMembers[] = 
{
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeElse *)0)->block},
};
StructMemberDefinition AstNodeForLoopStructMembers[] = 
{
{TYPE_AstNode_PTR, "targets", (uint64_t)&((AstNodeForLoop *)0)->targets},
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeForLoop *)0)->expression},
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeForLoop *)0)->block},
{TYPE_AstNode_PTR, "or_else", (uint64_t)&((AstNodeForLoop *)0)->or_else},
};
StructMemberDefinition AstNodeForIfClauseStructMembers[] = 
{
{TYPE_AstNode_PTR, "targets", (uint64_t)&((AstNodeForIfClause *)0)->targets},
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeForIfClause *)0)->expression},
{TYPE_AstNode_PTR, "if_clause", (uint64_t)&((AstNodeForIfClause *)0)->if_clause},
};
StructMemberDefinition AstNodeWhileStructMembers[] = 
{
{TYPE_AstNode_PTR, "condition", (uint64_t)&((AstNodeWhile *)0)->condition},
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeWhile *)0)->block},
{TYPE_AstNode_PTR, "or_else", (uint64_t)&((AstNodeWhile *)0)->or_else},
};
StructMemberDefinition AstNodeTypeParamStructMembers[] = 
{
{TYPE_AstNode_PTR, "name", (uint64_t)&((AstNodeTypeParam *)0)->name},
{TYPE_AstNode_PTR, "bound", (uint64_t)&((AstNodeTypeParam *)0)->bound},
{TYPE_bool, "star", (uint64_t)&((AstNodeTypeParam *)0)->star},
{TYPE_bool, "double_star", (uint64_t)&((AstNodeTypeParam *)0)->double_star},
};
StructMemberDefinition AstNodeClassDefStructMembers[] = 
{
{TYPE_AstNode_PTR, "decarators", (uint64_t)&((AstNodeClassDef *)0)->decarators},
{TYPE_AstNode_PTR, "name", (uint64_t)&((AstNodeClassDef *)0)->name},
{TYPE_AstNode_PTR, "type_params", (uint64_t)&((AstNodeClassDef *)0)->type_params},
{TYPE_AstNode_PTR, "arguments", (uint64_t)&((AstNodeClassDef *)0)->arguments},
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeClassDef *)0)->block},
};
StructMemberDefinition AstNodeFunctionDefStructMembers[] = 
{
{TYPE_AstNode_PTR, "decarators", (uint64_t)&((AstNodeFunctionDef *)0)->decarators},
{TYPE_AstNode_PTR, "name", (uint64_t)&((AstNodeFunctionDef *)0)->name},
{TYPE_AstNode_PTR, "type_params", (uint64_t)&((AstNodeFunctionDef *)0)->type_params},
{TYPE_AstNode_PTR, "arguments", (uint64_t)&((AstNodeFunctionDef *)0)->arguments},
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeFunctionDef *)0)->block},
{TYPE_AstNode_PTR, "star", (uint64_t)&((AstNodeFunctionDef *)0)->star},
{TYPE_AstNode_PTR, "double_star", (uint64_t)&((AstNodeFunctionDef *)0)->double_star},
{TYPE_AstNode_PTR, "return_type", (uint64_t)&((AstNodeFunctionDef *)0)->return_type},
{TYPE_int, "star_pos", (uint64_t)&((AstNodeFunctionDef *)0)->star_pos},
{TYPE_int, "slash_pos", (uint64_t)&((AstNodeFunctionDef *)0)->slash_pos},
};
StructMemberDefinition AstNodeLambdaDefStructMembers[] = 
{
{TYPE_AstNode_PTR, "arguments", (uint64_t)&((AstNodeLambdaDef *)0)->arguments},
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeLambdaDef *)0)->expression},
{TYPE_AstNode_PTR, "star", (uint64_t)&((AstNodeLambdaDef *)0)->star},
{TYPE_AstNode_PTR, "double_star", (uint64_t)&((AstNodeLambdaDef *)0)->double_star},
};
StructMemberDefinition AstNodeFunctionCallStructMembers[] = 
{
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeFunctionCall *)0)->expression},
{TYPE_AstNode_PTR, "args", (uint64_t)&((AstNodeFunctionCall *)0)->args},
};
StructMemberDefinition AstNodeKwargStructMembers[] = 
{
{TYPE_AstNode_PTR, "name", (uint64_t)&((AstNodeKwarg *)0)->name},
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeKwarg *)0)->expression},
};
StructMemberDefinition AstNodeKvPairStructMembers[] = 
{
{TYPE_AstNode_PTR, "key", (uint64_t)&((AstNodeKvPair *)0)->key},
{TYPE_AstNode_PTR, "value", (uint64_t)&((AstNodeKvPair *)0)->value},
};
StructMemberDefinition AstNodeSubscriptStructMembers[] = 
{
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeSubscript *)0)->expression},
{TYPE_AstNode_PTR, "slices", (uint64_t)&((AstNodeSubscript *)0)->slices},
};
StructMemberDefinition AstNodeSliceStructMembers[] = 
{
{TYPE_AstNode_PTR, "start", (uint64_t)&((AstNodeSlice *)0)->start},
{TYPE_AstNode_PTR, "end", (uint64_t)&((AstNodeSlice *)0)->end},
{TYPE_AstNode_PTR, "step", (uint64_t)&((AstNodeSlice *)0)->step},
{TYPE_AstNode_PTR, "named_expr", (uint64_t)&((AstNodeSlice *)0)->named_expr},
};
StructMemberDefinition AstNodeTryStructMembers[] = 
{
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeTry *)0)->block},
{TYPE_AstNode_PTR, "handlers", (uint64_t)&((AstNodeTry *)0)->handlers},
{TYPE_AstNode_PTR, "or_else", (uint64_t)&((AstNodeTry *)0)->or_else},
{TYPE_AstNode_PTR, "finally", (uint64_t)&((AstNodeTry *)0)->finally},
};
StructMemberDefinition AstNodeWithItemStructMembers[] = 
{
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeWithItem *)0)->expression},
{TYPE_AstNode_PTR, "target", (uint64_t)&((AstNodeWithItem *)0)->target},
};
StructMemberDefinition AstNodeWithStructMembers[] = 
{
{TYPE_AstNode_PTR, "items", (uint64_t)&((AstNodeWith *)0)->items},
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeWith *)0)->block},
};
StructMemberDefinition AstNodeExceptStructMembers[] = 
{
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeExcept *)0)->expression},
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeExcept *)0)->block},
};
StructMemberDefinition AstNodeStarExpressionStructMembers[] = 
{
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeStarExpression *)0)->expression},
};
StructMemberDefinition AstNodeImportTargetStructMembers[] = 
{
{TYPE_AstNode_PTR, "dotted_name", (uint64_t)&((AstNodeImportTarget *)0)->dotted_name},
{TYPE_AstNode_PTR, "as", (uint64_t)&((AstNodeImportTarget *)0)->as},
};
StructMemberDefinition AstNodeFromImportTargetStructMembers[] = 
{
{TYPE_AstNode_PTR, "name", (uint64_t)&((AstNodeFromImportTarget *)0)->name},
{TYPE_AstNode_PTR, "as", (uint64_t)&((AstNodeFromImportTarget *)0)->as},
};
StructMemberDefinition AstNodeRaiseStructMembers[] = 
{
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeRaise *)0)->expression},
{TYPE_AstNode_PTR, "from_expression", (uint64_t)&((AstNodeRaise *)0)->from_expression},
};
StructMemberDefinition AstNodeFromStructMembers[] = 
{
{TYPE_AstNode_PTR, "dotted_name", (uint64_t)&((AstNodeFrom *)0)->dotted_name},
{TYPE_AstNode_PTR, "targets", (uint64_t)&((AstNodeFrom *)0)->targets},
{TYPE_bool, "is_wildcard", (uint64_t)&((AstNodeFrom *)0)->is_wildcard},
};
StructMemberDefinition AstNodeMatchStructMembers[] = 
{
{TYPE_AstNode_PTR, "subject", (uint64_t)&((AstNodeMatch *)0)->subject},
{TYPE_AstNode_PTR, "case_block", (uint64_t)&((AstNodeMatch *)0)->case_block},
};
StructMemberDefinition AstNodeAttributeRefStructMembers[] = 
{
{TYPE_AstNode_PTR, "name", (uint64_t)&((AstNodeAttributeRef *)0)->name},
{TYPE_AstNode_PTR, "attribute", (uint64_t)&((AstNodeAttributeRef *)0)->attribute},
};
StructMemberDefinition AstNodeFileStructMembers[] = 
{
{TYPE_AstNode_PTR, "children", (uint64_t)&((AstNodeFile *)0)->children},
};
StructMemberDefinition AstNodeBlockStructMembers[] = 
{
{TYPE_AstNode_PTR, "children", (uint64_t)&((AstNodeBlock *)0)->children},
};
StructMemberDefinition AstNodeTupleStructMembers[] = 
{
{TYPE_AstNode_PTR, "children", (uint64_t)&((AstNodeTuple *)0)->children},
};
StructMemberDefinition AstNodeGenExprStructMembers[] = 
{
{TYPE_AstNode_PTR, "expression", (uint64_t)&((AstNodeGenExpr *)0)->expression},
{TYPE_AstNode_PTR, "for_if_clauses", (uint64_t)&((AstNodeGenExpr *)0)->for_if_clauses},
};
StructMemberDefinition AstNodeImportStructMembers[] = 
{
{TYPE_AstNode_PTR, "children", (uint64_t)&((AstNodeImport *)0)->children},
};
StructMemberDefinition AstNodeListStructMembers[] = 
{
{TYPE_AstNode_PTR, "children", (uint64_t)&((AstNodeList *)0)->children},
};
StructMemberDefinition AstNodeDictStructMembers[] = 
{
{TYPE_AstNode_PTR, "children", (uint64_t)&((AstNodeDict *)0)->children},
};
EnumMemberDefinition  ParseErrorTypeEnumMembers[] =
{
{"NONE", 0},
{"INVALID_SYNTAX", 2},
{"GENERAL", 3},
};
EnumMemberDefinition  TypeInfoTypeEnumMembers[] =
{
{"ANY", 0},
{"INTEGER", 1},
{"FLOAT", 2},
{"STRING", 3},
{"BOOLEAN", 4},
{"COMPLEX", 5},
{"NONE", 6},
{"NOT_IMPLEMENTED", 7},
{"LIST", 8},
{"DICT", 9},
{"KVPAIR", 10},
{"UNION", 11},
{"CLASS", 12},
{"FUNCTION", 13},
{"UNKNOWN", 14},
{"SIZE", 15},
};
StructMemberDefinition TypeInfoListStructMembers[] = 
{
{TYPE_TypeInfo_PTR, "item_type", (uint64_t)&((TypeInfoList *)0)->item_type},
};
StructMemberDefinition TypeInfoParameterisedStructMembers[] = 
{
{TYPE_TypeInfo_PTR, "parameters", (uint64_t)&((TypeInfoParameterised *)0)->parameters},
};
StructMemberDefinition TypeInfoDictStructMembers[] = 
{
{TYPE_TypeInfo_PTR, "key_type", (uint64_t)&((TypeInfoDict *)0)->key_type},
{TYPE_TypeInfo_PTR, "val_type", (uint64_t)&((TypeInfoDict *)0)->val_type},
};
StructMemberDefinition TypeInfoKVpairStructMembers[] = 
{
{TYPE_TypeInfo_PTR, "val_type", (uint64_t)&((TypeInfoKVpair *)0)->val_type},
{TYPE_TypeInfo_PTR, "key_type", (uint64_t)&((TypeInfoKVpair *)0)->key_type},
};
StructMemberDefinition TypeInfoUnionStructMembers[] = 
{
{TYPE_TypeInfo_PTR, "left", (uint64_t)&((TypeInfoUnion *)0)->left},
{TYPE_TypeInfo_PTR, "right", (uint64_t)&((TypeInfoUnion *)0)->right},
};
StructMemberDefinition TypeInfoClassStructMembers[] = 
{
{TYPE_SymbolTableEntry_PTR, "custom_symbol", (uint64_t)&((TypeInfoClass *)0)->custom_symbol},
};
StructMemberDefinition TypeInfoFunctionStructMembers[] = 
{
{TYPE_TypeInfo_PTR, "return_type", (uint64_t)&((TypeInfoFunction *)0)->return_type},
{TYPE_SymbolTableEntry_PTR, "custom_symbol", (uint64_t)&((TypeInfoFunction *)0)->custom_symbol},
};
