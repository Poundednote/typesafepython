#include <stdint.h>
#include "parser.h"
#include "tokeniser.h"

#ifdef _WIN32
#define PAGE_SIZE (0x1000)
#endif
#define KILOBYTES(n) (n * (size_t)1024)
#define MEGABYTES(n) ((KILOBYTES(n)) * 1024)
#define GIGABYTES(n) ((MEGABYTES(n)) * 1024)


static std::string debug_static_type_to_string(TypeInfo type_info) {
    switch (type_info.type) {
        case TypeInfoType::INTEGER: return "int";
        case TypeInfoType::FLOAT: return "float";
        case TypeInfoType::STRING: return "str";
        case TypeInfoType::BOOLEAN: return "bool";
        case TypeInfoType::COMPLEX: return "complex";
        case TypeInfoType::NONE: return "None";
        case TypeInfoType::UNKNOWN: return "~";
        case TypeInfoType::CUSTOM: return type_info.custom_symbol->key.identifier;
    }

    return "";
}

static void debug_print_indent(uint32_t indent) {
    for (int i = 0; i < indent; ++i) {
        printf("    ");
    }
}

static void debug_print_node(AstNode *node, uint32_t indent) {
    std::string token = "";
    std::string value = "";

    token = debug_token_type_to_string(node->token.type);
    value = node->token.value;

    if (value == "") {
        value = "No value";
    }

    printf("\n");
    debug_print_indent(indent);
    printf("node: %s, value: %s, node_type: %d static_type: %s {",
           token.c_str(), value.c_str(), (int)node->type,
           debug_static_type_to_string(node->static_type).c_str());

}

static void debug_print_parse_tree(AstNode *node, uint32_t indent) {
    if (!node) {
        return;
    }

    debug_print_node(node, indent);

    switch (node->type) {
    case AstNodeType::FILE: {
        AstNode *child = node->file.children;
        while (child) {
            debug_print_parse_tree(child, indent+1);
            child = child->adjacent_child;
        }

    } break;

    case AstNodeType::BLOCK: {
        AstNode *child = node->block.children;
        while (child) {
            debug_print_parse_tree(child, indent+1);
            child = child->adjacent_child;
        }

    } break;

    case AstNodeType::IMPORT: {
        debug_print_parse_tree(node->import.target, indent + 1);
        debug_print_parse_tree(node->import.as, indent + 1);
    } break;

    case AstNodeType::FOR_IF: {
        AstNode *target = node->for_if.targets;
        while (target) {
            debug_print_parse_tree(target, indent + 1);
            target = target->adjacent_child;
        }
        debug_print_parse_tree(node->for_if.expression, indent+1);
        debug_print_parse_tree(node->for_if.if_clause, indent+1);

    } break;

    case AstNodeType::KVPAIR:
        debug_print_parse_tree(node->kvpair.key, indent+1);
        debug_print_parse_tree(node->kvpair.value, indent+1);
        break;

    case AstNodeType::ELSE:
        debug_print_parse_tree(node->else_stmt.block, indent+1);
        break;

    case AstNodeType::IF:
        debug_print_parse_tree(node->if_stmt.condition, indent+1);
        debug_print_parse_tree(node->if_stmt.block, indent+1);
        debug_print_parse_tree(node->if_stmt.or_else, indent+1);
        break;

    case AstNodeType::WHILE:
        debug_print_parse_tree(node->while_loop.condition, indent+1);
        debug_print_parse_tree(node->while_loop.block, indent+1);
        debug_print_parse_tree(node->while_loop.or_else, indent+1);
        break;

    case AstNodeType::DECLARATION:
        debug_print_parse_tree(node->declaration.name, indent+1);
        debug_print_parse_tree(node->declaration.annotation, indent+1);
        debug_print_parse_tree(node->declaration.expression, indent+1);
        break;


    case AstNodeType::ASSIGNMENT:
        debug_print_parse_tree(node->assignment.name, indent+1);
        debug_print_parse_tree(node->assignment.expression, indent+1);
        break;

    case AstNodeType::SUBSCRIPT:
        debug_print_parse_tree(node->subscript.expression, indent+1);
        for (int i = 0; i < 3; ++i) {
            debug_print_parse_tree(node->subscript.children[i], indent+1);
        }

        break;

    case AstNodeType::FUNCTION_CALL: {
        debug_print_parse_tree(node->function_call.expression, indent+1);
        AstNode *arg = node->function_call.args;
        while(arg) {
            debug_print_parse_tree(arg, indent+1);
        }

    } break;

    case AstNodeType::STARRED:
        debug_print_parse_tree(node->star_expression.expression, indent+1);
        break;

    case AstNodeType::FUNCTION_DEF: {
        debug_print_parse_tree(node->function_def.name, indent+1);

        AstNode *arg = node->function_def.arguments;
        while(arg) {
            debug_print_parse_tree(arg, indent+1);
            arg = arg->adjacent_child;
        }

        debug_print_parse_tree(node->function_def.return_type, indent+1);
        debug_print_parse_tree(node->function_def.block, indent+1);
        AstNode *decorator = node->function_def.decarators;
        while (decorator) {
            debug_print_parse_tree(node->function_def.decarators, indent+1);
            decorator = decorator->adjacent_child;
        }

    } break;

    case AstNodeType::CLASS_DEF: {

        debug_print_parse_tree(node->class_def.name, indent+1);
        debug_print_parse_tree(node->class_def.arguments, indent+1);
        debug_print_parse_tree(node->class_def.block, indent+1);

        AstNode *decorator = node->class_def.decarators;
        while (decorator) {
            debug_print_parse_tree(node->function_def.decarators, indent+1);
            decorator = decorator->adjacent_child;
        }
    } break;

    case AstNodeType::FOR_LOOP:
        debug_print_parse_tree(node->for_loop.targets, indent+1);
        debug_print_parse_tree(node->for_loop.expression, indent+1);
        debug_print_parse_tree(node->for_loop.block, indent+1);
        debug_print_parse_tree(node->for_loop.or_else, indent+1);
        break;

    case AstNodeType::TRY: {
        debug_print_parse_tree(node->try_node.block, indent+1);
        AstNode *handler = node->try_node.handlers;
        while (handler) {
            debug_print_parse_tree(handler, indent+1);
            handler = handler->adjacent_child;
        }

        debug_print_parse_tree(node->try_node.finally, indent+1);

    } break;

    case AstNodeType::EXCEPT:
        debug_print_parse_tree(node->except.expression, indent+1);
        debug_print_parse_tree(node->except.block, indent+1);
        break;

    case AstNodeType::NARY: {
        AstNode *child = node->nary.children;

        while (child) {
            debug_print_parse_tree(child, indent+1);
            child = child->adjacent_child;
        }

    } break;

    case AstNodeType::BINARYEXPR:
        debug_print_parse_tree(node->binary.left, indent + 1);
        debug_print_parse_tree(node->binary.right, indent + 1);
        break;

    case AstNodeType::ATTRIBUTE_REF:
        debug_print_parse_tree(node->attribute_ref.name, indent+1);
        debug_print_parse_tree(node->attribute_ref.attribute, indent+1);
        break;

    case AstNodeType::UNARY:

        debug_print_parse_tree(node->unary.child, indent + 1);
        break;

    case AstNodeType::TERMINAL:
        printf("}");
        return;
    }


    printf("\n");
    debug_print_indent(indent);
    printf("}\n");
}


