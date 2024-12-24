#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
struct Tokeniser {
        char *at;
};

void next_char(Tokeniser *tokeniser)
{
        if (!(*(tokeniser->at))) {
                return;
        }

        tokeniser->at++;
}

bool match_key_word(Tokeniser *tokeniser, const char *keyword, size_t keyword_size)
{
        for (int i = 0; i < keyword_size && tokeniser->at; ++i) {
                if (*(tokeniser->at) != *keyword++) {
                        return false;
                }

                next_char(tokeniser);
        }

        return true;

}
bool is_whitespace(char c)
{
        return (c == ' ' || c == '\r' || c == '\n');
}

void eat_whitespace(Tokeniser *tokeniser)
{
        while (is_whitespace(*(tokeniser->at))) {
                next_char(tokeniser);
        }

        // check for comments
}

void eat_comments(Tokeniser *tokeniser) {

        while (match_key_word(tokeniser, "//", 2)) {
                while(*(tokeniser->at) && *(tokeniser->at) != '\n') {
                        next_char(tokeniser);
                }
                eat_whitespace(tokeniser);
        }
}

void goto_next_word(Tokeniser *tokeniser) {
        while(*(tokeniser->at) && !is_whitespace(*(tokeniser->at))) {
                eat_whitespace(tokeniser);
                next_char(tokeniser);
        }

        eat_whitespace(tokeniser);
        eat_comments(tokeniser);
}

int get_into_buffer_until_char_ignore_whitespace(Tokeniser *tokeniser, char c, char *buffer,
                                    size_t buffer_size)
{
        int i = 0;
        while (*(tokeniser->at) && *(tokeniser->at) != c) {
                if (i >= buffer_size) {
                        perror("buffer too smalle for name");
                        return 0;
                }
                buffer[i++] = *(tokeniser->at);
                next_char(tokeniser);
                eat_whitespace(tokeniser);
        }

        goto_next_word(tokeniser);
        return i;
}

int get_into_buffer_until_char(Tokeniser *tokeniser, char c, char *buffer,
                               size_t buffer_size)
{
        int i = 0;
        while (*(tokeniser->at) && *(tokeniser->at) != c) {
                if (i >= buffer_size) {
                        perror("buffer too smalle for name");
                        return 0;
                }
                buffer[i++] = *tokeniser->at;
                next_char(tokeniser);
        }

        goto_next_word(tokeniser);
        return i;
}

int inline char_to_num(char c) {
        return c - '0';
}

void parse_struct_members(Tokeniser *tokeniser, char *struct_name, int name_size)
{
        char buffer[256];
        while (*(tokeniser->at) && *(tokeniser->at) != '}') {
                int bytes_written =
                        get_into_buffer_until_char(
                                tokeniser, ' ', buffer,
                                sizeof(buffer));
                // match pointer
                if (match_key_word(tokeniser, "*", 1)) {
                        const char *pointer = "_PTR";

                        for (int i = 0; bytes_written < sizeof(buffer) &&
                                        i < sizeof("_PTR") - 1;
                             ++bytes_written, ++i) {
                                buffer[bytes_written] = *(pointer++);
                        }
                }

                printf("{TYPE_%.*s, ", bytes_written,
                       buffer);

                bytes_written = get_into_buffer_until_char(
                        tokeniser, ';', buffer,
                        sizeof(buffer));

                printf("\"%.*s\", ", bytes_written,
                       buffer);
                printf("(uint64_t)&((%.*s *)0)->%.*s", name_size,
                       struct_name, bytes_written, buffer);

                printf("},\n");
        }
}

void parse_struct(Tokeniser *tokeniser)
{
        

        char name[256];
        goto_next_word(tokeniser);

        int name_bytes_written =
                get_into_buffer_until_char_ignore_whitespace(
                        tokeniser, '{', name,
                        sizeof(name));

        printf("StructMemberDefinition %.*sStructMembers[] = \n{\n", name_bytes_written,
               name);

        parse_struct_members(tokeniser, name, name_bytes_written);

        printf("};");

        printf("\n");

}
void parse_enum(Tokeniser *tokeniser)
{
        char buffer[256];
        goto_next_word(tokeniser);
        match_key_word(tokeniser, "class",
                       sizeof("class") - 1);

        int bytes_written =
                get_into_buffer_until_char_ignore_whitespace(
                        tokeniser, '{', buffer,
                        sizeof(buffer));

        printf("EnumMemberDefinition %.*sEnumMembers[] =\n{\n",
               bytes_written, buffer);

        int last_val = 0;
        while (*(tokeniser->at) && *(tokeniser->at) != '}') {
                bytes_written = 0;
                while (*(tokeniser->at) &&
                       *(tokeniser->at) != ' ' &&
                       *(tokeniser->at) != ',') {
                        if (bytes_written >=
                            sizeof(buffer)) {
                                perror("buffer too small for name");
                                bytes_written = 0;
                                break;
                        }
                        buffer[bytes_written++] =
                                *(tokeniser->at);
                        next_char(tokeniser);
                }

                goto_next_word(tokeniser);

                printf("{\"%.*s\", ", bytes_written,
                       buffer);
                if (match_key_word(
                            tokeniser, "=",
                            1)) {
                        goto_next_word(tokeniser);
                        bytes_written =
                                get_into_buffer_until_char(
                                        tokeniser, ',', buffer,
                                        sizeof(buffer));

                        printf("%.*s", bytes_written,
                               buffer);

                        last_val = 0;
                        for (int i = 0;
                             i < bytes_written; ++i) {
                                last_val =
                                        last_val * 10 +
                                        char_to_num(
                                                buffer[i]);
                        }

                } else {
                        printf("%.d", ++last_val);
                } // skip the equals if there is one
                printf("},\n");
        }
        printf("};\n");
}

int main(int argc, char **argv)
{
        FILE *target_f = nullptr;
        if (argc < 2) {
                perror("need args");
                exit(1);
        }
        fopen_s(&target_f, argv[1], "rb");
        if (!target_f) {
                perror("Couldn't open file");
                // cant compile a file that can't be opened
                exit(1);
        }

        // compute filesize
        fseek(target_f, 0, SEEK_END);
        size_t filesize = ftell(target_f);
        fseek(target_f, 0, SEEK_SET);
        char *file_buffer = (char *)malloc(filesize + 1);
        if (!file_buffer) {
                printf("malloc failed");
                exit(1);
        }

        // read contents into file
        fread_s(file_buffer, filesize, 1, filesize, target_f);
        file_buffer[filesize] = '\0';
        fclose(target_f);

        Tokeniser tokeniser = {};
        tokeniser.at = file_buffer;
        while (*(tokeniser.at)) {
                eat_whitespace(&tokeniser);
                if (!match_key_word(&tokeniser, "introspect",
                                    sizeof("introspect") - 1)) {
                        next_char(&tokeniser);
                        continue;
                }

                eat_whitespace(&tokeniser);
                if (match_key_word(&tokeniser, "struct",
                                   sizeof("struct") - 1)) {
                        parse_struct(&tokeniser);
                } else if (match_key_word(&tokeniser, "enum",
                                          sizeof("enum") - 1)) {
                        parse_enum(&tokeniser);
                }
                continue;
        }
}
