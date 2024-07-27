#include <stdio.h>
#include <stdint.h>

#include "parser.cpp"
#include "tokeniser.cpp"
#include "typing.cpp"

static inline void write_code_and_inc_offset(FILE *file, std::string string, uint32_t *main_offset) {
   *main_offset += string.length();
   fprintf_s(file, "%.*s", string.length(), string.c_str());
}

static void generate_code(FILE *file, AstNode *node, uint32_t main_offset, bool top_level) {
  std::string token_str = token_to_string(node->token);

  switch (node->type) {
  case AstNodeType::FILE: {
    write_code_and_inc_offset(file, "int main(void) {", &main_offset);
    AstNode *child = node->file.children;

    while (child) {
      generate_code(file, child, top_level, 16);
      child = child->adjacent_child;
    }

  } break;

  case AstNodeType::BINARYEXPR: {
    generate_code(file, node->binary.left, main_offset, top_level);
    write_code_and_inc_offset(file, token_str, &main_offset);
    generate_code(file, node->binary.right, main_offset, top_level);
    if (top_level) {
        write_code_and_inc_offset(file, ";", &main_offset);
    }

  } break;

  case AstNodeType::TERMINAL: {
    write_code_and_inc_offset(file, token_str, &main_offset);

    return;
  } break;

  case AstNodeType::UNARY: {
  } break;
  case AstNodeType::NARY: {
  } break;
  case AstNodeType::ASSIGNMENT: {
  } break;
  case AstNodeType::BLOCK: {
      AstNode *child = node->block.children;
      while (child) {
          generate_code(file, child, main_offset, top_level);
          child = child->adjacent_child;
      }
  } break;
  case AstNodeType::DECLARATION: {
  } break;
  case AstNodeType::IF: {
      write_code_and_inc_offset(file, "if(", &main_offset);
      generate_code(file, node->if_stmt.condition, main_offset, false);
      write_code_and_inc_offset(file, ")", &main_offset);
      write_code_and_inc_offset(file, "{", &main_offset);
      generate_code(file, node->if_stmt.block, main_offset, top_level);
      write_code_and_inc_offset(file, "}", &main_offset);
      generate_code(file, node->if_stmt.or_else, main_offset, top_level);
  } break;
  case AstNodeType::ELSE: {
      write_code_and_inc_offset(file, "else {", &main_offset);
      generate_code(file, node->else_stmt.block, main_offset, top_level);
      write_code_and_inc_offset(file, "}", &main_offset);
  } break;
  case AstNodeType::WHILE: {
  } break;
  case AstNodeType::FOR_LOOP: {
  } break;
  case AstNodeType::FOR_IF: {
  } break;
  case AstNodeType::FUNCTION_DEF: {
  } break;
  case AstNodeType::CLASS_DEF: {
  } break;
  case AstNodeType::FUNCTION_CALL: {
  } break;
  case AstNodeType::SUBSCRIPT: {
  } break;
  case AstNodeType::TRY: {
  } break;
  case AstNodeType::EXCEPT: {
  } break;
  case AstNodeType::STARRED: {
  } break;
  case AstNodeType::KVPAIR: {
  } break;
  case AstNodeType::IMPORT: {
  } break;
  }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        perror("Please specify a file");
        return EXIT_FAILURE;
    }

 //initilise input stream
    InputStream input_stream = InputStream::create_from_file(argv[1]);
    if (!input_stream.contents) {
        return EXIT_FAILURE;
    }

    // initilise tokeniser
    Tokeniser tokeniser = Tokeniser::init(&input_stream);

    Arena parse_arena = Arena::init(GIGABYTES(8));
    Arena symbol_table_arena = Arena::init(GIGABYTES(1));
    CompilerTables tables = CompilerTables::init(&symbol_table_arena);

    SymbolTableValue main_symbol_value = {};
    main_symbol_value.static_type.type = TypeInfoType::INTEGER;

    std::string main_identifier = "main";
    SymbolTableEntry *main_scope = tables.function_table->insert(
        &symbol_table_arena, main_identifier, 0, main_symbol_value);

    AstNode *root = parse_statements(&tokeniser, &parse_arena, main_scope, &tables, &symbol_table_arena);

    Arena scope_stack_sub_arena = {};
    scope_stack_sub_arena.memory = parse_arena.alloc(sizeof(void *)*1000);
    scope_stack_sub_arena.capacity = PAGE_SIZE;

    scope_stack_push(&scope_stack_sub_arena, main_scope);
    type_parse_tree(root, &scope_stack_sub_arena, &tables);
    //Freeing the scope_stack sub arena
    parse_arena.offset -= scope_stack_sub_arena.capacity;


    printf("\n");
    debug_print_parse_tree(root, 0);

    FILE *output_f;
    fopen_s(&output_f, "out.c", "w");
    generate_code(output_f, root, 0, true);
    fprintf_s(output_f, "return 0;}");
    fclose(output_f);

    printf("Finished Parsing %d lines", input_stream.line);

    parse_arena.destroy();
    input_stream.destroy(); // free
    return 0;
}