inline Arena Arena::init(size_t reserve) {
    Arena arena = {};

#ifdef _WIN32
    arena.memory = VirtualAlloc(0, reserve,
                                MEM_RESERVE, PAGE_NOACCESS);

    VirtualAlloc(arena.memory, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
#endif

    arena.capacity = PAGE_SIZE;
    arena.offset = 0;

    return arena;
}

//TODO make sure items have correct alignment
void *Arena::alloc(size_t size) {
    char *new_base = ((char *)this->memory+this->offset);
    if (this->offset+size >= this->capacity) {
        size_t bytes_to_alloc = PAGE_SIZE;

        if (size > PAGE_SIZE) {
            bytes_to_alloc = size;
        }

        VirtualAlloc(new_base, bytes_to_alloc, MEM_COMMIT, PAGE_READWRITE);

        if (!new_base) {
            perror("Last allocation failed");
            exit(1);
        }

        this->capacity += bytes_to_alloc;
    }

    assert(this->offset < this->capacity);
    this->offset += size;
    return (void *)new_base;
};

inline uint32_t SymbolTable::hash(std::string &string, SymbolTableEntry *scope) {
    uint32_t hash = 0;
    for (int i = 0; i < string.length(); ++i) {
        hash = hash * 101 + string[i];
    }

    if (scope != nullptr) {
        for (int i = 0; i < scope->key.identifier.length(); ++i) {
            hash = hash * 101 + scope->key.identifier[i];
        }
    }

    return hash % SYMBOL_TABLE_ARRAY_SIZE;
}

inline SymbolTableEntry *SymbolTable::lookup(std::string &string, SymbolTableEntry *scope) {
    SymbolTableEntry *entry = &this->table[hash(string, scope)];
    if (entry->key.identifier == "") {
        return nullptr;
    }

    if (string == "") {
        return nullptr;
    }

    if (entry->key.identifier == string &&
        entry->key.scope == scope) {

        return entry;
    }

    return nullptr;
}

inline SymbolTableEntry *SymbolTable::insert(Arena *arena, std::string string,
                                             SymbolTableEntry *scope,
                                             SymbolTableValue value) {

    SymbolTableEntry *entry = &this->table[this->hash(string, scope)];

    if (this->lookup(string, scope) != nullptr) {
        entry->value = value;
    }

    // Collision
    else if (entry->key.identifier != "") {
        entry->next_in_table = (SymbolTableEntry *)arena->alloc(sizeof(SymbolTableEntry));
        *entry->next_in_table = SymbolTableEntry();
    }

    else {
        entry->key.identifier = string;
        entry->key.scope = scope;
        entry->value = value;
    }

    return entry;
}

inline CompilerTables CompilerTables::init(Arena *arena) {
    CompilerTables compiler_tables = {};
    compiler_tables.variable_table = (SymbolTable *)arena->alloc(sizeof(SymbolTable[3]));
    *compiler_tables.variable_table = SymbolTable();
    compiler_tables.function_table = compiler_tables.variable_table+1;
    *compiler_tables.function_table = SymbolTable();
    compiler_tables.class_table = compiler_tables.variable_table+2;
    *compiler_tables.class_table = SymbolTable();

    return compiler_tables;
}

static inline AstNode *node_alloc(Arena *ast_arena, AstNode *node,
                                  SymbolTableEntry *scope) {

    AstNode *allocated_node = (AstNode *)ast_arena->alloc(sizeof(AstNode));

    // call constructor because AstNode contains std::string .....
    *allocated_node = AstNode();
    *allocated_node = *node;
    allocated_node->scope = scope;

    return allocated_node;
}

static inline AstNode *parse_single_token_into_node(Tokeniser *tokeniser,
                                                    Arena *ast_arena,
                                                    SymbolTableEntry *scope,
                                                    CompilerTables *compiler_tables,
                                                    Arena *symbol_table_arena) {
    AstNode node = {};
    node.token = tokeniser->last_returned;
    tokeniser->next_token();
    return node_alloc(ast_arena, &node, scope);
}

inline AstNode AstNode::create_unary(Token token, AstNode *child) {
    AstNode node = {};
    node.token = token;
    node.type = AstNodeType::UNARY;
    node.unary.child = child;

    return node;
}

inline AstNode AstNode::create_binary(Token token, AstNode *left,
                                      AstNode *right) {
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
                                                const char *message) {

    if (token.type != type) {
        printf("Syntax Error: line: %d col %d\nExpected Token '%s' instead of "
               "'%s'\n",
               token.line, token.column,
               debug_token_type_to_string(type).c_str(),
               debug_token_type_to_string(token.type).c_str());

        printf("%s\n", message);
        exit(1);
    }
}

static inline AstNode *parse_name(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {

    assert_token_and_print_debug(TokenType::IDENTIFIER,
                                 tokeniser->last_returned,
                                 "Failed to parse name");
    AstNode child = {};
    child.token = tokeniser->last_returned;
    tokeniser->next_token();
    return node_alloc(ast_arena, &child, scope);
}

static AstNode *parse_list_of_names_and_return_modifier(Tokeniser *tokeniser,
                                                        Arena *ast_arena,
                                                        SymbolTableEntry *scope,
                                                        CompilerTables *compiler_tables,
                                                        Arena *symbol_table_arena,
                                                        Token token) {

    AstNode parent = {};
    parent.type = AstNodeType::NARY;
    parent.token = token;
    AstNode *current_child = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena); // parse first child
    AstNode *head_child = current_child;
    while (tokeniser->last_returned.type == TokenType::COMMA) {
        tokeniser->next_token();
        AstNode *name = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        current_child->adjacent_child = name;
        current_child = current_child->adjacent_child;
    }

    parent.nary.children = head_child;
    return node_alloc(ast_arena, &parent, scope);
}

// TODO find out whether or not it would be better to store starred targets as
// nodes or simply have a boolean flag
static AstNode *parse_star_target(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {

    if (tokeniser->last_returned.type == TokenType::MULTIPLICATION) {
        tokeniser->next_token();
        AstNode starred_target = {};
        starred_target.type = AstNodeType::STARRED;
        starred_target.star_expression.expression = parse_primary(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

        return node_alloc(ast_arena, &starred_target, scope);
    }

    return parse_primary(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
}

static AstNode *parse_star_targets(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode *current_star_target = parse_star_target(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    AstNode *head_star_target = current_star_target;
    while (tokeniser->last_returned.type == TokenType::COMMA) {
        current_star_target->adjacent_child = parse_star_target(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        current_star_target = current_star_target->adjacent_child;
    }

    return head_star_target;
}

static AstNode *parse_function_call_arguments(Tokeniser *tokeniser,
                                              Arena *ast_arena,
                                              SymbolTableEntry *scope,
                                              CompilerTables *compiler_tables,
                                              Arena *symbol_table_arena) {
    AstNode *args_head;
    AstNode **arg = &args_head;
    while (tokeniser->last_returned.type != TokenType::CLOSED_PAREN) {
        if (tokeniser->last_returned.type == TokenType::NEWLINE) {
            printf("Expected Token ')' before newline\n");
            exit(1);
        }

        if (tokeniser->lookahead.type == TokenType::ASSIGN) {
            // check to see if the token before '=' is an identifier
            assert_token_and_print_debug(TokenType::IDENTIFIER,
                                            tokeniser->last_returned,
                                            "Keyword arguments must be valid identifiers\n");

            AstNode kwarg = {};
            kwarg.token = tokeniser->last_returned;
            kwarg.kwarg.name = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            kwarg.kwarg.expression = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

            if (!(kwarg.kwarg.expression)) {
              printf("Failed to parse function argument on line: %d col: %d\n",
                     tokeniser->last_returned.line,
                     tokeniser->last_returned.column);
              break;
            }

            *arg = node_alloc(ast_arena, &kwarg, scope);
        }

        else {
            *arg = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
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


        assert_comma_and_skip_over(tokeniser, ast_arena);
        arg = &((*arg)->adjacent_child);
    }

    tokeniser->next_token();
    return args_head;
}

// TODO: Python distinguishes between regular primary and those that end
// in just a list slice and attribute access
// See 'single_subscript_attribute_target'
//
static AstNode *parse_primary(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode *prev = nullptr;
    AstNode node = {};
    node.token = tokeniser->last_returned;

    AstNode *left = parse_atom(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

    // parse star primary
    // NOTE: What the fuck does this even mean
    // tokeniser->next_token();

    while (left != prev) {
        prev = left;
        left = parse_sub_primary(tokeniser, ast_arena, left, scope, compiler_tables, symbol_table_arena);
    }

    return left;
}

static AstNode *parse_sub_primary(Tokeniser *tokeniser, Arena *ast_arena,
                                  AstNode *left, SymbolTableEntry *scope,
                                  CompilerTables *compiler_tables, Arena *symbol_table_arena) {

    Token current_token = tokeniser->last_returned;

    if (current_token.type == TokenType::DOT) {
        AstNode attribute_ref = {.token = tokeniser->last_returned};
        tokeniser->next_token();

        assert_token_and_print_debug(TokenType::IDENTIFIER, tokeniser->last_returned, "");

        AstNode attribute = {.token = tokeniser->last_returned};

        attribute_ref.type = AstNodeType::ATTRIBUTE_REF;
        attribute_ref.attribute_ref.attribute =
            node_alloc(ast_arena, &attribute, scope);

        attribute_ref.attribute_ref.name = left;
        tokeniser->next_token();
        return node_alloc(ast_arena, &attribute_ref, scope);
    }

    else if (current_token.type == TokenType::OPEN_PAREN) {
        tokeniser->next_token();

        AstNode call = {.token = current_token,
                        .type = AstNodeType::FUNCTION_CALL};

        call.function_call.args = parse_function_call_arguments(tokeniser,
                                                                ast_arena, scope,
                                                                compiler_tables, symbol_table_arena);

        call.function_call.expression = left;
        tokeniser->next_token();
        left = node_alloc(ast_arena, &call, scope);
        return parse_sub_primary(tokeniser, ast_arena, left, scope, compiler_tables, symbol_table_arena);
    }

    else if (current_token.type == TokenType::SQUARE_OPEN_PAREN) {
        tokeniser->next_token();

        AstNode subscript = {.token = current_token,
                             .type = AstNodeType::SUBSCRIPT};
        AstNodeSubscript *subscript_proper = &subscript.subscript;

        AstNode **child = subscript_proper->children;
        while (tokeniser->last_returned.type != TokenType::SQUARE_CLOSED_PAREN) {
            if (tokeniser->last_returned.type == TokenType::NEWLINE) {
                printf("Expected Token ']' before newline\n");
                exit(1);
            }

            *child = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
            if (tokeniser->last_returned.type == TokenType::SQUARE_CLOSED_PAREN) {
            break;
            }

            assert_token_and_print_debug(
                TokenType::COLON, tokeniser->last_returned,
                "Subscript expressions must be seperated by a colon");

            tokeniser->next_token();
            child = ++child;
        }


        subscript_proper->expression = left;
        left = node_alloc(ast_arena, &subscript, scope);
        return parse_sub_primary(tokeniser, ast_arena, left, scope, compiler_tables, symbol_table_arena);
    }

    else {
        return left;
    }
}

static AstNode *parse_else(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    Token current_token = tokeniser->last_returned;
    switch (current_token.type) {
        case TokenType::ELSE: {
            AstNode else_node = {};
            else_node.token = current_token;
            else_node.type = AstNodeType::ELSE;
            AstNodeElse *else_node_proper = &else_node.else_stmt;
            assert_token_and_print_debug(TokenType::COLON, tokeniser->next_token(),
                                        "\n Error Parsing else satement");

            tokeniser->next_token(); // skip the colon

            else_node_proper->block  = parse_block(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            return node_alloc(ast_arena, &else_node, scope);
        }

        default:
            return nullptr;
    }
}

static AstNode *parse_elif(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    Token current_token = tokeniser->last_returned;
    switch (current_token.type)  {

        case TokenType::ELIF: {
            tokeniser->next_token();
            AstNode elif = {};
            elif.token = current_token;
            AstNodeIf *elif_proper = &elif.if_stmt;

            elif_proper->condition = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

            assert_token_and_print_debug(TokenType::COLON,
                                         tokeniser->last_returned,
                                         "\n Error Parsing elif satement");

            tokeniser->next_token(); // skip the colon

            elif_proper->block = parse_block(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            elif_proper->or_else = parse_elif(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            return node_alloc(ast_arena, &elif, scope);
        }

        default:
            return parse_else(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    }
}

static inline void assert_comma_and_skip_over(Tokeniser *tokeniser, Arena *ast_arena) {
    assert_token_and_print_debug(
        TokenType::COMMA, tokeniser->last_returned,
        "Function parameters must be seperated by commas");

    tokeniser->next_token();
}

static AstNode *parse_function_def_arguments(Tokeniser *tokeniser, Arena *ast_arena, AstNodeFunctionDef *function, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode *arg_head = nullptr;
    AstNode **arg = &arg_head;

    while (tokeniser->last_returned.type != TokenType::CLOSED_PAREN) {
        Token current_token = tokeniser->last_returned;
        AstNode argument_node = {};
        argument_node.token = current_token;

        // *args
        if (current_token.type == TokenType::MULTIPLICATION) {
            if (function->star) {
                printf("Syntax Error: Function can only contain one * argument\n");
                exit(1);
            }

            tokeniser->next_token();
            function->star = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

            if (tokeniser->last_returned.type == TokenType::CLOSED_PAREN) {
                break;
            }

            assert_comma_and_skip_over(tokeniser, ast_arena);
        }

        // **kwargs
        if (tokeniser->last_returned.type == TokenType::EXPONENTIATION) {
            tokeniser->next_token();
            function->double_star = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            assert_token_and_print_debug(
                TokenType::CLOSED_PAREN, tokeniser->last_returned,
                "** arguments cannot be followed by any additional arguments");
            break;
        }

        AstNodeDeclaration *param_proper = &argument_node.declaration;
        param_proper->name = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        SymbolTableValue value = {};
        value.symbol_type = SymbolType::VARIABLE;
        compiler_tables->function_table->insert(symbol_table_arena,
                             param_proper->name->token.value, scope,
                             value);

        assert_token_and_print_debug(
            TokenType::COLON, tokeniser->last_returned,
            "Function parameters require type signatures");

        tokeniser->next_token();

        param_proper->annotation = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

        // parse the assignment if there is one
        if (tokeniser->last_returned.type == TokenType::ASSIGN) {
            tokeniser->next_token();
            argument_node.type = AstNodeType::ASSIGNMENT;
            param_proper->expression = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
        }

        *arg = node_alloc(ast_arena, &argument_node, scope);
        arg = &((*arg)->adjacent_child);

        if (tokeniser->last_returned.type == TokenType::CLOSED_PAREN) {
            break;
        }

        assert_comma_and_skip_over(tokeniser, ast_arena);
    }

    tokeniser->next_token();
    return arg_head;
}

static AstNode *parse_block(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode block = {};
    block.token = tokeniser->last_returned;
    block.type = AstNodeType::BLOCK;
    AstNode **child = &block.block.children;

    assert_token_and_print_debug(TokenType::NEWLINE, tokeniser->last_returned,
                                 "Error in parsing block");

    assert_token_and_print_debug(TokenType::INDENT, tokeniser->next_token(),
                                 "Error in parsing block");

    tokeniser->next_token();

    while (tokeniser->last_returned.type != TokenType::DEDENT) {
        if (tokeniser->last_returned.type == TokenType::NEWLINE) {
            tokeniser->next_token();
            continue;
        }

        AstNode *statement = parse_statement(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);


        if (!statement) {
            printf("error parsing statment in block\n");
            debug_print_parse_tree(&block, 0);
            return nullptr;
        }

        *child = statement;
        child = &(*child)->adjacent_child;

    }

    tokeniser->next_token();
    return node_alloc(ast_arena, &block, scope);
}

static AstNode *parse_assignment_expression(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode node = {};
    node.token = tokeniser->last_returned;

    if (tokeniser->last_returned.type == TokenType::IDENTIFIER &&
        tokeniser->lookahead.type == TokenType::COLON) {

        node.type = AstNodeType::ASSIGNMENT;
        AstNodeAssignment *assignment = &node.assignment;
        assignment->name = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        assert_token_and_print_debug(TokenType::ASSIGN, tokeniser->next_token(),
                                     "\nError in parsing assignment on line: ");
        tokeniser->next_token();
        assignment->expression = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
        return node_alloc(ast_arena, &node, scope);
    }

    else {
        return parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
    }
}

static AstNode *parse_single_assignment_star_expression(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode node = {};
    node.token = tokeniser->last_returned;

    if (tokeniser->last_returned.type == TokenType::MULTIPLICATION) {
        return parse_star_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    }

    return parse_assignment_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
}

static AstNode *parse_assignment_star_expressions(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode *current_star_expression =
        parse_single_assignment_star_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    AstNode *head_star_expression = current_star_expression;
    while (tokeniser->last_returned.type == TokenType::COMMA) {
        tokeniser->next_token();
        current_star_expression->adjacent_child = parse_single_assignment_star_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        current_star_expression = current_star_expression->adjacent_child;
    }

    return head_star_expression;
}

static AstNode *parse_single_double_starred_kvpair(Tokeniser *tokeniser,
                                            Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {

    if (tokeniser->last_returned.type == TokenType::EXPONENTIATION) {
        AstNode double_starred = {};
        double_starred.type = AstNodeType::STARRED;
        double_starred.token = tokeniser->last_returned;
        AstNodeStarExpression *doule_starred_proper = &double_starred.star_expression;
        tokeniser->next_token();
        doule_starred_proper->expression =
            parse_bitwise_or_minimum_precedence(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

        return node_alloc(ast_arena, &double_starred, scope);
    }

    else {
        AstNode kvpair = {};
        kvpair.type = AstNodeType::KVPAIR;
        kvpair.token = tokeniser->last_returned;
        AstNodeKvPair *kvpair_proper = &kvpair.kvpair;
        kvpair_proper->key = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
        assert_token_and_print_debug(
            TokenType::COLON, tokeniser->last_returned,
            "Key Value pairs must be seperated by ':'");

        tokeniser->next_token();
        kvpair_proper->value = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

        return node_alloc(ast_arena, &kvpair, scope);
    }
}

static AstNode *parse_double_starred_kvpairs(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode *current_kvpair = parse_single_double_starred_kvpair(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    AstNode *head_kvpair = current_kvpair;
    while (tokeniser->last_returned.type == TokenType::COMMA) {
        tokeniser->next_token();
        current_kvpair = parse_single_double_starred_kvpair(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        current_kvpair = current_kvpair->adjacent_child;
    }

    return head_kvpair;
}

static AstNode *parse_for_if_clause(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode for_if = {};
    for_if.type = AstNodeType::FOR_IF;
    AstNodeForIfClause *for_if_proper = &for_if.for_if;
    for_if_proper->targets = parse_star_targets(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    for_if_proper->expression = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

    if (tokeniser->last_returned.type == TokenType::IF) {
        tokeniser->next_token();
        for_if_proper->if_clause = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
    }

    return node_alloc(ast_arena, &for_if, scope);
}

static AstNode *parse_atom(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    //parse list
    AstNode node = {};
    if (tokeniser->last_returned.type == TokenType::SQUARE_OPEN_PAREN) {
        node.token = tokeniser->last_returned;
        node.type = AstNodeType::LIST;

        tokeniser->next_token();
        node.nary.children = parse_single_assignment_star_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

        if (tokeniser->last_returned.type == TokenType::FOR) {
            tokeniser->next_token();
            node.type = AstNodeType::LISTCOMP;
            node.nary.children = parse_for_if_clause(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        }

        if (tokeniser->last_returned.type == TokenType::COMMA) {
            tokeniser->next_token();
            node.nary.children->adjacent_child = parse_assignment_star_expressions(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        }


        assert_token_and_print_debug(TokenType::SQUARE_CLOSED_PAREN,
                                     tokeniser->last_returned,
                                     "Mismatched parenthesis in list");
        tokeniser->next_token();
        return node_alloc(ast_arena, &node, scope);
    }

    else if (tokeniser->last_returned.type == TokenType::OPEN_PAREN) {
        node.token = tokeniser->last_returned;
        node.type = AstNodeType::TUPLE;
        node.nary.children = parse_assignment_star_expressions(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        return node_alloc(ast_arena, &node, scope);
    }

    else if (tokeniser->last_returned.type == TokenType::CURLY_OPEN_PAREN) {
        node.token = tokeniser->last_returned;
        node.type = AstNodeType::DICT;
        tokeniser->next_token();
        AstNode first_child {};
        first_child.token = tokeniser->last_returned;
        AstNode *expression = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

        if (tokeniser->last_returned.type == TokenType::EXPONENTIATION) {
            node.nary.children = parse_double_starred_kvpairs(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            return node_alloc(ast_arena, &node, scope);
        }


        // parse set definition and first assignment expression manually
        else if (tokeniser->last_returned.type == TokenType::COLON &&
            tokeniser->lookahead.type == TokenType::ASSIGN) {

            tokeniser->next_token();
            tokeniser->next_token();

            first_child.type = AstNodeType::ASSIGNMENT;
            AstNodeAssignment *assignment = &first_child.assignment;
            assignment->name = expression;
            assignment->expression = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

            if (tokeniser->last_returned.type == TokenType::FOR) {
                node.type = AstNodeType::DICTCOMP;
                tokeniser->next_token();
                first_child.adjacent_child = parse_for_if_clause(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            }

            else if (tokeniser->last_returned.type == TokenType::COMMA) {
                assert_token_and_print_debug(TokenType::CURLY_CLOSED_PAREN,
                                             tokeniser->last_returned,
                                             "Mismatched '}' in set definition");

            }

            else {
                tokeniser->next_token();
                first_child.adjacent_child =
                    parse_assignment_star_expressions(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            }

        }

        //parse dictionary definition
        else if (tokeniser->last_returned.type == TokenType::COLON) {
            tokeniser->next_token();
            first_child.type = AstNodeType::KVPAIR;
            AstNodeKvPair *kvpair = &first_child.kvpair;

            kvpair->key = expression;
            kvpair->value = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

            if (tokeniser->last_returned.type == TokenType::FOR) {
                tokeniser->next_token();
                first_child.adjacent_child = parse_for_if_clause(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            }

            if (tokeniser->last_returned.type == TokenType::COMMA) {
                tokeniser->next_token();
                first_child.adjacent_child =
                    parse_double_starred_kvpairs(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

            }

            assert_token_and_print_debug(TokenType::CURLY_CLOSED_PAREN,
                                         tokeniser->last_returned,
                                         "Mismatched '}' in set definition");
            tokeniser->next_token();

        }

        // parse set definition
        else {
            if (tokeniser->last_returned.type == TokenType::FOR) {
                tokeniser->next_token();
                first_child.adjacent_child = parse_for_if_clause(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            }

            else {
                first_child.adjacent_child =
                    parse_assignment_star_expressions(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            }
        }

        node.nary.children = node_alloc(ast_arena, &first_child, scope);
        return node_alloc(ast_arena, &node, scope);
    }

    else if (tokeniser->last_returned.is_literal()) {
        node.token = tokeniser->last_returned;
        if (node.token.type == TokenType::IDENTIFIER) {

            SymbolTableValue value = {};
            value.symbol_type = SymbolType::VARIABLE;
            value.static_type.type = TypeInfoType::UNKNOWN;

        }

        tokeniser->next_token();

        return node_alloc(ast_arena, &node, scope);
    }

    else {
        return nullptr;
    }
}

static inline AstNode *parse_left(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode *left = nullptr;
    if (tokeniser->last_returned.type == TokenType::OPEN_PAREN) {
        tokeniser->next_token();
        left = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

        // parse elsements into tuple
        AstNode tuple = {};
        tuple.type = AstNodeType::TUPLE;

        if (tokeniser->last_returned.type == TokenType::COMMA) {
            tokeniser->next_token();
            tuple.nary.children = left;
            left->adjacent_child = parse_assignment_star_expressions(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            left = node_alloc(ast_arena, &tuple, scope);
        }

        // parse assignment_expression_into_tuple
        else if (tokeniser->last_returned.type == TokenType::COLON &&
                 tokeniser->lookahead.type == TokenType::ASSIGN) {

            tokeniser->next_token();
            tokeniser->next_token();

            AstNode assignment = {};
            assignment.type = AstNodeType::ASSIGNMENT;
            AstNodeAssignment *assignment_proper = &assignment.assignment;
            assignment_proper->name = left;
            assignment_proper->expression = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

            tuple.nary.children = node_alloc(ast_arena, &assignment, scope);
            left->adjacent_child = parse_assignment_star_expressions(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

            left = node_alloc(ast_arena, &tuple, scope);
        }

        // parse as if it was an expression that has just been bracketed
        else if (tokeniser->last_returned.type == TokenType::CLOSED_PAREN &&
                 tokeniser->lookahead.is_binary_op()) {

            tokeniser->next_token();
        }


        else {
            assert_token_and_print_debug(
                TokenType::CLOSED_PAREN, tokeniser->last_returned,
                "Mismatched parenthesis in tuple declaration");
            tokeniser->next_token();
        }

    }

    else {
        left = parse_primary(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    }

    return left;

}

// NOTE: this is exactly the same as parse_inc_precedence except the precedence cannot be less
// than bitwise_or expressions this is used for python star expressions
static AstNode *parse_inc_precedence_minimum_bitwise_or_precedence(Tokeniser *tokeniser,
                                                                   Arena *ast_arena,
                                                                   AstNode *left,
                                                                   SymbolTableEntry *scope,
                                                                   CompilerTables *compiler_tables,
                                                                   Arena *symbol_table_arena,
                                                                   int min_precedence) {

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
        AstNode *right = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, current_precedence);

        if(!right) {
            printf("UNEXPECTED END OF TOKEN STREAM\n");
            return nullptr;
        }

        AstNode binary_op_node = AstNode::create_binary(current_token,
                                                        left, right);
        return node_alloc(ast_arena, &binary_op_node, scope);
    }

    else {
        return left;
    }
}

static AstNode *parse_bitwise_or_minimum_precedence(Tokeniser *tokeniser,
                                                    Arena *ast_arena,
                                                    SymbolTableEntry *scope,
                                                    CompilerTables *compiler_tables,
                                                    Arena *symbol_table_arena,
                                                    int min_precedence) {

    AstNode *left = parse_left(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    // parse star primary

    while (true) {
        AstNode *node = parse_inc_precedence_minimum_bitwise_or_precedence(
            tokeniser, ast_arena, left, scope, compiler_tables, symbol_table_arena, min_precedence);

        if (node == left) {
            break;
        }

        left = node;
    }

    return left;
}

static AstNode *parse_star_expression(Tokeniser *tokeniser,
                                      Arena *ast_arena,
                                      SymbolTableEntry *scope,
                                      CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    if (tokeniser->last_returned.type != TokenType::MULTIPLICATION) {
        return parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
    }

    return parse_bitwise_or_minimum_precedence(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
}

static AstNode *parse_increasing_precedence(Tokeniser *tokeniser, Arena *ast_arena,
                                            AstNode *left, SymbolTableEntry *scope,
                                            CompilerTables *compiler_tables,
                                            Arena *symbol_table_arena,
                                            int min_precedence) {

    Token current_token = tokeniser->last_returned;

    if (current_token.type == TokenType::ENDFILE) {
        return left;
    }

    if (current_token.type == TokenType::OPEN_PAREN) {
        tokeniser->next_token();
        parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
    }


    if (!current_token.is_binary_op()) {
        return left;
    }

    int current_precedence = current_token.precedence();
    if (current_precedence > min_precedence) {
        tokeniser->next_token();
        AstNode *right = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, current_precedence);

        if(!right) {
            printf("UNEXPECTED END OF TOKEN STREAM\n");
            return nullptr;
        }

        AstNode binary_op_node = AstNode::create_binary(current_token,
                                                        left, right);
        return node_alloc(ast_arena, &binary_op_node, scope);
    }

    else {
        return left;
    }
}

static AstNode *parse_expression(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope,
                                 CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena,
                                 int min_precedence) {

    AstNode *left = parse_left(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    // parse star primary

    while (true) {
        AstNode *node =
            parse_increasing_precedence(tokeniser, ast_arena, left, scope, compiler_tables, symbol_table_arena, min_precedence);
        if (node == left) {
            break;
        }

        left = node;
    }

    return left;
}

static AstNode *parse_assignment(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode node = {};
    node.token = tokeniser->last_returned;

    if (tokeniser->lookahead.type == TokenType::ASSIGN) {
        node.type = AstNodeType::ASSIGNMENT;
        AstNodeAssignment *assignment = &node.assignment;
        assignment->name = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

        assert_token_and_print_debug(
            TokenType::ASSIGN, tokeniser->last_returned,
            "\nError in parsing assignment on line: ");
        tokeniser->next_token();

        assignment->expression = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
    }

    else if (tokeniser->lookahead.type == TokenType::COLON) {
        node.type = AstNodeType::DECLARATION;
        AstNodeDeclaration *declaration = &node.declaration;
        declaration->name = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

        SymbolTableValue value = {};
        value.symbol_type = SymbolType::VARIABLE;
        compiler_tables->variable_table->insert(symbol_table_arena,
                                                declaration->name->token.value,
                                                scope, value);

        tokeniser->next_token();
        declaration->annotation = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

        if ( tokeniser->last_returned.type == TokenType::ASSIGN) {
            tokeniser->next_token();
            declaration->expression = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
        }
    }

    return node_alloc(ast_arena, &node, scope);
}

static inline AstNode *parse_next_star_expression_into_child(
    Tokeniser *tokeniser,
    Arena *ast_arena,
    SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {

    Token current_token = tokeniser->last_returned;
    tokeniser->next_token();
    if (tokeniser->last_returned.type == TokenType::NEWLINE) {
        AstNode node = AstNode::create_unary(current_token, nullptr);
        return node_alloc(ast_arena, &node, scope);
    }

    AstNode *child = parse_star_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    AstNode node = AstNode::create_unary(current_token, child);

    return node_alloc(ast_arena, &node, scope);
}

static inline AstNode *parse_next_single_star_expression_into_child(
    Tokeniser* tokeniser,
    Arena *ast_arena,
    SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {

    Token current_token = tokeniser->last_returned;
    tokeniser->next_token();
    if (tokeniser->last_returned.type == TokenType::NEWLINE) {
        AstNode node = AstNode::create_unary(current_token, nullptr);
        return node_alloc(ast_arena, &node, scope);
    }

    AstNode *child = parse_star_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    AstNode node = AstNode::create_unary(current_token, child);

    return node_alloc(ast_arena, &node, scope);
}

static inline AstNode *parse_next_star_expressions_into_children(Tokeniser *tokeniser,
                                                                 Arena *ast_arena,
                                                                 SymbolTableEntry *scope,
                                                                 CompilerTables *compiler_tables,
                                                                 Arena *symbol_table_arena,
                                                                 Token token) {

    AstNode node = {};
    node.token = token;
    node.type = AstNodeType::UNARY;
    tokeniser->next_token();
    AstNode *child = parse_star_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    node.unary.child = child;

    if (tokeniser->last_returned.type == TokenType::COMMA &&
        tokeniser->lookahead.type != TokenType::NEWLINE) {
        AstNode tuple = {};
        tuple.type = AstNodeType::TUPLE;
        tuple.tuple.children = child;
        AstNode *next_child = tuple.tuple.children->adjacent_child;

        while (tokeniser->last_returned.type == TokenType::COMMA) {
            // if there is more than one return value then make it a tuple
            tokeniser->next_token();
            next_child = parse_star_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            next_child = next_child->adjacent_child;
        }

        node.unary.child = node_alloc(ast_arena, &tuple, scope);
    }

    return node_alloc(ast_arena, &node, scope);;
}

static AstNode *parse_function_def(Tokeniser *tokeniser, Arena *ast_arena, SymbolTableEntry *scope, CompilerTables *compiler_tables, Arena *symbol_table_arena) {
    AstNode node = {};
    node.token = tokeniser->last_returned;
    node.type = AstNodeType::FUNCTION_DEF;
    AstNodeFunctionDef *function_proper = &node.function_def;

    assert_token_and_print_debug(TokenType::IDENTIFIER, tokeniser->next_token(),
                                 "\n Error parsing function definition");

    function_proper->name = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    SymbolTableValue value = {};
    value.symbol_type = SymbolType::FUNCTION;
    value.static_type.type = TypeInfoType::UNKNOWN;

    compiler_tables->function_table->insert(
        symbol_table_arena, function_proper->name->token.value, scope, value);

    assert_token_and_print_debug(TokenType::OPEN_PAREN,
                                 tokeniser->last_returned,
                                 "\n Error parsing function definition");
    tokeniser->next_token();

    SymbolTableEntry *last_scope = scope;
    scope = compiler_tables->function_table->lookup(
        function_proper->name->token.value, scope);

    function_proper->arguments = parse_function_def_arguments(
        tokeniser, ast_arena, function_proper, scope, compiler_tables,
        symbol_table_arena);

    assert_token_and_print_debug(TokenType::ARROW, tokeniser->last_returned,
                                 "Error parsing function definition");

    tokeniser->next_token();
    function_proper->return_type = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

    assert_token_and_print_debug(TokenType::COLON, tokeniser->last_returned,
                                 "Error parsing function definition");

    tokeniser->next_token();
    function_proper->block = parse_block(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    scope = last_scope;

    return node_alloc(ast_arena, &node, scope);
}
static AstNode *parse_dotted_name(Tokeniser *tokeniser, Arena *ast_arena,
                                  SymbolTableEntry *scope, CompilerTables *compiler_tables,
                                  Arena *symbol_table_arena) {

    AstNode *left = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    AstNode *right;
    Token next_token = tokeniser->last_returned;
    if (next_token.type == TokenType::DOT) {
        tokeniser->next_token();
        right = parse_dotted_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    }

    else {
        return left;
    }

    AstNode binary = AstNode::create_binary(next_token, left, right);
    return node_alloc(ast_arena, &binary, scope);
}

static AstNode *parse_class_def(Tokeniser *tokeniser, Arena *ast_arena,
                                SymbolTableEntry *scope, CompilerTables *compiler_tables,
                                Arena *symbol_table_arena) {

    AstNode node = {};
    node.token = tokeniser->last_returned;
    node.type = AstNodeType::CLASS_DEF;
    AstNodeClassDef *class_node = &node.class_def;



    assert_token_and_print_debug(TokenType::IDENTIFIER, tokeniser->next_token(),
                                 "\n Error parsing class_node definition");

    class_node->name = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    SymbolTableValue value = {};
    value.symbol_type = SymbolType::CLASS;
    value.symbol_type = SymbolType::CLASS;
    compiler_tables->class_table->insert(symbol_table_arena,
                         class_node->name->token.value,
                         scope, value);

    if ( tokeniser->last_returned.type == TokenType::OPEN_PAREN) {
        class_node->arguments = parse_function_call_arguments(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    }

    assert_token_and_print_debug(TokenType::COLON, tokeniser->last_returned,
                                 "Error parsing class_node definition");
    tokeniser->next_token();
    SymbolTableEntry *last_scope = scope;
    scope =
        compiler_tables->class_table
            ->lookup(class_node->name->token.value, scope);
                class_node->block = parse_block(
            tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    scope = last_scope;

    return node_alloc(ast_arena, &node, scope);
}

static AstNode *parse_statement(Tokeniser *tokeniser, Arena *ast_arena,
                                SymbolTableEntry *scope, CompilerTables *compiler_tables,
                                Arena *symbol_table_arena) {

    Token current_token = tokeniser->last_returned;

    if (current_token.type == TokenType::ENDFILE) {
        return nullptr;
    }

    AstNode node = {};
    node.token = current_token;

    switch (current_token.type) {

        case TokenType::IDENTIFIER: {
            if (tokeniser->lookahead.type != TokenType::COLON) {
                return parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);
            }

            return parse_assignment(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

        } break;

        case TokenType::IMPORT: {
            node.type = AstNodeType::IMPORT;
            AstNodeImport *import_node = &node.import;
            tokeniser->next_token();
            import_node->target = parse_dotted_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

            if (tokeniser->last_returned.type == TokenType::AS) {
                tokeniser->next_token();
                import_node->as = parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            }

        } break;

        case TokenType::AT: {
            AstNode *head_decorator = nullptr;
            AstNode **decorator = &head_decorator;
            while (tokeniser->last_returned.type == TokenType::AT) {
                tokeniser->next_token();
                *decorator = parse_assignment_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
                assert_token_and_print_debug(
                    TokenType::NEWLINE, tokeniser->last_returned,
                    "expected newline after decarator declaration");

                decorator = &((*decorator)->adjacent_child);
                tokeniser->next_token();
            }

            if (tokeniser->last_returned.type == TokenType::CLASS) {
                AstNode *class_def = parse_class_def(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
                class_def->class_def.decarators = head_decorator;
                return class_def;
            }

            if (tokeniser->last_returned.type == TokenType::DEF) {
                AstNode *funcion_def = parse_function_def(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
                funcion_def->function_def.decarators = head_decorator;
                return funcion_def;
            }
        }

        case TokenType::IF: {
            tokeniser->next_token();
            node.type = AstNodeType::IF;
            AstNodeIf *if_stmt = &node.if_stmt;
            if_stmt->condition = parse_assignment_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            assert_token_and_print_debug(TokenType::COLON,
                                         tokeniser->last_returned,
                                         "\n Error Parsing If Satement");
            tokeniser->next_token();

            if_stmt->block = parse_block(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            if (!if_stmt->block) {
                printf("Failed to parse block inside if on line: %d\n", current_token.line);
                exit(1);
            }

            if_stmt->or_else = parse_elif(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

        } break;

        case TokenType::WHILE: {
            node.type = AstNodeType::WHILE;
            AstNodeWhile *while_node = &node.while_loop;
            tokeniser->next_token();
            while_node->condition = parse_assignment_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

            assert_token_and_print_debug(TokenType::COLON,
                                         tokeniser->last_returned,
                                         "Error parsing while statment");

            tokeniser->next_token();
            while_node->block = parse_block(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            while_node->or_else = parse_else(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        } break;

        //TODO: find out what the hell a star_target is in the python grammar
        case TokenType::FOR: {
            node.type = AstNodeType::FOR_LOOP;
            node.token = tokeniser->last_returned;
            AstNodeForLoop *for_node = &node.for_loop;
            tokeniser->next_token();

            for_node->targets = parse_star_targets(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

            assert_token_and_print_debug(TokenType::IN_TOK,
                                         tokeniser->next_token(),
                                         "Error parsing for statment");

            tokeniser->next_token();
            for_node->expression = parse_star_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

            assert_token_and_print_debug(
                TokenType::COLON, tokeniser->last_returned,
                "expected colon after the end of for statement");

            tokeniser->next_token();
            for_node->block = parse_block(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            for_node->or_else = parse_else(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        } break;

        case TokenType::TRY: {
            node.type = AstNodeType::TRY;
            AstNodeTry *try_node = &node.try_node;

            assert_token_and_print_debug(
                TokenType::COLON, tokeniser->next_token(),
                "Expected colon at the end of try statment");

            tokeniser->next_token();
            try_node->block = parse_block(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

            // Parse except handlers
            AstNode **handler = &try_node->handlers;
            while (tokeniser->last_returned.type != TokenType::ENDFILE) {
                Token current_token = tokeniser->last_returned;

                if (!(current_token.type == TokenType::EXCEPT)) {
                    break;
                }

                AstNode except = {.token = current_token, .type = AstNodeType::EXCEPT};
                AstNodeExcept *except_proper = &except.except;

                if (!(tokeniser->next_token().type == TokenType::COLON)) {
                    except_proper->expression = parse_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena, 0);

                    if (tokeniser->last_returned.type == TokenType::AS) {
                        parse_name(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
                    }

                    assert_token_and_print_debug(
                        TokenType::COLON, tokeniser->last_returned,
                        "\n Error Parsing except satement");
                }

                tokeniser->next_token();
                except_proper->block = parse_block(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

                *handler = node_alloc(ast_arena, &except, scope);
                handler = &((*handler)->adjacent_child);
            }

            // parse else
            try_node->or_else = parse_else(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);

            // parse finally statement
            if (tokeniser->last_returned.type == TokenType::FINALLY) {
                assert_token_and_print_debug(
                    TokenType::COLON, tokeniser->next_token(),
                    "\n Error Parsing finally statement");

                tokeniser->next_token();
                try_node->finally = parse_block(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
            }

        } break;

        case TokenType::DEF: {
            return parse_function_def(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        }

        case TokenType::CLASS: {
            return parse_class_def(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        }

        case TokenType::RETURN: {
            return parse_next_star_expressions_into_children(
                tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena,
                tokeniser->last_returned);
        }

        case TokenType::YIELD: {
            return parse_next_star_expressions_into_children(
                tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena,
                tokeniser->last_returned);
        }

        case TokenType::RAISE: {
            if (tokeniser->lookahead.type != TokenType::NEWLINE) {
                return parse_next_single_star_expression_into_child(
                    tokeniser, ast_arena, scope, compiler_tables,
                    symbol_table_arena);
            }

            return node_alloc(ast_arena, &node, scope);
        }

        case TokenType::GLOBAL: {
            return parse_list_of_names_and_return_modifier(tokeniser,
                                                           ast_arena,
                                                           scope,
                                                           compiler_tables,
                                                           symbol_table_arena,
                                                           current_token);
        }

        case TokenType::NONLOCAL: {
            return parse_list_of_names_and_return_modifier(tokeniser,
                                                           ast_arena,
                                                           scope,
                                                           compiler_tables,
                                                           symbol_table_arena,
                                                           current_token);
        }

        case TokenType::PASS: {
            return parse_single_token_into_node(
                tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        }

        case TokenType::CONTINUE: {
            return parse_single_token_into_node(
                tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        }

        case TokenType::BREAK: {
            return parse_single_token_into_node(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        }

        default:
            return parse_star_expression(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
    }

    return node_alloc(ast_arena, &node, scope);
}

static AstNode *parse_statements(Tokeniser *tokeniser, Arena *ast_arena,
                                 SymbolTableEntry *scope, CompilerTables *compiler_tables,
                                 Arena *symbol_table_arena) {
    AstNode file_node = {};
    file_node.token = tokeniser->last_returned;
    file_node.type = AstNodeType::FILE;
    AstNode **child = &file_node.file.children;

    while (tokeniser->last_returned.type != TokenType::ENDFILE) {
        if (tokeniser->last_returned.type == TokenType::NEWLINE) {
            tokeniser->next_token();
            continue;
        }

        AstNode *statement = parse_statement(tokeniser, ast_arena, scope, compiler_tables, symbol_table_arena);
        if (!statement) {
            printf("PARSING ERROR\n");
            debug_print_parse_tree(&file_node, 0);
            break;
        }

        *child = statement;
        child = &((*child)->adjacent_child);

    }

    return node_alloc(ast_arena, &file_node, scope);
}
