#include <iostream>
#include <iterator>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "tokeniser.cpp"
#include "parser.cpp"
#include "tokeniser.h"

#define INIT_MAIN() Test TEST = {};
#define START_TEST() Test test = {};
#define END_TEST() test.passed = true; return test;
#define TEST(test) TEST = test(); if (!(TEST.passed)) {std::cout << "\nfailed case: " << TEST.cases << " on line: " << TEST.line << " in " << #test "\n"; return 1;}
#define FAILCASE(condition, info) test.line = __LINE__; if (!(condition)) {std::cerr << "Expected condition:" << #condition << "\n" << "Actual value: " << (info) << "\n"; test.passed = false; return test;} else {++test.cases;}

struct Test {
    int cases = 1;
    bool passed = false;
    int line;
};

InputStream create_stream_from_mock_file(std::string mock_file) {
    InputStream stream = {};
    stream.contents = new char[mock_file.length()];
    if (stream.contents == nullptr) {
       std::cerr << "SHITFAILED";
    }

    for (int i = 0; i < mock_file.length(); ++i) {
        stream.contents[i] = mock_file[i];
    }

    stream.size = mock_file.length();
    return stream;
}

static Test tokenise_file_test() {
    START_TEST()
    std::string mock_file = "(),=: or and not == != <= <>= > is in |^&<<>>+-* / // % ** identifier 123 123.321 \"string\" None True False return raise global nonlocal";
    InputStream stream = create_stream_from_mock_file(mock_file);
    std::vector<Token> tokens = {};
    tokenise_file(&stream, &tokens);

    for (int i = 1; i < tokens.size()-1; ++i) {
        int current = (i-1)+(int)TokenType::OPEN_PAREN;
        // check last token is nonlocal
        if (i+1 >= tokens.size() && tokens[i].type != TokenType::NONLOCAL) {
            std::cerr << "Unexpected end of token stream\n";
            std::cerr << "Last token was: " << tokens[i-1].debug_to_string()
                      << "\nline: " << tokens[i-1].line
                      << " col: " << tokens[i-1].column
                      << "\nexpected next token as: "
                      << ((Token *)(&current))->debug_to_string() << "\n";

            return test;
        }

        if ((int)tokens[i].type != current) {
            std::cerr << "token: " << tokens[i].debug_to_string()
                        << "\nline: " << tokens[i].line
                        << " col: " << tokens[i].column
                        << "\ndidn't match with expected for "
                        << ((Token *)(&current))->debug_to_string()
                        << " in file \n";

          return test;
        }

        if (tokens[i].type == TokenType::INT_LIT) {
            if (tokens[i].value != "123") {
                std::cerr << "INT token with value: " << tokens[i].value
                            << "failed to match with expected value of: 123\n";

              return test;
          }
        }

        if (tokens[i].type == TokenType::FLOAT_LIT) {
            if (tokens[i].value != "123.321") {
                std::cerr << "INT token with value: " << tokens[i].value
                        << " failed to match with expected value of: 123.321\n";
                return test;
          }
        }

        if (tokens[i].type == TokenType::STRING_LIT) {
            if (tokens[i].value != "string") {
                std::cerr << "String token with value: " << tokens[i].value
                          << " failed to match with expected value of: string\n";
            return test;
          }
        }
    }

    ++test.cases;

    END_TEST()
}

static Test tokenise_file_indentation_test() {
    START_TEST()
    const char *mock_file = "\n    \n        ";
    InputStream stream = create_stream_from_mock_file(mock_file);
    std::vector<Token> tokens = {};
    tokenise_file(&stream, &tokens);

    FAILCASE(tokens[2].type == TokenType::INDENT, tokens[2].debug_to_string());
    FAILCASE(tokens[4].type == TokenType::INDENT, tokens[4].debug_to_string());
    FAILCASE(tokens[5].type == TokenType::INDENT, tokens[5].debug_to_string());
    FAILCASE(tokens[6].type == TokenType::DEDENT, tokens[6].debug_to_string());
    FAILCASE(tokens[7].type == TokenType::DEDENT, tokens[7].debug_to_string());

    mock_file = "\n        \n    3+3\n";
    stream = create_stream_from_mock_file(mock_file);
    tokens.clear();
    tokenise_file(&stream, &tokens);

    FAILCASE(tokens[2].type == TokenType::INDENT, tokens[3].debug_to_string());
    FAILCASE(tokens[3].type == TokenType::INDENT, tokens[4].debug_to_string());
    FAILCASE(tokens[5].type == TokenType::DEDENT, tokens[6].debug_to_string());

    END_TEST()
}

static Test floats_and_numbers_types_test() {
    START_TEST();
    const char *mock_file = "3 + 3\n 3 - 3.0\n3.0 * 3\n3.0 - 3.0\n3 / 3\n 3 // 3";
    InputStream stream = create_stream_from_mock_file(mock_file);
    std::vector<Token> tokens = {};

    tokenise_file(&stream, &tokens);
    TokenStream token_stream  = TokenStream(tokens);
    ASTNodeArena ast_arena = ASTNodeArena(token_stream.tokens.size()+1);
    ASTNode *root = parse_statements(&token_stream, &ast_arena, false);
    type_parse_tree(root);
    debug_print_parse_tree(root, "        ");

    ASTNode *child = root->children;
    FAILCASE(child->static_type == StaticType::INTEGER, debug_static_type_to_string(child->static_type));
    child = child->next;
    FAILCASE(child->static_type == StaticType::DECIMAL, debug_static_type_to_string(child->static_type));
    child = child->next;
    FAILCASE(child->static_type == StaticType::DECIMAL, debug_static_type_to_string(child->static_type));
    child = child->next;
    FAILCASE(child->static_type == StaticType::DECIMAL, debug_static_type_to_string(child->static_type));
    child = child->next;
    FAILCASE(child->static_type == StaticType::DECIMAL, debug_static_type_to_string(child->static_type));
    child = child->next;
    FAILCASE(child->static_type == StaticType::INTEGER, debug_static_type_to_string(child->static_type));

    END_TEST()
}

#if 0
static bool parse_expression_test() {
    InputStream input_stream = InputStream::create_from_file("tests/test_precedence.typ");
    if (input_stream.contents == nullptr) {
        return false;
    }

    std::vector<Token> tokens = {};
    tokenise_file(&input_stream, &tokens);
    TokenStream token_stream = TokenStream(tokens);

    ASTNodeArena ast_arena = ASTNodeArena(token_stream.tokens.size()+1);
    ASTNode file_node = {.token = token_stream.peek(0), .type = ASTNodeType::NARY};

    while (token_stream.peek(0)) {
        ASTNode *child = parse_statement(&token_stream, &ast_arena, false);
        if (!child) {
            break;
        }

        file_node.nary.n_children++;
        file_node.nary.children = child;

        token_stream.next_token();
    }

    ast_arena.node_alloc(&file_node);

    return true;
}
#endif

int main() {
    INIT_MAIN()
    TEST(tokenise_file_test);
    TEST(tokenise_file_indentation_test);
    TEST(floats_and_numbers_types_test);

    std::cout << "ALL TESTS PASSED\n";
    return 0;
}
