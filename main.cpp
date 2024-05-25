#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <sys/stat.h>

#include "parser.cpp"
#include "tokeniser.cpp"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Please specify a file" << std::endl;
        return EXIT_FAILURE;
    }

    //initilise input stream
    InputStream input_stream = InputStream::create_from_file(argv[1]);
    if (!input_stream.contents) {
        return EXIT_FAILURE;
    }

    // Assume 1 character per token
    std::vector<Token> tokens = {};
    tokens.reserve(input_stream.size);
    tokenise_file(&input_stream, &tokens);

    // pull tokens into a stream for iteration
    TokenStream token_stream = TokenStream(tokens);

    // initilise arena and assume tokens map 1:1 onto ASTnodes
    ASTNodeArena ast_arena = ASTNodeArena(token_stream.tokens.size()+1);


    ASTNode *root = parse_statements(&token_stream, &ast_arena, false);
    type_parse_tree(root);
    std::cout << "FINISHED PARSING " << input_stream.line << " lines";
    //debug_print_parse_tree(root, "    ");
    return 0;
}

