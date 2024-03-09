#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

enum class TokenType {
    identifier,
    int_lit,
    string_lit,
    newline,
    open_paren,
    closed_paren,
    assign,
    type_annot,
    colon,
    comma,
    open_quote,
    close_quote,
    root,

    // Math
    addition,
    subtraction,
    multiplication,
    division,
    floor_div,
    exponentiation,
    remainder,
};

struct Token {
    TokenType type;
    std::string value;
};

enum class StaticTypes {
};

struct TypedIdentifier {
    StaticTypes type;
    char value[76];
};

struct ASTNode {
    Token token;
    ASTNode *left;
    ASTNode *right;
};

void tokenise_expr(const std::string input, std::vector<Token> &tokens) {
    std::string buffer = {};
    for (int i = 0; i < input.length(); ++i) {

        if (std::isalpha(input[i])) {
            while (std::isalpha(input[i])) {
                buffer.push_back(input[i]);
                ++i;
            }

            Token identifier = {.type = TokenType::identifier, .value = buffer};
            tokens.push_back(identifier);
            buffer.clear();
            --i;
        }

        else if (std::isdigit(input[i])) {
            while (std::isdigit(input[i])) {
                buffer.push_back(input[i]);
                ++i;
            }

            Token int_lit = {.type = TokenType::int_lit, .value = buffer};
            tokens.push_back(int_lit);
            buffer.clear();
            --i;
        }

        switch (input[i]) {
            case '(': {
                Token open_paren = {.type = TokenType::open_paren, .value = "("};
                tokens.push_back(open_paren);

                int open_paren_index = i;
                while (input[i] != ')') {
                    ++i;
                }

                tokenise_expr(input.substr(open_paren_index+1, i-1-open_paren_index), tokens);
                Token closed_parenthesis = {.type = TokenType::closed_paren, .value = ")"};
                tokens.push_back(closed_parenthesis);
            } break;

            case '"': {
                Token open_quote = {.type = TokenType::open_quote, .value = "\""};
                tokens.push_back(open_quote);
                ++i;
                while (input[i] != '"') {
                    buffer.push_back(input[i++]);
                }

                Token string = {.type = TokenType::string_lit, .value = buffer};
                tokens.push_back(string);
                buffer.clear();

                Token close_quote = {.type = TokenType::close_quote, .value = "\""};
                tokens.push_back(close_quote);
            } break;

            case ',': {
                Token comma = {.type = TokenType::comma, .value = ","};
                tokens.push_back(comma);

            } break;

            case ':': {
                Token colon = {.type = TokenType::colon, .value = ":"};
                tokens.push_back(colon);
                i += 2;
                while (input[i] != ' ') {
                    buffer.push_back(input[i++]);
                }
                Token type_annot = {.type = TokenType::type_annot, .value = buffer};
                tokens.push_back(type_annot);
            } break;

            case '=': {
                Token assign = {.type = TokenType::assign, .value = "="};
                tokens.push_back(assign);
            } break;

            case '\n': {
                Token newline = {.type = TokenType::newline, .value = "\n"};
                tokens.push_back(newline);
            } break;


            case '+': {
                Token addition = {.type = TokenType::addition, .value = "+"};
                tokens.push_back(addition);
            } break;

            case '-': {
                Token subtraction = {.type = TokenType::subtraction, .value = "-"};
                tokens.push_back(subtraction);
            } break;

            case '*': {
                if (input [i+1] == '*') {
                    Token exponentiation = {.type = TokenType::exponentiation, .value = "**"};
                    tokens.push_back(exponentiation);
                }

                Token multiplication = {.type = TokenType::multiplication, .value = "*"};
                tokens.push_back(multiplication);
            } break;

            case '/': {
                if (input[i+1] == '/') {
                    Token floor_div = {.type = TokenType::floor_div, .value = "//"};
                    tokens.push_back(floor_div);
                    break;
                }

                Token division = {.type = TokenType::division, .value = "/"};
                tokens.push_back(division);
            } break;

            case ' ': {

                while(input[++i] == ' ') {
                    ++i;
                }
                --i;
            } break;
        }
    }

    return;
}

ASTNode *build_parse_tree(const std::vector<Token> &tokens, ASTNode *root) {
    for (int i = 0; i < tokens.size(); ++i) {
        switch (tokens[i].type) {
            case TokenType::assign: {
                ASTNode *assignment = new ASTNode({.token = tokens[i], .left = {}, .right = {}});
                if (tokens[i - 2].type != TokenType::colon) {
                    std::cerr << "Expected type annotation before assignment";
                }

                ASTNode *identifier = new ASTNode({.token = tokens[i - 3], .left = {}, .right = {}});
                ASTNode *type_annot = new ASTNode({.token = tokens[i - 1], .left = {}, .right = {}});

                identifier->left = type_annot;
                assignment->left = identifier;
            }
            }
        }

    return root;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Please specify a file";
        return EXIT_FAILURE;
    }

    std::ifstream target_f;
    target_f.open(argv[1]);

    std::stringstream str_stream;
    str_stream << target_f.rdbuf();
    const std::string input = str_stream.str();

    std::vector<Token> tokens = {};
    tokenise_expr(input, tokens);

    std::ofstream output_f;
    output_f.open("out.py");

    std::vector<ASTNode *> identifiers = {};

    return 0;
}
