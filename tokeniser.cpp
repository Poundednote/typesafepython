#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "tokeniser.h"
#include "utils.h"

//FIXME the person who wrote this tokeniser is a fucking retard and it
//is filled with bad abstractions that dont fucking work the input stream
//concept is just straight retarded its now hard to add anything to it because its such a
//peice of shit. it was me I am the retard

bool Token::is_binary_op()
{
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

        case TokenType::NOT_IN:
                return true;

        case TokenType::OR:
                return true;

        case TokenType::AND:
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

bool Token::is_unary_op()
{
        switch (this->type) {
        case TokenType::NOT:
                return true;
        case TokenType::SUBTRACTION:
                return true;

        default:
               return false;
        }

}

bool Token::is_literal()
{
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

bool Token::is_comparrison_op()
{
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

int Token::precedence()
{
        switch (this->type) {
        // map values of equivalent precence to each other
        case TokenType::SUBTRACTION:
                return (int)TokenType::ADDITION + 1;

        case TokenType::DIVISION:
                return (int)TokenType::MULTIPLICATION +1;

        case TokenType::FLOOR_DIV:
                return (int)TokenType::MULTIPLICATION + 1;

        case TokenType::REMAINDER:
                return (int)TokenType::MULTIPLICATION + 1;

        case TokenType::OPEN_PAREN:
                return 0;

        default:
                return (int)this->type+1;
        }
}


InputStream InputStream::create_from_file(const char *filename)
{
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
        input_stream.contents = new char[input_stream.size + 1];
        input_stream.line = 1;

        if (input_stream.contents == nullptr) {
                // this shouldn't happend in practice
                exit(1);
        }

        // read contents into file
        fread_s(input_stream.contents, input_stream.size, 1, input_stream.size,
                target_f);
        input_stream.contents[input_stream.size] = 0;
        fclose(target_f);

        return input_stream;
}

void InputStream::destroy()
{
        delete[] this->contents;
        *this = {};
}

char InputStream::peek(uint32_t ahead)
{
        if ((this->position + ahead) >= this->size) {
                return 0;
        }

        assert(this->position < size);
        return (this->contents)[this->position + ahead];
}

char InputStream::next_char()
{
        char current_char = this->contents[this->position];
        if (current_char == '\0') {
                return 0;
        }

        ++this->col;
        if (current_char == '\n') {
                ++this->line;
                this->col = 0;
        }

        assert(this->position + 1 <= size);
        return this->contents[++this->position];
}

char eat_whitespace(InputStream *stream)
{
        while (stream->peek(0) == ' ' || stream->peek(0) == '\r') {
                stream->next_char();
        }

        return stream->peek(0);
}

char eat_newline(InputStream *stream)
{
        while (stream->peek(0) == '\n' ||
               stream->peek(0) == '\r') {
               stream->next_char();
        }

        return stream->peek(0);

}

bool match_string(const char *string, size_t string_size, InputStream *stream)
{
        for (int i = 0; i < string_size; ++i) {
                if (string[i] != stream->peek(i))
                        return false;
        }

        for (int i = 0; i < string_size; ++i) {
                stream->next_char();
        }

        return true;
}

void parse_string_into_buffer(char c, InputStream *stream, std::string &buffer)
{
        char next = stream->peek(0);
        for (; next; next = stream->next_char()) {
                if (next == c)
                        break;
                buffer.push_back(next);
        }
}

bool match_string_and_parse_into_buffer(InputStream *stream, std::string &buffer)
{
        if (match_string("f\"", 2, stream) || match_string("r\"", 2, stream) ||
            match_string("u\"", 2, stream) || match_string("b\"", 2, stream) ||
            match_string("fr\"", 3, stream) ||
            match_string("rf\"", 3, stream) ||
            match_string("br\"", 3, stream) ||
            match_string("rb\"", 3, stream) || match_string("\"", 1, stream)) {
                parse_string_into_buffer('"', stream, buffer);
                return true;
        }

        if (match_string("f'", 2, stream) || match_string("r'", 2, stream) ||
            match_string("u'", 2, stream) || match_string("b'", 2, stream) ||
            match_string("fr'", 3, stream) || match_string("rf'", 3, stream) ||
            match_string("br'", 3, stream) || match_string("rb'", 3, stream) ||
            match_string("'", 1, stream)) {
                parse_string_into_buffer('\'', stream, buffer);
                return true;
        }

        return false;
}

char check_for_comments(InputStream *stream)
{
        char current = stream->peek(0);
        if (match_string("\"\"\"", sizeof("\"\"\"")-1, stream)) {
                while (!match_string("\"\"\"", sizeof("\"\"\"")-1,
                                     stream)) {
                        current = stream->next_char();
                }

                current = stream->peek(0);
                current = eat_whitespace(stream);
        }

        if (match_string("'''", sizeof("'''")-1, stream)) {
                while (!match_string("'''", sizeof("'''")-1, stream)) {
                        current = stream->next_char();
                }

                current = stream->peek(0);
                current = eat_whitespace(stream);
        }


        if (current == '#') {
                while (current != '\n') {
                        current = stream->next_char();
                }
        }

        return current;
}

Tokeniser Tokeniser::init(InputStream *stream)
{
        Tokeniser tokeniser = {};
        tokeniser.stream = stream;
        tokeniser.last_returned = tokeniser.get_next_lookahead();
        tokeniser.current_indent_level = tokeniser.lookahead_indent_level;
        tokeniser.lookahead = tokeniser.last_returned;
        tokeniser.lookahead = tokeniser.get_next_lookahead();

        return tokeniser;
}

Token Tokeniser::next_token()
{
        this->last_returned = this->lookahead;
        this->current_indent_level = this->lookahead_indent_level;
        this->lookahead = this->get_next_lookahead();
        return last_returned;
};

Token Tokeniser::get_next_lookahead()
{
        std::string buffer = "";
        char current = this->stream->peek(0);

        Token token = {};

        // TODO there may or may not be a better way of doing this indentation
        // Check for dedents after newline
        if (this->lookahead.type == TokenType::NEWLINE && !paren_count) {
                this->lookahead_indent_level = 0;
                int spaces = 0;
                for (; current && current == ' ';
                     current = this->stream->next_char()) {
                        ++spaces;

                        if (spaces == 4) {
                                ++this->lookahead_indent_level;
                                spaces = 0;
                        }
                }
        }


        current = eat_whitespace(this->stream);
        current = check_for_comments(this->stream);

        token.line = this->stream->line;
        token.column = this->stream->col;

        if (match_string_and_parse_into_buffer(stream, buffer)) {
                token.value = buffer;
                token.type = TokenType::STRING_LIT;
                stream->next_char();
                eat_whitespace(stream);
                while (this->stream->peek(0) == '\n' && this->paren_count) {
                        this->stream->next_char();
                        eat_whitespace(this->stream);
                        check_for_comments(this->stream);
                        if (match_string_and_parse_into_buffer(stream, buffer))
                                this->stream->next_char();
                }
                eat_whitespace(this->stream);

                token.value = buffer;
                return token;
        }

        // KEYWORDS
        if (std::isalpha(current) || current == '_') {
                buffer.push_back(current);
                for (char alpha_char = this->stream->next_char(); alpha_char;
                     alpha_char = this->stream->next_char()) {
                        if (!(std::isalnum(alpha_char) || alpha_char == '_')) {
                                break;
                        }

                        buffer.push_back(alpha_char);
                }

                // KEWORDS
                if (buffer == "is") {
                        token.type = TokenType::IS;
                } else if (buffer == "in") {
                        token.type = TokenType::IN_TOK;
                } else if (buffer == "or") {
                        token.type = TokenType::OR;
                } else if (buffer == "and") {
                        token.type = TokenType::AND;
                } else if (buffer == "not") {
                        token.type = TokenType::NOT;
                        eat_whitespace(this->stream);
                        buffer.clear();
                        if(match_string("in", sizeof("in") - 1, this->stream))
                                token.type = TokenType::NOT_IN;

                } else if (buffer == "True") {
                        token.type = TokenType::BOOL_TRUE;
                } else if (buffer == "False") {
                        token.type = TokenType::BOOL_FALSE;
                } else if (buffer == "None") {
                        token.type = TokenType::NONE;
                } else if (buffer == "return") {
                        token.type = TokenType::RETURN;
                } else if (buffer == "yield") {
                        token.type = TokenType::YIELD;
                } else if (buffer == "raise") {
                        token.type = TokenType::RAISE;
                } else if (buffer == "global") {
                        token.type = TokenType::GLOBAL;
                } else if (buffer == "nonlocal") {
                        token.type = TokenType::NONLOCAL;
                } else if (buffer == "if") {
                        token.type = TokenType::IF;
                } else if (buffer == "elif") {
                        token.type = TokenType::ELIF;
                } else if (buffer == "else") {
                        token.type = TokenType::ELSE;
                } else if (buffer == "def") {
                        token.type = TokenType::DEF;
                } else if (buffer == "class") {
                        token.type = TokenType::CLASS;
                } else if (buffer == "while") {
                        token.type = TokenType::WHILE;
                } else if (buffer == "for") {
                        token.type = TokenType::FOR;
                } else if (buffer == "try") {
                        token.type = TokenType::TRY;
                } else if (buffer == "with") {
                        token.type = TokenType::WITH;
                } else if (buffer == "except") {
                        token.type = TokenType::EXCEPT;
                } else if (buffer == "as") {
                        token.type = TokenType::AS;
                } else if (buffer == "finally") {
                        token.type = TokenType::FINALLY;
                } else if (buffer == "pass") {
                        token.type = TokenType::PASS;
                } else if (buffer == "break") {
                        token.type = TokenType::BREAK;
                } else if (buffer == "continue") {
                        token.type = TokenType::CONTINUE;
                } else if (buffer == "import") {
                        token.type = TokenType::IMPORT;
                } else if (buffer == "del") {
                        token.type = TokenType::DEL;
                } else if (buffer == "match") {
                        token.type = TokenType::MATCH;
                } else if (buffer == "case") {
                        token.type = TokenType::MATCH;
                } else if (buffer == "from") {
                        token.type = TokenType::FROM;
                }

                else {
                        token.type = TokenType::IDENTIFIER;
                        token.value = buffer;
                }

                return token;
        }

        // NUMBERS
        else if (std::isdigit(current)) {
                bool decimal = false;

                if (current == '0') {
                        char next = this->stream->peek(1);
                        if (next == 'x' || next == 'o' || next == 'b') {
                                this->stream->next_char();
                                this->stream->next_char();
                        } else {
                                buffer.push_back(current);
                        }
                } else {
                        buffer.push_back(current);
                }

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
        case '(': {
                token.type = TokenType::OPEN_PAREN;
                this->paren_count++;
        } break;
        case ')': {
                token.type = TokenType::CLOSED_PAREN;
                this->paren_count--;
        } break;
        case '[': {
                token.type = TokenType::SQUARE_OPEN_PAREN;
                this->paren_count++;
        } break;
        case ']': {
                token.type = TokenType::SQUARE_CLOSED_PAREN;
                this->paren_count--;
        } break;
        case '{': {
                token.type = TokenType::CURLY_OPEN_PAREN;
                this->paren_count++;
        } break;
        case '}': {
                token.type = TokenType::CURLY_CLOSED_PAREN;
                this->paren_count--;
        } break;
        case ',': {
                token.type = TokenType::COMMA;
        } break;
        case '.': {
                token.type = TokenType::DOT;
        } break;
        case ':': {
                if (this->stream->peek(1) == '=') {
                        this->stream->next_char();
                        this->stream->next_char();
                        token.type = TokenType::COLON_EQUAL;
                } else {
                        token.type = TokenType::COLON;
                }
        } break;
        case '%': {
                token.type = TokenType::REMAINDER;
        } break;
        case '+': {
                token.type = TokenType::ADDITION;
        } break;
        case '&': {
                token.type = TokenType::BWAND;
        } break;
        case '^': {
                token.type = TokenType::BWXOR;
        } break;
        case '|': {
                token.type = TokenType::BWOR;
        } break;
        case '@': {
                token.type = TokenType::AT;
        } break;

        case '\n': {
                if (paren_count) {
                        stream->next_char();
                        return get_next_lookahead(); // skip newline
                }

                token.type = TokenType::NEWLINE;
                // Eat following Newlines
                while (stream->peek(1) == '\n' ||
                        stream->peek(1) == '\r') {
                        stream->next_char();
                }

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
                token.type = TokenType::ENDFILE;
        }

        }

        this->stream->next_char();
        return token;
}
