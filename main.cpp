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
    comma,
    open_quote,
    close_quote,

};

struct Token {
    TokenType type;
    std::optional<std::string> value;
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

        else if (input[i] == '(') {
            {
                Token token = {.type = TokenType::open_paren, .value = std::nullopt};
                tokens.push_back(token);
            }

            int open_paren_index = i;
            while (input[i] != ')') {
                ++i;
            }

            tokenise_expr(input.substr(open_paren_index+1, i-1-open_paren_index), tokens);
            Token closed_parenthesis = {.type = TokenType::closed_paren, .value = std::nullopt};
            tokens.push_back(closed_parenthesis);
        }

        else if (input[i] == '"') {
            {
                Token open_quote = {.type = TokenType::open_quote, .value = std::nullopt};
                tokens.push_back(open_quote);
            }

            ++i;
            while (input[i] != '"') {
                buffer.push_back(input[i++]);
            }

            Token string = {.type = TokenType::string_lit, .value = buffer};
            tokens.push_back(string);
            buffer.clear();

            Token close_quote = {.type = TokenType::close_quote, .value = std::nullopt};
            tokens.push_back(close_quote);
        }

        else if (input[i] == ',') {
            Token comma = {.type = TokenType::comma, .value = std::nullopt};
            tokens.push_back(comma);
        }

        else if (input[i] == '\n') {
            Token newline = {.type = TokenType::newline, .value = std::nullopt};
            tokens.push_back(newline);
        }

        else if (std::isspace(input[i])) {
            if (!buffer.empty()) {
              Token identifier = {.type = TokenType::identifier, .value = buffer};
              buffer.clear();
            }
            continue;
        }
    }

    return;
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

    return 0;
}
