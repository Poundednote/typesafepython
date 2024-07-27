#include <stdint.h>
#include <stdio.h>

#include "tokeniser.h"

bool Token::is_binary_op() {
    switch (this->type) {
    case TokenType::MULTIPLICATION:
        return true;

    case TokenType::DIVISION:
        return true;

    case TokenType::ADDITION:
        return true;

    case TokenType::SUBTRACTION:
        return true;

    case TokenType::FLOOR_DIV:
        return true;

    case TokenType::EXPONENTIATION:
        return true;

    case TokenType::REMAINDER:
        return true;

    case TokenType::EQ:
        return true;

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

    case TokenType::GT:
        return true;

    case TokenType::IS:
        return true;

    case TokenType::IN_TOK:
        return true;

    case TokenType::OR:
        return true;

    case TokenType::AND:
        return true;

    case TokenType::NOT:
        return true;

    case TokenType::BWOR:
        return true;

    case TokenType::BWXOR:
        return true;

    case TokenType::BWAND:
        return true;

    case TokenType::SHIFTLEFT:
        return true;

    case TokenType::SHIFTRIGHT:
        return true;

    default:
        return false;
    }
}

bool Token::is_literal() {
    switch (this->type) {
    case TokenType::IDENTIFIER:
        return true;
        break;

    case TokenType::NONE:
        return true;
        break;

    case TokenType::BOOL_TRUE:
        return true;
        break;

    case TokenType::BOOL_FALSE:
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


bool Token::is_comparrison_op() {
    switch (this->type) {
    case TokenType::EQ:
        return true;

    case TokenType::NE:
        return true;

    case TokenType::LE:
        return true;

    case TokenType::LT:
        return true;

    case TokenType::GE:
        return true;

    case TokenType::GT:
        return true;

    case TokenType::IS:
        return true;

    case TokenType::IN_TOK:
        return true;

    default:
        return false;
    }
}

int Token::precedence() {
    switch (this->type) {
    // map values of equivalent precence to each other
    case TokenType::SUBTRACTION:
        return (int)TokenType::ADDITION;

    case TokenType::DIVISION:
        return (int)TokenType::MULTIPLICATION;

    case TokenType::FLOOR_DIV:
        return (int)TokenType::MULTIPLICATION;

    case TokenType::REMAINDER:
        return (int)TokenType::MULTIPLICATION;

    case TokenType::OPEN_PAREN:
        return 0;

    default:
        return (int)this->type;
    }
}

std::string debug_token_type_to_string(enum TokenType type) {
    switch (type) {
    case TokenType::NEWLINE: return "\\n";
    case TokenType::INDENT: return "INDENT";
    case TokenType::DEDENT: return "DEDENT";
    case TokenType::OPEN_PAREN: return "(";
    case TokenType::CLOSED_PAREN: return ")";
    case TokenType::SQUARE_OPEN_PAREN: return "[";
    case TokenType::SQUARE_CLOSED_PAREN: return "]";
    case TokenType::CURLY_OPEN_PAREN: return "{";
    case TokenType::CURLY_CLOSED_PAREN: return "{";
    case TokenType::COMMA: return ",";
    case TokenType::DOT: return ".";
    case TokenType::ASSIGN: return "=";
    case TokenType::COLON: return ":";
    case TokenType::OR: return "or";
    case TokenType::AND: return "and";
    case TokenType::NOT: return "not";
    case TokenType::EQ: return "==";
    case TokenType::NE: return "!=";
    case TokenType::LE: return "<=";
    case TokenType::LT: return "<";
    case TokenType::GE: return ">=";
    case TokenType::GT: return ">";
    case TokenType::IS: return "is";
    case TokenType::IN_TOK: return "in";
    case TokenType::BWOR: return "|";
    case TokenType::BWXOR: return "^";
    case TokenType::BWAND: return "&";
    case TokenType::SHIFTLEFT: return "<<";
    case TokenType::SHIFTRIGHT: return ">>";
    case TokenType::ADDITION: return "+";
    case TokenType::SUBTRACTION: return "-";
    case TokenType::MULTIPLICATION: return "*";
    case TokenType::DIVISION: return "/";
    case TokenType::FLOOR_DIV: return "//";
    case TokenType::REMAINDER: return "%";
    case TokenType::EXPONENTIATION: return "**";
    case TokenType::IDENTIFIER: return "identifier";
    case TokenType::INT_LIT: return "INT";
    case TokenType::FLOAT_LIT: return "FLOAT";
    case TokenType::STRING_LIT: return "STRING";
    case TokenType::NONE: return "None";
    case TokenType::BOOL_TRUE: return "True";
    case TokenType::BOOL_FALSE: return "False";
    case TokenType::RETURN: return "return";
    case TokenType::RAISE: return "raise";
    case TokenType::GLOBAL: return "global";
    case TokenType::NONLOCAL: return "nonlocal";
    case TokenType::FILE: return "FILE";
    case TokenType::ENDFILE: return "EOF";
    case TokenType::YIELD: return "yield";
    case TokenType::IF: return "if";
    case TokenType::ELIF: return "elif";
    case TokenType::ELSE: return "else";
    case TokenType::ARROW: return "ARROW";
    case TokenType::DEF: return "def";
    case TokenType::CLASS: return "class";
    case TokenType::WHILE: return "while";
    case TokenType::FOR: return "for";
    case TokenType::TRY: return "try";
    case TokenType::EXCEPT: return "except";
    case TokenType::AS: return "as";
    case TokenType::FINALLY: return "finally";
    case TokenType::PASS: return "pass";
    case TokenType::BREAK: return "break";
    case TokenType::CONTINUE: return "continue";
    case TokenType::AT: return "@";
    case TokenType::IMPORT: return "import";
    }

    return "UNKNOWN";
}

std::string token_to_string(Token token) {
    switch (token.type) {
    case TokenType::OR: return "or";
    case TokenType::AND: return "and";
    case TokenType::NOT: return "not";
    case TokenType::EQ: return "==";
    case TokenType::NE: return "!=";
    case TokenType::LE: return "<=";
    case TokenType::LT: return "<";
    case TokenType::GE: return ">=";
    case TokenType::GT: return ">";
    case TokenType::IS: return "is";
    case TokenType::IN_TOK: return "in";
    case TokenType::BWOR: return "|";
    case TokenType::BWXOR: return "^";
    case TokenType::BWAND: return "&";
    case TokenType::SHIFTLEFT: return "<<";
    case TokenType::SHIFTRIGHT: return ">>";
    case TokenType::ADDITION: return "+";
    case TokenType::SUBTRACTION: return "-";
    case TokenType::MULTIPLICATION: return "*";
    case TokenType::DIVISION: return "/";
    case TokenType::FLOOR_DIV: return "//";
    case TokenType::REMAINDER: return "%";
    case TokenType::EXPONENTIATION: return "**";
    case TokenType::DOT: return ".";
    case TokenType::RETURN: return "return";
    case TokenType::YIELD: return "yield";
    case TokenType::RAISE: return "return";
    case TokenType::GLOBAL: return "global";
    case TokenType::NONLOCAL: return "nonlocal";
    case TokenType::IF: return "if";
    case TokenType::ELIF: return "elif";
    case TokenType::ELSE: return "else";
    case TokenType::DEF: return "def";
    case TokenType::CLASS: return "class";
    case TokenType::WHILE: return "while";
    case TokenType::FOR: return "for";
    case TokenType::TRY: return "try";
    case TokenType::WITH: return "with";
    case TokenType::EXCEPT: return "except";
    case TokenType::FINALLY: return "finally";
    case TokenType::AS: return "as";
    case TokenType::PASS: return "pass";
    case TokenType::CONTINUE: return "continue";
    case TokenType::BREAK: return "break";
    case TokenType::IDENTIFIER: return token.value;
    case TokenType::INT_LIT: return token.value;
    case TokenType::FLOAT_LIT: return token.value;
    case TokenType::STRING_LIT: return token.value;
    case TokenType::NONE: return "none";
    case TokenType::BOOL_TRUE: return "True";
    case TokenType::BOOL_FALSE: return "False";
    case TokenType::OPEN_PAREN: return "(";
    case TokenType::CLOSED_PAREN: return ")";
    case TokenType::SQUARE_OPEN_PAREN: return "[";
    case TokenType::SQUARE_CLOSED_PAREN: return "]";
    case TokenType::CURLY_OPEN_PAREN: return "{";
    case TokenType::CURLY_CLOSED_PAREN: return "}";
    case TokenType::COMMA: return ",";
    case TokenType::ASSIGN: return "=";
    case TokenType::COLON: return ":";
    case TokenType::ARROW: return "->";
    case TokenType::AT: return "at";
    case TokenType::IMPORT: return "import";
    case TokenType::NEWLINE: return "\n";
    case TokenType::INDENT: return "    ";
    case TokenType::DEDENT: return "";
    case TokenType::FILE: return "";
    case TokenType::ENDFILE: return "";
    }
}

InputStream InputStream::create_from_file(const char *filename) {
    // initilise input stream

    InputStream input_stream = {};
    FILE *target_f;

    fopen_s(&target_f, filename, "rb");
    if (!target_f) {
        perror("Couldn't open file");
        // cant compile a file that can't be opened
        exit(1);
    }

    // compute filesize
    fseek(target_f, 0, SEEK_END);
    input_stream.size = ftell(target_f);
    fseek(target_f, 0, SEEK_SET);
    input_stream.contents = new char[input_stream.size+1];
    input_stream.line = 1;

    if (input_stream.contents == nullptr) {
        // this shouldn't happend in practice
        exit(1);
    }

    // read contents into file
    fread_s(input_stream.contents, input_stream.size, 1, input_stream.size, target_f);
    input_stream.contents[input_stream.size] = 0;
    fclose(target_f);

    return input_stream;
}

void InputStream::destroy() {
    delete[] this->contents;
    *this = {};
}

char InputStream::peek(uint32_t ahead) {
    if ((this->position + ahead) >= this->size) {
        return 0;
    }

    assert(this->position < size);
    return (this->contents)[this->position + ahead];
}

char InputStream::next_char() {
    char current_char = this->contents[this->position];
    if (current_char == '\0') {
        return 0;
    }

    ++this->col;
    if (current_char == '\n') {
        ++this->line;
        this->col = 0;
    }

    assert(this->position+1 <= size);
    return this->contents[++this->position];
}

Tokeniser Tokeniser::init(InputStream *stream) {
    Tokeniser tokeniser = {};
    tokeniser.stream = stream;
    tokeniser.last_returned = tokeniser.get_next_lookahead();
    tokeniser.lookahead = tokeniser.last_returned;
    tokeniser.lookahead = tokeniser.get_next_lookahead();

    return tokeniser;
}

Token Tokeniser::next_token() {
    // save to temp because the get_next_token depends on the
    // last returned token value
    this->last_returned = this->lookahead;
    this->lookahead = this->get_next_lookahead();
    return last_returned;
};

Token Tokeniser::get_next_lookahead() {
    std::string buffer = "";
    char current = this->stream->peek(0);

    Token token = {};
    token.line = this->stream->line;
    token.column = this->stream->col;

    while (this->dedents_to_process > 0) {
        token.type = TokenType::DEDENT;
        --indent_level;
        --this->dedents_to_process;

        return token;
    }

    // Check for dedents after newline
    if (this->lookahead.type == TokenType::NEWLINE &&
        this->indent_level >= 1) {

        for (int i = 0; i < indent_level*4; ++i) {
            if (!current) {
                break;
            }

            if (current != ' ') {
                if (i % 4 != 0 ) {
                    printf("Bad Indentation on line: %d \nIndentation is 4 spaces\n",
                           this->stream->line);
                    exit(1);
                }

                token.type = TokenType::DEDENT;
                this->dedents_to_process = --this->indent_level - i/4;
                return token;
            }

            current = this->stream->next_char();
        }
    }

    if ((this->lookahead.type == TokenType::NEWLINE ||
        this->lookahead.type == TokenType::INDENT)
        && current == ' ') {

        int spaces = 0;
        for (; current; current = this->stream->next_char()) {
            if (current != ' ') {
                break;
            }

            ++spaces;

            if (spaces == 4) {
                ++this->indent_level;
                spaces = 0;
                token.type = TokenType::INDENT;
                this->stream->next_char();
                return token;
            }
        }
    }

    // Eat whitespace
    while (current == ' ') {
        current = this->stream->next_char();
        token.line = this->stream->line;
        token.column = this->stream->col;
    }

    // Skip \r for windows users
    while (current == '\r') {
        current = this->stream->next_char();
        token.line = this->stream->line;
        token.column = this->stream->col;
    }

    // parse indent if on newline

    // KEYWORDS
    if (std::isalpha(current)) {
        buffer.push_back(current);
        for (char alpha_char = this->stream->next_char(); alpha_char;
                alpha_char = this->stream->next_char()) {

            if (!(std::isalnum(alpha_char) || alpha_char == '_')) {
                break;
            }

            buffer.push_back(alpha_char);
        }

        // KEWORDS
        if (buffer == "is") {token.type = TokenType::IS;}
        else if (buffer == "in") {token.type = TokenType::IN_TOK;}
        else if (buffer == "or") {token.type = TokenType::OR;}
        else if (buffer == "and") {token.type = TokenType::AND;}
        else if (buffer == "not") {token.type = TokenType::NOT;}
        else if (buffer == "True") {token.type = TokenType::BOOL_TRUE;}
        else if (buffer == "False") {token.type = TokenType::BOOL_FALSE;}
        else if (buffer == "None") {token.type = TokenType::NONE;}
        else if (buffer == "return") {token.type = TokenType::RETURN;}
        else if (buffer == "yield") {token.type = TokenType::YIELD;}
        else if (buffer == "raise") {token.type = TokenType::RAISE;}
        else if (buffer == "global") {token.type = TokenType::GLOBAL;}
        else if (buffer == "nonlocal") {token.type = TokenType::NONLOCAL;}
        else if (buffer == "if") {token.type = TokenType::IF;}
        else if (buffer == "elif") {token.type = TokenType::ELIF;}
        else if (buffer == "else") {token.type = TokenType::ELSE;}
        else if (buffer == "def") {token.type = TokenType::DEF;}
        else if (buffer == "class") {token.type = TokenType::CLASS;}
        else if (buffer == "while") {token.type = TokenType::WHILE;}
        else if (buffer == "for") {token.type = TokenType::FOR;}
        else if (buffer == "try") {token.type = TokenType::TRY;}
        else if (buffer == "with") {token.type = TokenType::WITH;}
        else if (buffer == "except") {token.type = TokenType::EXCEPT;}
        else if (buffer == "as") {token.type = TokenType::AS;}
        else if (buffer == "finally") {token.type = TokenType::FINALLY;}
        else if (buffer == "pass") {token.type = TokenType::PASS;}
        else if (buffer == "break") {token.type = TokenType::BREAK;}
        else if (buffer == "continue") {token.type = TokenType::CONTINUE;}
        else if (buffer == "import") {token.type = TokenType::IMPORT;}

        else {
            token.type = TokenType::IDENTIFIER;
            token.value = buffer;
        }

        return token;
    }

    // NUMBERS
    else if (std::isdigit(current)) {
        buffer.push_back(current);
        bool decimal = false;
        for (char digit_char = this->stream->next_char(); digit_char;
                digit_char = this->stream->next_char()) {

            if (digit_char == '.') {
                decimal = true;
            }

            else if (!std::isdigit(digit_char)) {
                break;
            }

            buffer.push_back(digit_char);
 }

        token.type = TokenType::INT_LIT;
        token.value = buffer;

        if (decimal) {
            token.type = TokenType::FLOAT_LIT;
        }

        return token;
    }

    switch (current) {
        case '(': {token.type = TokenType::OPEN_PAREN;} break;
        case ')': {token.type = TokenType::CLOSED_PAREN;} break;
        case '[': {token.type = TokenType::SQUARE_OPEN_PAREN;} break;
        case ']': {token.type = TokenType::SQUARE_CLOSED_PAREN;} break;
        case '{': {token.type = TokenType::CURLY_OPEN_PAREN;} break;
        case '}': {token.type = TokenType::CURLY_CLOSED_PAREN;} break;
        case ',': {token.type = TokenType::COMMA;} break;
        case '.': {token.type = TokenType::DOT;} break;
        case ':': {token.type = TokenType::COLON;} break;
        case '%': {token.type = TokenType::REMAINDER;} break;
        case '+': {token.type = TokenType::ADDITION;} break;
        case '&': {token.type = TokenType::BWAND;} break;
        case '^': {token.type = TokenType::BWXOR;} break;
        case '|': {token.type = TokenType::BWOR;} break;
        case '@': {token.type = TokenType::AT;} break;

        case '\n': {
            token.type = TokenType::NEWLINE;
            // Eat following newlines
            while (this->stream->peek(1) == '\n' || this->stream->peek(1) == '\r') {
                this->stream->next_char();
            }

        } break;

        case '"': {
            for (char next = this->stream->next_char();
                 next;
                 next = this->stream->next_char()) {

                if (next == '"') break;
                buffer.push_back(next);
            }

            token.value = buffer;
            token.type = TokenType::STRING_LIT;
            buffer.clear();
        } break;

        case '!': {
            if (this->stream->peek(1) == '=') {
                token.type = TokenType::NE;
                this->stream->next_char();
            }

        } break;

        case '=': {
            if (this->stream->peek(1) == '=') {
                token.type = TokenType::EQ;
                this->stream->next_char();
                break;
            }

            token.type = TokenType::ASSIGN;
        } break;

        case '*': {
            if (this->stream->peek(1) == '*') {
                token.type = TokenType::EXPONENTIATION;
                this->stream->next_char();
                break;
            }

            token.type = TokenType::MULTIPLICATION;
        } break;

        case '/': {
            if (this->stream->peek(1) == '/') {
                token.type = TokenType::FLOOR_DIV;
                this->stream->next_char();
                break;
            }
            token.type = TokenType::DIVISION;
        } break;

        case '-': {
            if (this->stream->peek(1) == '>') {
                token.type = TokenType::ARROW;
                this->stream->next_char();
                break;
            }
            token.type = TokenType::SUBTRACTION;
        } break;

        case '<': {
            if (this->stream->peek(1) == '<') {
                token.type = TokenType::SHIFTLEFT;
                this->stream->next_char();
                break;
            }

            if (this->stream->peek(1) == '=') {
                token.type = TokenType::LE;
                this->stream->next_char();
                break;
            }

            token.type = TokenType::LT;

        } break;

        case '>': {
            if (this->stream->peek(1) == '>') {
                token.type = TokenType::SHIFTRIGHT;
                this->stream->next_char();
                break;
            }

            else if (this->stream->peek(1) == '=') {
                token.type = TokenType::GE;
                this->stream->next_char();
                break;
            }

            token.type = TokenType::GT;
        } break;

        case 0: {
            if (indent_level > 0) {
                token.type = TokenType::DEDENT;
                this->dedents_to_process = --indent_level;
            }

            else {
                token.type = TokenType::ENDFILE;
            }
        }
    }

    this->stream->next_char();
    return token;
}
