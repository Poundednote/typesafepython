#ifndef TOKENISER_H_
#define TOKENISER_H_

#include <stdint.h>
#include <string>

// Listed in reverse order of precedence for token_precendce()
// precedence may be used even in non math operations such as typed assignments
// operators clumped together have the same precendece
//
enum class TokenType {

    // BOOLEAN
    OR = 1,

    AND = 2,

    NOT = 3,

    // COMPARISON
    EQ = 4,
    NE = 5,
    LE = 6,
    LT = 7,
    GE = 8,
    GT = 9,
    IS = 10,
    IN_TOK = 11,

    //BITWISE
    BWOR = 12,

    BWXOR = 13,

    BWAND = 14,

    SHIFTLEFT = 15,
    SHIFTRIGHT = 16,

    // MATH
    ADDITION = 17,
    SUBTRACTION = 18,

    MULTIPLICATION = 19,
    DIVISION = 20,
    FLOOR_DIV = 21,
    REMAINDER = 22,

    EXPONENTIATION = 23,

    DOT = 24, // Technically a binary op

    // KEYWORDS
    RETURN = 25,
    YIELD = 26,
    RAISE = 27,
    GLOBAL = 28,
    NONLOCAL = 29,
    IF = 30,
    ELIF = 31,
    ELSE = 32,
    DEF = 33,
    CLASS = 34,
    WHILE = 35,
    FOR = 36,
    TRY = 37,
    EXCEPT = 38,
    FINALLY = 39,
    WITH = 40,
    AS = 41,
    PASS = 42,
    BREAK = 43,
    CONTINUE = 44,
    DEL = 45,

    // ATOMS
    IDENTIFIER = 46,
    INT_LIT = 47,
    FLOAT_LIT = 48,
    STRING_LIT = 49,
    NONE = 50,
    BOOL_TRUE = 51,
    BOOL_FALSE = 52,

    // MISC
    OPEN_PAREN = 53,
    CLOSED_PAREN = 54,
    SQUARE_OPEN_PAREN = 55,
    SQUARE_CLOSED_PAREN = 56,
    CURLY_OPEN_PAREN = 57,
    CURLY_CLOSED_PAREN = 58,
    COMMA = 59,
    ASSIGN = 60,
    COLON = 61,
    ARROW = 62,
    AT = 63,
    IMPORT = 64,
    NEWLINE = 65,
    INDENT = 66,
    DEDENT = 67,

    // FILE
    FILE = 68,
    ENDFILE = 69,
};

struct Token {
    enum TokenType type = TokenType::OR;
    uint32_t line = 0;
    uint32_t column = 0;
    std::string value = "";

    int precedence();
    bool is_binary_op();
    bool is_literal();
    bool is_comparrison_op();
    bool is_num();
};


struct InputStream {
    int32_t position = 0;
    char *contents;
    uint64_t size = 0;
    uint32_t col = 0;
    uint32_t line = 1;

    static InputStream create_from_file(const char *filename);
    void destroy();

    inline char peek(uint32_t ahead);
    inline char next_char();
};

struct Tokeniser {
    InputStream *stream;
    Token last_returned;
    Token lookahead;
    uint32_t indent_level = 0;
    uint32_t dedents_to_process = 0;

    static Tokeniser init(InputStream *stream);
    Token next_token();
    //
    // get_next_token will return the next lookahead token not the next token for the parser
    Token get_next_lookahead();
};

std::string debug_token_type_to_string(enum TokenType type);
#endif // TOKENISER_H_
