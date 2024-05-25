#include <cctype>
#include <cstdint>
#include <cstring>
#include <stdint.h>
#include <cstdint>
#include <vector>
#include <assert.h>
#include <iostream>

#include "tokeniser.h"

bool Token::is_binary_op() {
    switch (this->type) {
    case TokenType::MULTIPLICATION:
        return true;
        break;

    case TokenType::DIVISION:
        return true;
        break;

    case TokenType::ADDITION:
        return true;
        break;

    case TokenType::SUBTRACTION:
        return true;
        break;

    case TokenType::ASSIGN:
        return true;
        break;

    case TokenType::COLON:
        return true;
        break;

    case TokenType::FLOOR_DIV:
        return true;
        break;

    case TokenType::EXPONENTIATION:
        return true;
        break;

    case TokenType::REMAINDER:
        return true;
        break;

    case TokenType::EQ:
        return true;
        break;

    case TokenType::NE:
        return true;
        break;

    case TokenType::LE:
        return true;
        break;

    case TokenType::LT:
        return true;
        break;

    case TokenType::GE:
        return true;
        break;

    case TokenType::GT:
        return true;
        break;

    case TokenType::IS:
        return true;
        break;

    case TokenType::IN:
        return true;
        break;

    case TokenType::OR:
        return true;
        break;

    case TokenType::AND:
        return true;
        break;

    case TokenType::BWOR:
        return true;
        break;

    case TokenType::BWXOR:
        return true;
        break;

    case TokenType::BWAND:
        return true;
        break;

    case TokenType::SHIFTLEFT:
        return true;
        break;

    case TokenType::SHIFTRIGHT:
        return true;
        break;

    default:
        return false;
    }
}

bool Token::is_atom() {
    switch (this->type) {
    case TokenType::IDENTIFIER:
        return true;
        break;

    case TokenType::NONE:
        return true;
        break;

    case TokenType::TRUE:
        return true;
        break;

    case TokenType::INT_LIT:
        return true;
        break;

    case TokenType::FLOAT_LIT:
        return true;
        break;

    case TokenType::STRING_LIT:
        return true;
        break;

    default:
        return false;
    }
}


TokenType Token::precendence() {
    switch (this->type) {
    // map values of equivalent precence to each other
    case TokenType::SUBTRACTION:
        return TokenType::ADDITION;

    case TokenType::DIVISION:
        return TokenType::MULTIPLICATION;

    case TokenType::FLOOR_DIV:
        return TokenType::MULTIPLICATION;

    case TokenType::REMAINDER:
        return TokenType::MULTIPLICATION;

    default:
        return this->type;
    }
}

inline std::string Token::debug_to_string() {
    switch (this->type) {
    case TokenType::NEWLINE:
        return "NEWLINE";

    case TokenType::INDENT:
        return "INDENT";

    case TokenType::DEDENT:
        return "DEDENT";

    case TokenType::OPEN_PAREN:
        return "OPEN";

    case TokenType::CLOSED_PAREN:
        return "CLOSED";

    case TokenType::COMMA:
        return "COMMA";

    case TokenType::ASSIGN:
        return "ASSIGN";

    case TokenType::COLON:
        return "COLON";

    case TokenType::OR:
        return "OR";

    case TokenType::AND:
        return "AND";

    case TokenType::NOT:
        return "NOT";

    case TokenType::EQ:
        return "EQ";

    case TokenType::NE:
        return "NE";

    case TokenType::LE:
        return "LE";

    case TokenType::LT:
        return "LT";

    case TokenType::GE:
        return "GE";

    case TokenType::GT:
        return "GT";

    case TokenType::IS:
        return "IS";

    case TokenType::IN:
        return "IN";

    case TokenType::BWOR:
        return "BWOR";

    case TokenType::BWXOR:
        return "BWXOR";

    case TokenType::BWAND:
        return "BWAND";

    case TokenType::SHIFTLEFT:
        return "SHIFTLEFT";

    case TokenType::SHIFTRIGHT:
        return "SHIFTRIGHT";

    case TokenType::ADDITION:
        return "ADDITION";

    case TokenType::SUBTRACTION:
        return "SUBTRACTION";

    case TokenType::MULTIPLICATION:
        return "MULTIPLICATION";

    case TokenType::DIVISION:
        return "DIVISION";

    case TokenType::FLOOR_DIV:
        return "FLOOR";

    case TokenType::REMAINDER:
        return "REMAINDER";

    case TokenType::EXPONENTIATION:
        return "EXPONENTIATION";

    case TokenType::IDENTIFIER:
        return "IDENTIFIER";

    case TokenType::INT_LIT:
        return "INT";

    case TokenType::FLOAT_LIT:
        return "FLOAT";

    case TokenType::STRING_LIT:
        return "STRING";

    case TokenType::NONE:
        return "NONE";

    case TokenType::TRUE:
        return "TRUE";

    case TokenType::FALSE:
        return "FALSE";

    case TokenType::RETURN:
        return "RETURN";

    case TokenType::RAISE:
        return "RAISE";

    case TokenType::GLOBAL:
        return "GLOBAL";

    case TokenType::NONLOCAL:
        return "NONLOCAL";

    case TokenType::FILE:
        return "FILE";

    case TokenType::ENDFILE:
        return "ENDFILE";

    case TokenType::ERROR:
        return "ERROR";

    }

    return "UNKNOWN";
}

