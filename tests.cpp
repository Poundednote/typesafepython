#include <stdio.h>
#include <iostream>

#include "tokeniser.cpp"
#include "parser.cpp"
#include "typing.cpp"

#define PARSER_TESTS 1

#define INIT_MAIN() Test TEST = {};
#define START_TEST() Test actual_test = {}; Test *test = &actual_test;
#define END_TEST() test->passed = true; return *test;
#define TEST(test) TEST = test(); if (!(TEST.passed)) {printf("\nfailed case: %d on line: %d in %s\n", TEST.cases, TEST.line, #test); return 1;}

#define ASSERT(condition, info) \
  test->line = __LINE__;                                                       \
  if (!(condition)) {                                                          \
    std::cout << "Expected condition: " << #condition " Aditional info: " << info << std::endl; \
    test->passed = false;                                                      \
    return *test;                                                              \
  } else {                                                                     \
    ++test->cases;                                                             \
  }

std::string additional_info = "";

//TODO test precedence programtically
//TODO compund stmt tetss
//TODO primary tests
//TODO target tests

struct Test {
    int cases = 1;
    bool passed = false;
    int line;
};

InputStream input_stream_create_from_string(const char *string) {
    InputStream input_stream = {};
    input_stream.contents = (char *)string;
    input_stream.size = strlen(string);

    return input_stream;
}

static Test tokenise_file_test() {
    START_TEST()
        const char *mock_file = "or and not == != <= <>= > is in |^&<<>>+-* / // % ** . return yield raise global nonlocal if elif else def class while for try except finally as pass break continue identifier 123 123.321 \"string\" None True False ()[]{},=:->@import\n";
    InputStream input_stream = input_stream_create_from_string(mock_file);
    Tokeniser tokeniser = Tokeniser::init(&input_stream);

    int i = 1;
    for (Token current = tokeniser.last_returned;
         i < 63;
         current = tokeniser.next_token(), ++i) {


        if ((int)current.type != i) {
            printf("token: %s \nline: %d col: %d \ndidn't match with expected for %s",
                   debug_token_type_to_string(current.type).c_str(),
                   current.line, current.column,
                   debug_token_type_to_string(*(enum TokenType *)(&i)).c_str());



            return *test;
        }

        if (current.type == TokenType::INT_LIT) {
            if (current.value != "123") {
              printf("INT token with value: '%s' failed to match with expected value of: 123\n",
                     current.value.c_str());

              return *test;
          }
        }

        if (current.type == TokenType::FLOAT_LIT) {
            if (current.value != "123.321") {
              printf("INT token with value: '%s' failed to match with expected "
                     "value of: 123.321\n",
                     current.value.c_str());

              return *test;
          }
        }

        if (current.type == TokenType::STRING_LIT) {
            if (current.value != "string") {

              printf("String token with value: '%s' failed to match with expected "
                     "value of: string", current.value.c_str());

            return *test;
          }
        }

        if (current.type == TokenType::IDENTIFIER) {
            if (current.value != "identifier") {
            printf("identifer token with value: '%s' failed to match with "
                   "expected value of: identifier\n",
                   current.value.c_str());

            return *test;
          }
        }

        ++test->cases;
    }


    END_TEST()
}

static Test tokenise_file_indentation_test() {
    START_TEST()
    const char *mock_file = "\n    \n        ";
    InputStream input_stream = input_stream_create_from_string(mock_file);
    Tokeniser tokeniser = Tokeniser::init(&input_stream);

    Token current = tokeniser.next_token();
    ASSERT(current.type == TokenType::INDENT, debug_token_type_to_string(current.type).c_str());
    tokeniser.next_token();
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::INDENT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::DEDENT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::DEDENT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::ENDFILE, debug_token_type_to_string(current.type).c_str());

    mock_file = "\n        \n    3+3\n";

    input_stream = input_stream_create_from_string(mock_file);
    tokeniser = Tokeniser::init(&input_stream);

    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::INDENT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::INDENT, debug_token_type_to_string(current.type).c_str());
    tokeniser.next_token();

    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::DEDENT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::INT_LIT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::ADDITION, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::INT_LIT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::NEWLINE, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::DEDENT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::ENDFILE, debug_token_type_to_string(current.type).c_str());

    mock_file = "\n        \n3+3\n";
    input_stream = input_stream_create_from_string(mock_file);
    tokeniser = Tokeniser::init(&input_stream);
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::INDENT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::INDENT, debug_token_type_to_string(current.type).c_str());
    tokeniser.next_token();

    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::DEDENT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::DEDENT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::INT_LIT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::ADDITION, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::INT_LIT, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::NEWLINE, debug_token_type_to_string(current.type).c_str());
    current = tokeniser.next_token();
    ASSERT(current.type == TokenType::ENDFILE, debug_token_type_to_string(current.type).c_str());


    END_TEST();
}

