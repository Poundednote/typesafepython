#ifndef TOKENISER_H_
#define TOKENISER_H_

#include <cstdint>
#include <stdint.h>
#include <string>
#include <stdlib.h>
#include <vector>

enum class TokenType {
    // Listed in reverse order of precedence for token_precendce()
    // precedence may be used even in non math operations such as typed assignments
    // operators clumped together have the same precendece
    //
    //
    //FILE
    FILE,
    ENDFILE,

    //ATOM

    // MISC
    NEWLINE,
    OPEN_PAREN,
    CLOSED_PAREN,
    TYPE_ANNOT,
    COMMA,
    OPEN_QUOTE,
    CLOSE_QUOTE,
    ROOT,


    ASSIGN,

    COLON,

    OR,

    AND,

    NOT,

    EQ,
    NE,
    LE,
    LT,
    GE,
    GT,
    ISNOT,
    IS,
    NOTIN,
    IN,

    BWOR,

    BWXOR,

    BWAND,

    SHIFTLEFT,
    SHIFTRIGHT,

    ADDITION,
    SUBTRACTION,

    MULTIPLICATION,
    DIVISION,
    FLOOR_DIV,
    REMAINDER,

    EXPONENTIATION,

    IDENTIFIER,
    INT_LIT,
    STRING_LIT,
    NONE,
    TRUE,
    FALSE,

    END // marks the end of a token stream
};

struct Token {
    TokenType type;

    std::string value;
    int32_t line = 0;
    int32_t column = 0;

    TokenType precendence();
    bool is_binary_op();
};


struct TokenStream {
    uint32_t n_tokens = 0;
    uint32_t position = 0;
    std::vector<Token> &tokens;

    TokenStream(uint32_t n_tokens, std::vector<Token> &tokens) : tokens(tokens) {
        this->n_tokens = n_tokens;
        this->position = 0;
    }

    Token *peek(uint32_t ahead);
    Token *next_token();
};


struct InputStream {
    uint32_t position = 0;
    char *contents;
    long int size;

    char peek(uint32_t ahead);
    char next_char();
};


void tokenise_file(InputStream *input, std::vector<Token> *tokens);
#endif // TOKENISER_H_