InputStream::InputStream() {
}

InputStream InputStream::create_from_file(const char *filename) {
    // initilise input stream
    InputStream input_stream = {};
    FILE *target_f = fopen(filename, "r");
    if (!target_f) {
        perror("Couldn't open file");
        return input_stream;
    }

    // compute filesize
    fseek(target_f, 0, SEEK_END);
    input_stream.size = ftell(target_f);
    fseek(target_f, 0, SEEK_SET);

    input_stream.contents = new char[input_stream.size];

    if (input_stream.contents == nullptr) {
        perror("error allocating buffer");
        return input_stream;
    }

    // read contents into file
    fread(input_stream.contents, input_stream.size, 1, target_f);
    fclose(target_f);
    return input_stream;
}

char InputStream::peek(uint32_t ahead) {
    if ((this->position + ahead) >= this->size) {
        return 0;
    }

    return this->contents[this->position + ahead];
}

char InputStream::next_char() {
    if ((this->position+1) >= this->size) {
        return 0;
    }

    ++this->col;
    if (this->contents[this->position] == '\n') {
        this->col = 0;
        ++this->line;
    }

    return this->contents[++this->position];
}

InputStream::InputStream(const InputStream &source) {
    this->contents = new char[source.size];
    memcpy(this->contents, source.contents, source.size);
    this->size = source.size;
    this->col = source.col;
    this->line = source.line;
}

Token *TokenStream::peek(uint32_t ahead) {
    if ((this->position + ahead) >= this->tokens.size()) {
        return nullptr;
    }

    return &this->tokens[this->position+ahead];
}

Token *TokenStream::next_token() {
    if ((this->position+1) >= this->tokens.size()) {
        return nullptr;
    }

    return &this->tokens[++this->position];
}

