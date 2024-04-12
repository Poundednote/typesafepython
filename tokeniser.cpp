#include <stdint.h>
#include <vector>
#include <assert.h>

#include "tokeniser.h"

bool Token::is_binary_op() {
    switch (this->type) {
        case TokenType::MULTIPLICATION: {
            return true;
        } break;

        case TokenType::DIVISION: {
            return true;
        } break;

        case TokenType::ADDITION: {
            return true;
        } break;

        case TokenType::SUBTRACTION: {
            return true;
        } break;

        case TokenType::ASSIGN: {
            return true;
        } break;

        case TokenType::COLON: {
            return true;
        } break;

        case TokenType::FLOOR_DIV: {
            return true;
        } break;

        case TokenType::EXPONENTIATION: {
            return true;
        } break;

        case TokenType::REMAINDER: {
            return true;
        } break;

        case TokenType::EQ: {
            return true;
        } break;

        case TokenType::NE: {
            return true;
        } break;

        case TokenType::LE: {
            return true;
        } break;

        case TokenType::LT: {
            return true;
        } break;
        case TokenType::GE: {
            return true;
        }
        case TokenType::GT: {
            return true;
        } break;
        case TokenType::ISNOT: {
            return true;
        }
        case TokenType::IS: {
            return true;
        } break;
        case TokenType::NOTIN: {
            return true;
        }
        case TokenType::IN: {
            return true;
        } break;
        case TokenType::BWOR: {
            return true;
        } break;
        case TokenType::BWXOR: {
            return true;
        } break;
        case TokenType::BWAND: {
            return true;
        } break;
        case TokenType::SHIFTLEFT: {
            return true;
        } break;

        case TokenType::SHIFTRIGHT: {
            return true;
        } break;

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

    return this->contents[++this->position];
}

Token *TokenStream::peek(uint32_t ahead) {
    if ((this->position + ahead) >= this->n_tokens) {
        return nullptr;
    }

    return &this->tokens[this->position+ahead];
}

Token *TokenStream::next_token() {
    if ((this->position+1) >= this->n_tokens) {
        return nullptr;
    }

    return &this->tokens[++this->position];
}

void process_word_into_buf(InputStream *input, std::string &buffer) {
}

void tokenise_file(InputStream *input, std::vector<Token> *tokens) {
    std::string buffer = {};
    // IDENTIFIERS & KEYWORDS
    for (char current = input->peek(0); current; current = input->next_char()) {
        if (std::isalpha(current)) {
            buffer.push_back(current);
            for (char alpha_char = input->peek(1); alpha_char;
                 alpha_char = input->peek(1)) {

              if (!std::isalpha(alpha_char))
                break;
              buffer.push_back(alpha_char);
              input->next_char();
            }
            // KEWORDS
            if (buffer == "is" && input->peek(' ')) {
                Token is = {.type = TokenType::IS};
                tokens->push_back(is);
                buffer.clear();
            }


            else if (buffer == "in" && input->peek(' ')) {
                Token in = {.type = TokenType::IN};
                tokens->push_back(in);
                buffer.clear();
            }

            else if (buffer == "not" && input->peek(' ')) {
                Token bool_not = {.type = TokenType::NOT};
                tokens->push_back(bool_not);
                buffer.clear();
            }

            else if (buffer == "True" && input->peek(' ')) {
                Token bool_true = {.type = TokenType::TRUE,
                                    .value = buffer};

                tokens->push_back(bool_true);
                buffer.clear();
            }

            else if (buffer == "False" && input->peek(' ')) {
                Token bool_false = {.type = TokenType::FALSE,
                                    .value = buffer};

                tokens->push_back(bool_false);
                buffer.clear();
            }

            else if (buffer == "None" && input->peek(' ')) {
                Token bool_none = {.type = TokenType::NONE,
                                    .value = buffer};

                tokens->push_back(bool_none);
                buffer.clear();
            }

            else {
                Token identifier = {.type = TokenType::IDENTIFIER, .value = buffer};
                tokens->push_back(identifier);
                buffer.clear();
            }
        }

        // NUMBERS
        else if (std::isdigit(current)) {
            buffer.push_back(current);
            for (char digit_char = input->peek(1); digit_char;
                 digit_char = input->peek(1)) {
                if (!std::isdigit(digit_char)) break;
                buffer.push_back(digit_char);
                input->next_char();
            }

            Token int_lit = {.type = TokenType::INT_LIT, .value = buffer};
            tokens->push_back(int_lit);
            buffer.clear();
        }

        switch (current) {

            case '\n': {
                Token newline = {.type = TokenType::NEWLINE};
                tokens->push_back(newline);
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
                Token open_quote = {.type = TokenType::OPEN_QUOTE};
                tokens->push_back(open_quote);

                for (char next = input->peek(1); (next = input->peek(1));) {
                    if (next == '"') break;
                    buffer.push_back(input->next_char());
                }

                Token string = {.type = TokenType::STRING_LIT, .value = buffer};
                tokens->push_back(string);
                buffer.clear();

                Token close_quote = {.type = TokenType::CLOSE_QUOTE};
                tokens->push_back(close_quote);
                input->next_char();
            } break;

            case ',': {
                Token comma = {.type = TokenType::COMMA};
                tokens->push_back(comma);

            } break;

            case ':': {
                Token colon = {.type = TokenType::COLON};
                tokens->push_back(colon);
            } break;

            case '=': {
                if (input->peek(1) == '=') {
                    Token equal = {.type = TokenType::EQ};
                    input->next_char();
                    tokens->push_back(equal);
                    break;
                }

                Token assign = {.type = TokenType::ASSIGN};
                tokens->push_back(assign);

            } break;

            //ARITHMETIC & LOGIC
            case '*': {
                if (input->peek(1) == '*') {
                    Token exponentiation = {.type = TokenType::EXPONENTIATION};
                    tokens->push_back(exponentiation);
                    input->next_char();
                    break;
                }

                Token multiplication = {.type = TokenType::MULTIPLICATION};
                tokens->push_back(multiplication);
            } break;

            case '/': {
                if (input->peek(1) == '/') {
                    Token floor_div = {.type = TokenType::FLOOR_DIV};
                    tokens->push_back(floor_div);
                    input->next_char();
                    break;
                }

                Token division = {.type = TokenType::DIVISION};
                tokens->push_back(division);
            } break;

            case '+': {
                Token addition = {.type = TokenType::ADDITION};
                tokens->push_back(addition);
            } break;

            case '-': {
                Token subtraction = {.type = TokenType::SUBTRACTION};
                tokens->push_back(subtraction);
            } break;

            case '<': {
                if (input->peek(1) == '<') {
                    Token lessthan = {.type = TokenType::SHIFTLEFT};
                    tokens->push_back(lessthan);
                    input->next_char();
                    break;
                }

                if (input->peek(1) == '=') {
                    Token lteaqualto = {.type = TokenType::LE};
                    tokens->push_back(lteaqualto);
                    input->next_char();
                    break;
                }

                Token lessthan = {.type = TokenType::LT};
                tokens->push_back(lessthan);

            } break;

            case '>': {
                if (input->peek(1) == '>') {
                    Token greaterthan = {.type = TokenType::SHIFTRIGHT};
                    tokens->push_back(greaterthan);
                    input->next_char();
                    break;
                }

                if (input->peek(1) == '=') {
                    Token gteaqualto = {.type = TokenType::GE};
                    tokens->push_back(gteaqualto);
                    input->next_char();
                    break;
                }

                Token greaterthan = {.type = TokenType::GT};
                tokens->push_back(greaterthan);

            } break;

            case '&': {
                Token bitwiseand = {.type = TokenType::BWAND};
                tokens->push_back(bitwiseand);
            } break;

            case '^': {
                Token bitwisexor = {.type = TokenType::BWXOR};
                tokens->push_back(bitwisexor);
            } break;

            case '|': {
                Token bitwiseor = {.type = TokenType::BWOR};
                tokens->push_back(bitwiseor);
            } break;

            case ' ': {
                continue;
            } break;
        }
    }
}