#if PARSER_TESTS

static Test floats_and_numbers_types_test() {
    START_TEST();
    const char *mock_file = "3 + 3\n3 - 3.0\n3.0 * 3\n3.0 - 3.0\n3 / 3\n 3 // 3";
    InputStream input_stream = input_stream_create_from_string(mock_file);

    if (!input_stream.contents) {
        exit(1);
    }

    Tokeniser tokeniser = Tokeniser::init(&input_stream);
    Arena ast_arena = Arena::init(GIGABYTES(32));

    Arena symbol_table_arena = Arena::init(GIGABYTES(1));

    CompilerTables tables = CompilerTables::init(&symbol_table_arena);

    SymbolTableValue main_symbol_value = {};
    main_symbol_value.static_type.type = TypeInfoType::INTEGER;

    std::string main_identifier = "main";
    SymbolTableEntry *main_scope = tables.function_table->insert(
        &symbol_table_arena, main_identifier, 0, main_symbol_value);

    AstNode *root = parse_statements(&tokeniser, &ast_arena, main_scope, &tables, &symbol_table_arena);
    AstNode *child = root->nary.children;

    Arena scope_stack = Arena();
    type_parse_tree(root, &scope_stack, &tables, tables.variable_table);
    scope_stack.destroy();

    ASSERT(child->static_type.type == TypeInfoType::INTEGER, debug_static_type_to_string(child->static_type).c_str());
    child = child->adjacent_child;
    ASSERT(child->static_type.type == TypeInfoType::FLOAT, debug_static_type_to_string(child->static_type).c_str());
    child = child->adjacent_child;
    ASSERT(child->static_type.type == TypeInfoType::FLOAT, debug_static_type_to_string(child->static_type).c_str());
    child = child->adjacent_child;
    ASSERT(child->static_type.type == TypeInfoType::FLOAT, debug_static_type_to_string(child->static_type).c_str());
    child = child->adjacent_child;
    ASSERT(child->static_type.type == TypeInfoType::FLOAT, debug_static_type_to_string(child->static_type).c_str());
    child = child->adjacent_child;
    ASSERT(child->static_type.type == TypeInfoType::INTEGER, debug_static_type_to_string(child->static_type).c_str());

    ast_arena.destroy();
    symbol_table_arena.destroy();

    END_TEST();
}