void tokenise_file(InputStream *input, std::vector<Token> *tokens) {
    // push an initial file token so that root of the AST has a token
    // may be able to keep it null but i didn't and i trust myself in the past
    //
    Token file_tok = {.type = TokenType::FILE};
    tokens->push_back(file_tok);

    std::string buffer = "";
    uint32_t indent_level = 0;

    for (char current = input->peek(0); current; current = input->next_char()) {
        // IDENTIFIERS
        if (std::isalpha(current)) {
            buffer.push_back(current);
            for (char alpha_char = input->peek(1); alpha_char;
                 alpha_char = input->peek(1)) {

                if (!std::isalpha(alpha_char)) {
                    break;
                }

              buffer.push_back(alpha_char);
              input->next_char();
            }

            // KEWORDS
            if (buffer == "is") {
                Token is = {.type = TokenType::IS, .line = input->line, .column = input->col };
                tokens->push_back(is);
                buffer.clear();
            }

            else if (buffer == "in") {
                Token in = {.type = TokenType::IN, .line = input->line, .column = input->col };
                tokens->push_back(in);
                buffer.clear();
            }

            else if (buffer == "or") {
                Token bool_not = {.type = TokenType::OR, .line = input->line, .column = input->col };
                tokens->push_back(bool_not);
                buffer.clear();
            }

            else if (buffer == "and") {
                Token bool_not = {.type = TokenType::AND, .line = input->line, .column = input->col };
                tokens->push_back(bool_not);
                buffer.clear();
            }

            else if (buffer == "not") {
                Token bool_not = {.type = TokenType::NOT, .line = input->line, .column = input->col };
                tokens->push_back(bool_not);
                buffer.clear();
            }

            else if (buffer == "True") {
                Token bool_true = {.type = TokenType::TRUE, .line = input->line, .column = input->col };

                tokens->push_back(bool_true);
                buffer.clear();
            }

            else if (buffer == "False") {
                Token bool_false = {.type = TokenType::FALSE, .line = input->line, .column = input->col };

                tokens->push_back(bool_false);
                buffer.clear();
            }

            else if (buffer == "None") {
                Token bool_none = {.type = TokenType::NONE, .line = input->line, .column = input->col };

                tokens->push_back(bool_none);
                buffer.clear();
            }

            else if (buffer == "return") {
                Token return_kw = {.type = TokenType::RETURN, .line = input->line, .column = input->col };

                tokens->push_back(return_kw);
                buffer.clear();
            }

            else if (buffer == "raise") {
                Token raise = {.type = TokenType::RAISE, .line = input->line, .column = input->col };

                tokens->push_back(raise);
                buffer.clear();
            }

            else if (buffer == "global") {
                Token global = {.type = TokenType::GLOBAL, .line = input->line, .column = input->col };

                tokens->push_back(global);
                buffer.clear();
            }

            else if (buffer == "nonlocal") {
                Token nonlocal = {.type = TokenType::NONLOCAL, .line = input->line, .column = input->col };

                tokens->push_back(nonlocal);
                buffer.clear();
            }

            else {
                Token identifier = {.type = TokenType::IDENTIFIER, .line = input->line, .column = input->col, .value = buffer};
                tokens->push_back(identifier);
                buffer.clear();
            }
        }

        // NUMBERS
        else if (std::isdigit(current)) {
            buffer.push_back(current);
            bool decimal = false;
            for (char digit_char = input->peek(1); digit_char;
                 digit_char = input->peek(1)) {

                if (digit_char == '.') {
                    decimal = true;
                }

                else if (!std::isdigit(digit_char)) {
                    break;
                }

                buffer.push_back(digit_char);
                input->next_char();
            }

            Token number = {.type = TokenType::INT_LIT, .line = input->line, .column = input->col, .value = buffer};

            if (decimal) {
                number.type = TokenType::FLOAT_LIT;
            }

            tokens->push_back(number);
            buffer.clear();
        }

        switch (current) {
            case '\n': {
                Token newline = {.type = TokenType::NEWLINE,
                                    .line = input->line,
                                    .column = input->col};

                tokens->push_back(newline);

                while (input->peek(1) == '\n') {
                    input->next_char();

                    Token newline = {.type = TokenType::NEWLINE,
                                     .line = input->line,
                                     .column = input->col};

                    tokens->push_back(newline);
                    break;
                }

                if (indent_level >= 1) {
                    for (int i = indent_level; i > -1; --i) {
                        for (int j = 0; j < 4; ++j) {
                            int lookahead = i*4+j + 1;
                            if (!input->peek(lookahead) || !(input->peek(lookahead) == ' ')) {
                                Token dedent = {.type = TokenType::DEDENT,
                                                .line = input->line,
                                                .column = input->col};

                                tokens->push_back(dedent);
                                break;
                            }
                        }
                    }
                }
            } break;

            case '(': {
                Token open_paren = {};
                open_paren.type = TokenType::OPEN_PAREN;
                open_paren.value = ")";
                tokens->push_back(open_paren);

            } break;

            case ')': {
                Token closed_paren = {};
                closed_paren.type = TokenType::CLOSED_PAREN;
                closed_paren.value = ")";
                tokens->push_back(closed_paren);

            } break;

            case '"': {
                for (char next = input->next_char(); next; next = input->next_char()) {
                    if (next == '"') break;
                    buffer.push_back(next);
                }

                Token string = {.type = TokenType::STRING_LIT, .line = input->line, .column = input->col, .value = buffer};
                tokens->push_back(string);
                buffer.clear();
            } break;

            case ',': {
                Token comma = {.type = TokenType::COMMA, .line = input->line, .column = input->col };
                tokens->push_back(comma);

            } break;

            case ':': {
                Token colon = {.type = TokenType::COLON, .line = input->line, .column = input->col };
                tokens->push_back(colon);
            } break;

            case '!': {
                char next = input->peek(1);
                if (next == '=') {
                    Token noteq = {.type = TokenType::NE, .line = input->line, .column = input->col };
                    input->next_char();
                    tokens->push_back(noteq);
                }

                else if (next == ' ') {
                    Token notatoken = {.type = TokenType::ERROR, .line = input->line, .column = input->col };
                    tokens->push_back(notatoken);
                }

            } break;

            case '=': {
                if (input->peek(1) == '=') {
                    Token equal = {.type = TokenType::EQ, .line = input->line, .column = input->col };
                    input->next_char();
                    tokens->push_back(equal);
                    break;
                }

                Token assign = {.type = TokenType::ASSIGN, .line = input->line, .column = input->col };
                tokens->push_back(assign);

            } break;

            //ARITHMETIC & LOGIC
            case '*': {
                if (input->peek(1) == '*') {
                    Token exponentiation = {.type = TokenType::EXPONENTIATION, .line = input->line, .column = input->col };
                    tokens->push_back(exponentiation);
                    input->next_char();
                    break;
                }

                Token multiplication = {.type = TokenType::MULTIPLICATION, .line = input->line, .column = input->col };
                tokens->push_back(multiplication);
            } break;

            case '/': {
                if (input->peek(1) == '/') {
                    Token floor_div = {.type = TokenType::FLOOR_DIV, .line = input->line, .column = input->col };
                    tokens->push_back(floor_div);
                    input->next_char();
                    break;
                }

                Token division = {.type = TokenType::DIVISION, .line = input->line, .column = input->col };
                tokens->push_back(division);
            } break;

            case '%': {
                Token remainder = {.type = TokenType::REMAINDER, .line = input->line, .column = input->col };
                tokens->push_back(remainder);
            } break;

            case '+': {
                Token addition = {.type = TokenType::ADDITION, .line = input->line, .column = input->col };
                tokens->push_back(addition);
            } break;

            case '-': {
                Token subtraction = {.type = TokenType::SUBTRACTION, .line = input->line, .column = input->col };
                tokens->push_back(subtraction);
            } break;

            case '<': {
                if (input->peek(1) == '<') {
                    Token lessthan = {.type = TokenType::SHIFTLEFT, .line = input->line, .column = input->col };
                    tokens->push_back(lessthan);
                    input->next_char();
                    break;
                }

                if (input->peek(1) == '=') {
                    Token lteaqualto = {.type = TokenType::LE, .line = input->line, .column = input->col };
                    tokens->push_back(lteaqualto);
                    input->next_char();
                    break;
                }

                Token lessthan = {.type = TokenType::LT, .line = input->line, .column = input->col };
                tokens->push_back(lessthan);

            } break;

            case '>': {
                if (input->peek(1) == '>') {
                    Token greaterthan = {.type = TokenType::SHIFTRIGHT, .line = input->line, .column = input->col };
                    tokens->push_back(greaterthan);
                    input->next_char();
                    break;
                }

                else if (input->peek(1) == '=') {
                    Token gteaqualto = {.type = TokenType::GE, .line = input->line, .column = input->col };
                    tokens->push_back(gteaqualto);
                    input->next_char();
                    break;
                }

                Token greaterthan = {.type = TokenType::GT, .line = input->line, .column = input->col };
                tokens->push_back(greaterthan);

            } break;

            case '&': {
                Token bitwiseand = {.type = TokenType::BWAND, .line = input->line, .column = input->col };
                tokens->push_back(bitwiseand);
            } break;

            case '^': {
                Token bitwisexor = {.type = TokenType::BWXOR, .line = input->line, .column = input->col };
                tokens->push_back(bitwisexor);
            } break;

            case '|': {
                Token bitwiseor = {.type = TokenType::BWOR, .line = input->line, .column = input->col };
                tokens->push_back(bitwiseor);
            } break;

            case ' ': {
                int spaces = 1;
                for (char space_char = input->peek(1); space_char;
                    space_char = input->peek(1)) {

                    if (!(space_char == ' ')) {
                        break;
                    }
                    ++spaces;

                    if (spaces == 4) {
                        ++indent_level;
                        spaces = 0;

                        Token indent = {.type = TokenType::INDENT,
                                        .line = input->line,
                                        .column = input->col};

                        tokens->push_back(indent);
                    }

                    input->next_char();
                }

            } break;
        }
    }

    // Append DEDENT token when we reach end of file
    for (int i = 0; i < indent_level; ++i) {
        Token dedent = {.type = TokenType::DEDENT,
                        .line = input->line,
                        .column = input->col};
        tokens->push_back(dedent);
    }

    Token endfile = {.type = TokenType::ENDFILE, .line = input->line, .column = input->col};
    tokens->push_back(endfile);

}
