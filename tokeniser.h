#ifndef TOKENISER_H_
#define TOKENISER_H_

#include <cstdint>
#include <cstring>
#include <stdint.h>
#include <string>
#include <stdlib.h>
#include <vector>

// Listed in reverse order of precedence for token_precendce()
// precedence may be used even in non math operations such as typed assignments
// operators clumped together have the same precendece
//
enum class TokenType {
    // MISC
    NEWLINE = 0,
    INDENT = 1,
    DEDENT = 2,
    OPEN_PAREN = 3,
    CLOSED_PAREN = 4,
    COMMA = 5,
    ASSIGN = 6,
    COLON = 7,

    OR = 8,

    AND = 9,

    NOT = 10,

    EQ = 11,
    NE = 12,
    LE = 13,
    LT = 14,
    GE = 15,
    GT = 16,
    IS = 17,
    IN = 18,

    BWOR = 19,

    BWXOR = 20,

    BWAND = 21,

    SHIFTLEFT = 22,
    SHIFTRIGHT = 23,

    ADDITION = 24,
    SUBTRACTION = 25,

    MULTIPLICATION = 26,
    DIVISION = 27,
    FLOOR_DIV = 28,
    REMAINDER = 29,

    EXPONENTIATION = 30,

    // ATOMS
    IDENTIFIER = 31,
    INT_LIT = 32,
    FLOAT_LIT = 33,
    STRING_LIT = 34,
    NONE = 35,
    TRUE = 36,
    FALSE = 37,

    RETURN = 38,
    RAISE = 39,
    GLOBAL = 40,
    NONLOCAL = 41,

    //FILE
    FILE = 42,
    ENDFILE = 43,
    ERROR = 44
};

struct Token {
    TokenType type;
    uint32_t line = 0;
    uint32_t column = 0;
    std::string value = "";

    TokenType precendence();
    bool is_binary_op();
    bool is_atom();
    std::string debug_to_string();
};

struct TokenStream {
    uint32_t position = 0;
    std::vector<Token> &tokens;

    TokenStream(std::vector<Token> &t) : tokens(t) {
        this->position = 0;
    }

    inline Token *peek(uint32_t ahead);
    inline Token *next_token();
};

struct InputStream {
    uint32_t position = 0;
    char *contents = nullptr;
    uint64_t size = 0;
    uint32_t col = 0;
    uint32_t line = 0;

    InputStream();

    static InputStream create_from_file(const char *filename);
    InputStream(const InputStream &source);
    const InputStream& operator=(const InputStream& rval) {
        delete[] this->contents;

        this->position = rval.position;
        this->contents = new char[rval.size];
        memcpy(this->contents, rval.contents, rval.size);
        this->size = rval.size;
        this->col = rval.col;
        this->line = rval.line;

        return *this;
 }

    ~InputStream() {
        delete[] this->contents;
    }

    inline char peek(uint32_t ahead);
    inline char next_char();
};

void tokenise_file(InputStream *input, std::vector<Token> *tokens);
#endif // TOKENISER_H_