static Test ifelse_test() {
    START_TEST();
    InputStream input_stream = InputStream::create_from_file("tests/ifelse.py");
    if (!input_stream.contents) {exit(1);}
    Tokeniser tokeniser = Tokeniser::init(&input_stream);
    Arena ast_arena = Arena::init(GIGABYTES(32));

    Arena symbol_table_arena = Arena::init(GIGABYTES(1));

    CompilerTables tables = CompilerTables::init(&symbol_table_arena);

    SymbolTableValue main_symbol_value = {};
    main_symbol_value.static_type.type = TypeInfoType::INTEGER;

    std::string main_identifier = "main";
    SymbolTableEntry *main_scope = tables.function_table->insert(
        &symbol_table_arena, main_identifier, 0, main_symbol_value);

    AstNode *root = parse_statements(&tokeniser, &ast_arena, main_scope, &tables, &symbol_table_arena);
    AstNode *statement = root->nary.children;
    ASSERT(statement->token.type == TokenType::IF, debug_token_type_to_string(statement->token.type).c_str());

    statement = statement->adjacent_child;
    ASSERT(statement->token.type == TokenType::IF, debug_token_type_to_string(statement->token.type).c_str());
    ASSERT(statement->if_stmt.condition->token.type == TokenType::GT, debug_token_type_to_string(statement->if_stmt.condition->token.type).c_str());
    ASSERT(statement->if_stmt.or_else->token.type == TokenType::ELIF, debug_token_type_to_string(statement->if_stmt.or_else->token.type).c_str());

    statement = statement->adjacent_child;
    ASSERT(statement->token.type == TokenType::IF, debug_token_type_to_string(statement->token.type).c_str());
    ASSERT(statement->if_stmt.condition->token.type == TokenType::GT, debug_token_type_to_string(statement->if_stmt.condition->token.type).c_str());
    ASSERT(statement->if_stmt.or_else->token.type == TokenType::ELSE, debug_token_type_to_string(statement->if_stmt.or_else->token.type).c_str());

    statement = statement->adjacent_child;
    ASSERT(statement->token.type == TokenType::IF, debug_token_type_to_string(statement->token.type).c_str());
    ASSERT(statement->if_stmt.condition->token.type == TokenType::GT, debug_token_type_to_string(statement->if_stmt.condition->token.type).c_str());
    ASSERT(statement->if_stmt.or_else->token.type == TokenType::ELIF, debug_token_type_to_string(statement->if_stmt.or_else->token.type).c_str());
    AstNode *or_else = statement->if_stmt.or_else->if_stmt.or_else;
    ASSERT(or_else->token.type == TokenType::ELSE, debug_token_type_to_string(or_else->token.type).c_str());

    statement = statement->adjacent_child;
    ASSERT(statement->token.type == TokenType::IF, debug_token_type_to_string(statement->token.type).c_str());
    ASSERT(statement->if_stmt.condition->token.type == TokenType::GT, debug_token_type_to_string(statement->if_stmt.condition->token.type).c_str());
    ASSERT(statement->if_stmt.or_else->token.type == TokenType::ELIF, debug_token_type_to_string(statement->if_stmt.or_else->token.type).c_str());
    or_else = statement->if_stmt.or_else->if_stmt.or_else;
    ASSERT(or_else->token.type == TokenType::ELIF, debug_token_type_to_string(or_else->token.type).c_str());
    ASSERT(or_else->if_stmt.condition->token.type == TokenType::GT, debug_token_type_to_string(or_else->if_stmt.condition->token.type).c_str());
    or_else = or_else->if_stmt.or_else;
    ASSERT(or_else->token.type == TokenType::ELSE, debug_token_type_to_string(or_else->token.type).c_str());

    ast_arena.destroy();
    symbol_table_arena.destroy();
    input_stream.destroy();

    END_TEST();
}

