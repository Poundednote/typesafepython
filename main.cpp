#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <sys/stat.h>

#include "parser.cpp"
#include "tokeniser.cpp"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Please specify a file";
        return EXIT_FAILURE;
    }

    // open file
    FILE *target_f = fopen(argv[1], "r");
    if (!target_f) {
        perror("Couldn't open file");
    }

    // compute filesize
    fseek(target_f, 0, SEEK_END);
    long int filesize = ftell(target_f);
    fseek(target_f, 0, SEEK_SET);

    //initilise input stream
    InputStream input_stream = {};
    input_stream.contents = new char[filesize];
    input_stream.size = filesize;

    // read file into contents
    fread(input_stream.contents, filesize, 1, target_f);

    // Assume 1 character per token
    std::vector<Token> tokens;
    tokenise_file(&input_stream, &tokens);

    // pull tokens into a stream for interation
    TokenStream token_stream = TokenStream(tokens.size(), tokens);
    token_stream.n_tokens = tokens.size();
    token_stream.tokens = tokens;

    // initilise arena and assume tokens map 1:1 onto ASTnodes
    ASTNodeArena ast_arena = ASTNodeArena(token_stream.n_tokens+1);

    ASTNode file_node = {.token = token_stream.peek(0), .type = ASTNodeType::FILE};
    while (token_stream.peek(0)->type != TokenType::END) {
        ASTNode *child = parse_expression(&token_stream, &ast_arena, false);

        if (child->token->type == TokenType::END) {
            break;
        }

        file_node.n_children++;
    }

    ast_arena.node_alloc(&file_node);

    return 0;
}
