#ifndef TOKENISER_H_
#define TOKENISER_H_

#include <stdint.h>
#include <string>
#include "tpython.h"
#include "utils.h"


// Listed in reverse order of precedence for token_precendce()
// precedence may be used even in non math operations such as typed assignments
// operators clumped together have the same precendece
//
//

// TODO add from token
introspect enum class TokenType {
        // NOTE/TODO: could replace single char enum values with their ascii value making
        // error messages easier
        // would require a seperate precedence table since the current values are
        // precedence ordered for binary ops
        // and other tokens where it matters

        // BOOLEAN
        OR = 0,

        AND = 1,

        NOT = 2,

        // COMPARISON
        EQ = 3,
        NE = 4,
        LE = 5,
        LT = 6,
        GE = 7,
        GT = 8,
        IS = 9,
        IN_TOK = 10,
        NOT_IN = 11,

        // BITWISE
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

        DOT = 24, // Technically a binary op but not treated like the others

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
        MATCH = 46,
        CASE = 47,

        // ATOMS
        IDENTIFIER = 48,
        INT_LIT = 49,
        FLOAT_LIT = 50,
        STRING_LIT = 51,
        FSTRING = 52,
        NONE = 53,
        BOOL_TRUE = 54,
        BOOL_FALSE = 55,

        // MISC
        OPEN_PAREN = 56,
        CLOSED_PAREN = 57,
        SQUARE_OPEN_PAREN = 58,
        SQUARE_CLOSED_PAREN = 59,
        CURLY_OPEN_PAREN = 60,
        CURLY_CLOSED_PAREN = 61,
        COMMA = 62,
        ASSIGN = 63,
        COLON = 64,
        COLON_EQUAL = 65,
        ARROW = 66,
        AT = 67,
        IMPORT = 68,
        FROM = 69,
        NEWLINE = 70,
        INDENT = 71,
        DEDENT = 72,

        // FILE
        FILE = 73,
        ENDFILE = 74,
};

struct Token {
        enum TokenType type = TokenType::OR;
        uint32_t line = 0;
        uint32_t column = 0;
        std::string value = "";

        int precedence();
        bool is_binary_op();
        bool is_unary_op();
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
        uint32_t current_indent_level = 0;
        uint32_t lookahead_indent_level = 0;
        uint32_t paren_count = 0;

        static Tokeniser init(InputStream *stream);
        Token next_token();

        // get_next_token will return the next lookahead token not the next token for the parser
        Token get_next_lookahead();
};

#endif // TOKENISER_H_
