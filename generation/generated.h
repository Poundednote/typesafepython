EnumMemberDefinition  AstNodeTypeEnumMembers[] =
{
{"FILE", 0},
{"BINARYEXPR", 1},
{"UNARY", 2},
{"NARY", 3},
{"TUPLE", 4},
{"DICT", 5},
{"DICTCOMP", 6},
{"LIST", 7},
{"LISTCOMP", 8},
{"TERMINAL", 9},
{"ASSIGNMENT", 10},
{"BLOCK", 11},
{"DECLARATION", 12},
{"TYPE_ANNOTATION", 13},
{"IF", 14},
{"ELSE", 15},
{"WHILE", 16},
{"FOR_LOOP", 17},
{"FOR_IF", 18},
{"FUNCTION_DEF", 19},
{"CLASS_DEF", 20},
{"FUNCTION_CALL", 21},
{"SUBSCRIPT", 22},
{"ATTRIBUTE_REF", 23},
{"TRY", 24},
{"WITH", 25},
{"WITH_ITEM", 26},
{"EXCEPT", 27},
{"STARRED", 28},
{"KVPAIR", 29},
{"IMPORT", 30},
{"IMPORT_TARGET", 31},
{"FROM", 32},
{"FROM_TARGET", 33},
{"UNION", 34},
{"MATCH", 35},
{"RAISE", 36},
{"IF_EXPR", 37},
{"GEN_EXPR", 38},
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
{TYPE_AstNode_PTR, "name", (uint64_t)&((AstNodeAssignment *)0)->name},
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
StructMemberDefinition AstNodeClassDefStructMembers[] = 
{
{TYPE_AstNode_PTR, "decarators", (uint64_t)&((AstNodeClassDef *)0)->decarators},
{TYPE_AstNode_PTR, "name", (uint64_t)&((AstNodeClassDef *)0)->name},
{TYPE_AstNode_PTR, "arguments", (uint64_t)&((AstNodeClassDef *)0)->arguments},
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeClassDef *)0)->block},
};
StructMemberDefinition AstNodeFunctionDefStructMembers[] = 
{
{TYPE_AstNode_PTR, "decarators", (uint64_t)&((AstNodeFunctionDef *)0)->decarators},
{TYPE_AstNode_PTR, "name", (uint64_t)&((AstNodeFunctionDef *)0)->name},
{TYPE_AstNode_PTR, "arguments", (uint64_t)&((AstNodeFunctionDef *)0)->arguments},
{TYPE_AstNode_PTR, "block", (uint64_t)&((AstNodeFunctionDef *)0)->block},
{TYPE_AstNode_PTR, "star", (uint64_t)&((AstNodeFunctionDef *)0)->star},
{TYPE_AstNode_PTR, "double_star", (uint64_t)&((AstNodeFunctionDef *)0)->double_star},
{TYPE_AstNode_PTR, "return_type", (uint64_t)&((AstNodeFunctionDef *)0)->return_type},
{TYPE_bool, "has_star", (uint64_t)&((AstNodeFunctionDef *)0)->has_star},
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
{TYPE_AstNode_PTR, "start", (uint64_t)&((AstNodeSubscript *)0)->start},
{TYPE_AstNode_PTR, "end", (uint64_t)&((AstNodeSubscript *)0)->end},
{TYPE_AstNode_PTR, "step", (uint64_t)&((AstNodeSubscript *)0)->step},
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
EnumMemberDefinition  TokenTypeEnumMembers[] =
{
{"OR", 0},
{"AND", 1},
{"NOT", 2},
{"EQ", 3},
{"NE", 4},
{"LE", 5},
{"LT", 6},
{"GE", 7},
{"GT", 8},
{"IS", 9},
{"IN_TOK", 10},
{"NOT_IN", 11},
{"BWOR", 12},
{"BWXOR", 13},
{"BWAND", 14},
{"SHIFTLEFT", 15},
{"SHIFTRIGHT", 16},
{"ADDITION", 17},
{"SUBTRACTION", 18},
{"MULTIPLICATION", 19},
{"DIVISION", 20},
{"FLOOR_DIV", 21},
{"REMAINDER", 22},
{"EXPONENTIATION", 23},
{"DOT", 24},
{"RETURN", 25},
{"YIELD", 26},
{"RAISE", 27},
{"GLOBAL", 28},
{"NONLOCAL", 29},
{"IF", 30},
{"ELIF", 31},
{"ELSE", 32},
{"DEF", 33},
{"CLASS", 34},
{"WHILE", 35},
{"FOR", 36},
{"TRY", 37},
{"EXCEPT", 38},
{"FINALLY", 39},
{"WITH", 40},
{"AS", 41},
{"PASS", 42},
{"BREAK", 43},
{"CONTINUE", 44},
{"DEL", 45},
{"MATCH", 46},
{"CASE", 47},
{"IDENTIFIER", 48},
{"INT_LIT", 49},
{"FLOAT_LIT", 50},
{"STRING_LIT", 51},
{"FSTRING", 52},
{"NONE", 53},
{"BOOL_TRUE", 54},
{"BOOL_FALSE", 55},
{"OPEN_PAREN", 56},
{"CLOSED_PAREN", 57},
{"SQUARE_OPEN_PAREN", 58},
{"SQUARE_CLOSED_PAREN", 59},
{"CURLY_OPEN_PAREN", 60},
{"CURLY_CLOSED_PAREN", 61},
{"COMMA", 62},
{"ASSIGN", 63},
{"COLON", 64},
{"COLON_EQUAL", 65},
{"ARROW", 66},
{"AT", 67},
{"IMPORT", 68},
{"FROM", 69},
{"NEWLINE", 70},
{"INDENT", 71},
{"DEDENT", 72},
{"FILE", 73},
{"ENDFILE", 74},
};
EnumMemberDefinition  TypeInfoTypeEnumMembers[] =
{
{"UNKNOWN", 0},
{"ANY", 1},
{"INTEGER", 2},
{"FLOAT", 3},
{"STRING", 4},
{"BOOLEAN", 5},
{"NONE", 6},
{"COMPLEX", 7},
{"LIST", 8},
{"DICT", 9},
{"KVPAIR", 10},
{"UNION", 11},
{"CLASS", 12},
{"SIZE", 13},
};