static Test functiondef_test() {
    START_TEST();
    InputStream input_stream = InputStream::create_from_file("tests/functiondef.py");
    if (!input_stream.contents) {exit(1);}
    Tokeniser tokeniser = Tokeniser::init(&input_stream);
    Arena ast_arena = Arena::init(GIGABYTES(32));

    Arena symbol_table_arena = Arena::init(GIGABYTES(1));

    CompilerTables tables = CompilerTables::init(&symbol_table_arena);

    SymbolTableValue main_symbol_value = {};
    main_symbol_value.static_type.type = TypeInfoType::INTEGER;

    std::string main_identifier = "main";
    SymbolTableEntry *main_scope = tables.function_table->insert(
        &symbol_table_arena, main_identifier, 0, main_symbol_value);

    AstNode *root = parse_statements(&tokeniser, &ast_arena, main_scope, &tables, &symbol_table_arena);
    AstNode *statement = root->nary.children;
    ASSERT(statement->type == AstNodeType::FUNCTION_DEF,
           "NOT FUNCTIONDEF");

    ASSERT(statement->token.type == TokenType::DEF,
           debug_token_type_to_string(statement->token.type));

    ASSERT(statement->function_def.arguments == nullptr,
           "NOT NULL");

    ASSERT(statement->function_def.block->nary.children->token.type == TokenType::ADDITION,
           debug_token_type_to_string(statement->function_def.block->nary.children->token.type));

    ASSERT(statement->function_def.block->nary.children->adjacent_child->token.type == TokenType::ADDITION,
           debug_token_type_to_string(statement->function_def.block->nary.children->adjacent_child->token.type));

    ASSERT(statement->function_def.return_type->token.value == "int",
           statement->function_def.return_type->token.value);

    statement = statement->adjacent_child;
    ASSERT(statement->type == AstNodeType::FUNCTION_DEF,
           "NOT FUNCTIONDEF");
    ASSERT(statement->token.type == TokenType::DEF,
           debug_token_type_to_string(statement->token.type));

    AstNode *param = statement->function_def.arguments;

    ASSERT(param->token.type == TokenType::IDENTIFIER,
           debug_token_type_to_string(param->token.type));
    ASSERT(param->declaration.annotation->token.type == TokenType::IDENTIFIER,
           debug_token_type_to_string(param->declaration.annotation->token.type));
    ASSERT(param->declaration.annotation->token.value == "int",
           param->declaration.annotation->token.value);
    param = param->adjacent_child;

    ASSERT(param->token.type == TokenType::IDENTIFIER,
           debug_token_type_to_string(param->token.type));
    ASSERT(param->declaration.annotation->token.type == TokenType::IDENTIFIER,
           debug_token_type_to_string(param->declaration.annotation->token.type));
    ASSERT(param->declaration.annotation->token.value == "string",
           param->declaration.annotation->token.value);

    ASSERT(statement->function_def.block->nary.children->token.type == TokenType::ADDITION,
           debug_token_type_to_string(statement->function_def.block->nary.children->token.type));

    ASSERT(statement->function_def.return_type->token.value == "string",
           statement->function_def.return_type->token.value);

    ast_arena.destroy();
    symbol_table_arena.destroy();

    END_TEST();
}


static Test precedence_test_helper(Test *test, AstNode *root) {
    if (!root) {
        return *test;
    }

    AstNode *child = root->nary.children;
    while (child) {
        int additional_info = child->token.precedence();
        ASSERT(child->token.precedence() >= root->token.precedence(), root->token.precedence());
        precedence_test_helper(test, child);
        child = child->adjacent_child;
    }

    return *test;

}

static Test precedence_test() {
    START_TEST();
    InputStream input_stream = InputStream::create_from_file("tests/precedence.py");
    if (!input_stream.contents) {exit(1);}
    Tokeniser tokeniser = Tokeniser::init(&input_stream);
    Arena ast_arena = Arena::init(GIGABYTES(32));

    Arena symbol_table_arena = Arena::init(GIGABYTES(1));

    CompilerTables tables = CompilerTables::init(&symbol_table_arena);

    SymbolTableValue main_symbol_value = {};
    main_symbol_value.static_type.type = TypeInfoType::INTEGER;

    std::string main_identifier = "main";
    SymbolTableEntry *main_scope = tables.function_table->insert(
        &symbol_table_arena, main_identifier, 0, main_symbol_value);

    AstNode *root = parse_statements(&tokeniser, &ast_arena, main_scope, &tables, &symbol_table_arena);
    AstNode *statement = root->nary.children;
    while (statement) {
        precedence_test_helper(test, statement);
        statement = statement->adjacent_child;
    }

    ast_arena.destroy();
    symbol_table_arena.destroy();
    END_TEST();
}

#if 0
static Test primary_test() {
    START_TEST();
    InputStream input_stream = InputStream::create_from_file("tests/primary.py");
    if (!input_stream.contents) {exit(1);}
    Tokeniser tokeniser = Tokeniser(&input_stream);
    AstNodeArena ast_arena = AstNodeArena(1000);
    AstNode *root = parse_statements(&tokeniser, &ast_arena);
}

#endif

#endif

int main() {
    INIT_MAIN()
    TEST(tokenise_file_test);
    TEST(tokenise_file_indentation_test);

    #if PARSER_TESTS
    TEST(floats_and_numbers_types_test);
    TEST(ifelse_test);
    TEST(functiondef_test);
    TEST(precedence_test)
    #endif

    printf("ALL TESTS PASSED\n");
    return 0;
}
