#ifndef TOKENISER_H_
#define TOKENISER_H_

#include <stdint.h>
#include <string>
#include "main.h"
#include "utils.h"

// Listed in reverse order of precedence for token_precendce()
// precedence may be used even in non math operations such as typed assignments
// operators clumped together have the same precendece
//
//
//
// would require a seperate precedence table since the current values are
// precedence ordered for binary ops
// and other tokens where it matters
// TODO add from token
#define TOKEN_TYPE(e, s)
#define ALL_TOKEN_TYPES \
        TOKEN_TYPE(OR = 0, "or") \
        TOKEN_TYPE(AND = 1, "and") \
        TOKEN_TYPE(NOT = 2, "not") \
        TOKEN_TYPE(EQ = 3, "==") \
        TOKEN_TYPE(NE = 4, "!=") \
        TOKEN_TYPE(LE = 5, "<=") \
        TOKEN_TYPE(LT = 6, "<") \
        TOKEN_TYPE(GE = 7, ">=") \
        TOKEN_TYPE(GT = 8, ">") \
        TOKEN_TYPE(IS = 9, "is") \
        TOKEN_TYPE(IN_TOK = 10, "in") \
        TOKEN_TYPE(NOT_IN = 11, "not in") \
        TOKEN_TYPE(BWOR = 12, "|") \
        TOKEN_TYPE(BWXOR = 13, "^") \
        TOKEN_TYPE(BWAND = 14, "&") \
        TOKEN_TYPE(SHIFTLEFT = 15, "<<") \
        TOKEN_TYPE(SHIFTRIGHT = 16, ">>") \
        TOKEN_TYPE(ADDITION = 17, "+") \
        TOKEN_TYPE(SUBTRACTION = 18, "-") \
        TOKEN_TYPE(MULTIPLICATION = 19, "*") \
        TOKEN_TYPE(DIVISION = 20, "/") \
        TOKEN_TYPE(FLOOR_DIV = 21, "//") \
        TOKEN_TYPE(REMAINDER = 22, "%") \
        TOKEN_TYPE(EXPONENTIATION = 23, "**") \
        TOKEN_TYPE(DOT = 24, ".") \
        TOKEN_TYPE(RETURN = 25, "return") \
        TOKEN_TYPE(YIELD = 26, "yield") \
        TOKEN_TYPE(RAISE = 27, "raise") \
        TOKEN_TYPE(GLOBAL = 28, "global") \
        TOKEN_TYPE(NONLOCAL = 29, "nonlocal") \
        TOKEN_TYPE(IF = 30, "if") \
        TOKEN_TYPE(ELIF = 31, "elif") \
        TOKEN_TYPE(ELSE = 32, "else") \
        TOKEN_TYPE(DEF = 33, "def") \
        TOKEN_TYPE(CLASS = 34, "class") \
        TOKEN_TYPE(WHILE = 35, "while") \
        TOKEN_TYPE(FOR = 36, "for") \
        TOKEN_TYPE(TRY = 37, "try") \
        TOKEN_TYPE(EXCEPT = 38, "except") \
        TOKEN_TYPE(FINALLY = 39, "finally") \
        TOKEN_TYPE(WITH = 40, "with") \
        TOKEN_TYPE(AS = 41, "as") \
        TOKEN_TYPE(PASS = 42, "pass") \
        TOKEN_TYPE(BREAK = 43, "break") \
        TOKEN_TYPE(CONTINUE = 44, "continue") \
        TOKEN_TYPE(DEL = 45, "del") \
        TOKEN_TYPE(MATCH = 46, "match") \
        TOKEN_TYPE(CASE = 47, "case") \
        TOKEN_TYPE(LAMBDA = 48, "lambda") \
        TOKEN_TYPE(IDENTIFIER = 49, "") \
        TOKEN_TYPE(INT_LIT = 50, "") \
        TOKEN_TYPE(FLOAT_LIT = 51, "") \
        TOKEN_TYPE(STRING_LIT = 52, "") \
        TOKEN_TYPE(FSTRING = 53, "") \
        TOKEN_TYPE(NONE = 54, "None") \
        TOKEN_TYPE(BOOL_TRUE = 55, "True") \
        TOKEN_TYPE(BOOL_FALSE = 56, "False") \
        TOKEN_TYPE(OPEN_PAREN = 57, "(") \
        TOKEN_TYPE(CLOSED_PAREN = 58, ")") \
        TOKEN_TYPE(SQUARE_OPEN_PAREN = 59, "[") \
        TOKEN_TYPE(SQUARE_CLOSED_PAREN = 60, "]") \
        TOKEN_TYPE(CURLY_OPEN_PAREN = 61, "{") \
        TOKEN_TYPE(CURLY_CLOSED_PAREN = 62, "}") \
        TOKEN_TYPE(COMMA = 63, ",") \
        TOKEN_TYPE(ASSIGN = 64, "=") \
        TOKEN_TYPE(COLON = 65, ":") \
        TOKEN_TYPE(COLON_EQUAL = 66, ":=") \
        TOKEN_TYPE(ARROW = 67, "->") \
        TOKEN_TYPE(AT = 68, "@") \
        TOKEN_TYPE(IMPORT = 69, "import") \
        TOKEN_TYPE(FROM = 70, "from") \
        TOKEN_TYPE(NEWLINE = 71, "") \
        TOKEN_TYPE(SEMICOLON = 72, ";") \
        TOKEN_TYPE(INDENT = 73, "") \
        TOKEN_TYPE(DEDENT = 74, "") \
        TOKEN_TYPE(FILE = 75, "") \
        TOKEN_TYPE(ENDFILE = 76, "EOF")
#undef TOKEN_TYPE

enum class TokenType {
#define TOKEN_TYPE(e, s) e,
        ALL_TOKEN_TYPES
#undef TOKEN_TYPE
};
#undef TOKEN_TYPE
#define TOKEN_TYPE(e, s) s,
static const char *TOKEN_STRINGS[] = {ALL_TOKEN_TYPES};
#undef TOKEN_TYPE

struct Token {
        enum TokenType type = TokenType::OR;
        uint32_t line = 0;
        uint32_t column = 0;
        uint32_t indent_level;
        std::string value = "";

        int precedence();
        bool is_binary_op();
        bool is_unary_op();
        bool is_literal();
        bool is_comparrison_op();
        bool is_augassign_op();
        bool is_num();
};

struct InputStream {
        const char *filename;
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
        Token last_returned;
        uint32_t indent_level = 0;
        uint32_t paren_count = 0;
        // get_next_token will return the next lookahead token not the next token for the parser
};

struct TokenArray {
        Token *tokens;
        size_t size;
        uint64_t position;
        Token current;
        Token lookahead;
        const char *filename;

        Token next_token();
};

Token tokeniser_get_next_token(Tokeniser *tokeniser, InputStream *stream);

#endif // TOKENISER_H_
