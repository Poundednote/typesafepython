#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <windows.h>

#include <stdio.h>
#include <stdint.h>
#include <process.h>

#include "parser.cpp"
#include "utils.cpp"
#include "tokeniser.cpp"
#include "typing.cpp"
#include "tables.cpp"
#include "debug.cpp"

#if 0
static inline void write_code_and_inc_offset(FILE *file, std::string string,
                                             uint32_t *main_offset)
{
        *main_offset += string.length();
        fprintf_s(file, "%.*s", string.length(), string.c_str());
}

static void generate_code(FILE *file, AstNode *node, uint32_t main_offset,
                          bool top_level)
{
        std::string token_str = token_to_string(node->token);

        switch (node->type) {
        case AstNodeType::FILE: {
                write_code_and_inc_offset(file, "int main(void) {",
                                          &main_offset);
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
                generate_code(file, node->if_stmt.condition, main_offset,
                              false);
                write_code_and_inc_offset(file, ")", &main_offset);
                write_code_and_inc_offset(file, "{", &main_offset);
                generate_code(file, node->if_stmt.block, main_offset,
                              top_level);
                write_code_and_inc_offset(file, "}", &main_offset);
                generate_code(file, node->if_stmt.or_else, main_offset,
                              top_level);
        } break;
        case AstNodeType::ELSE: {
                write_code_and_inc_offset(file, "else {", &main_offset);
                generate_code(file, node->else_stmt.block, main_offset,
                              top_level);
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
#endif

#ifdef DEBUG
static inline uint64_t set_marker()
{
        LARGE_INTEGER marker;
        QueryPerformanceCounter(&marker);

        return marker.QuadPart;
}

static inline double get_time_in_seconds_from_marker(uint64_t marker) {

        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        uint64_t diff = counter.QuadPart - marker;
        double time_elapsed = (double)diff / (double)freq.QuadPart;
        return time_elapsed;
}

#else
static inline uint64_t set_marker()
{
        return 0;
}
static inline double get_time_in_seconds_from_marker(uint64_t marker)
{
        return 0;
}
#endif

static void python_path_overwrite_filepart(PythonPath *path, const char *filename, size_t filename_length)
{
        memset(path->file_part, 0, path->length_from_file_part);
        strncpy_s(path->file_part, path->length_from_file_part, filename,
                  filename_length);
}

static bool name_is_in_import_list(ImportList *list, AstNode *target)
{
        if (!target) {
               return false;
        }

        for (int i = 0; i < list->list_index; ++i) {
                AstNode *node = list->list[i];

                if (!node) {
                        continue;
                }

                if (node->import_target.dotted_name->token.value ==
                    target->import_target.dotted_name->token.value) {
                        return true;

                }
        }

        return false;
}

void parse_and_type_import_files_recursively(
        PythonPath *path, Arena *parse_arena, AstNode **node_in_list,
        Tables *tables, Arena *symbol_table_arena, Arena *scope_stack)
{
        if (!(*node_in_list)) {
                return;
        }

        std::string filename =
                (*node_in_list)->import_target.dotted_name->token.value;

        SymbolTableValue symbol_value = {};
        symbol_value.static_type.type = TypeInfoType::INTEGER;
        symbol_value.node = *node_in_list;
        SymbolTableEntry *scope = tables->symbol_table->insert(
                symbol_table_arena, filename, 0, &symbol_value);

        InputStream input_stream;

        if (filename == "sys") {
                input_stream = InputStream::create_from_file("sysmodule.tpy");
        } else if (filename == "import_test") {
                input_stream = InputStream::create_from_file("import_test.py");
        } else {
                filename += ".py";
                python_path_overwrite_filepart(path, filename.c_str(),
                                               filename.length());
                input_stream = InputStream::create_from_file(path->path_buffer);
        }

        if (!input_stream.contents) {
                perror("Error reading import file");
                exit(1);
        }

        // initilise tokeniser
        TokenArray token_array = token_array_create_from_input_stream(parse_arena, &input_stream);
        Parser parser =  {};
        parser.token_arr = &token_array;
        parser.ast_arena = parse_arena;
        parser.tables = tables;
        parser.symbol_table_arena = symbol_table_arena;
        ParseResult result = parse_statements(&parser);
        AstNode *root = result.node;


        *node_in_list = nullptr;

        for (int i = 0; i < tables->import_list->list_index; ++i) {
                AstNode **node = &tables->import_list->list[i];
                if (name_is_in_import_list(tables->import_list, *node)) {
                        continue;
                }

                parse_and_type_import_files_recursively(path, parse_arena, node,
                                                        tables,
                                                        symbol_table_arena,
                                                        scope_stack);
        }

        scope_stack_push(scope_stack, scope);
        type_parse_tree(root, parse_arena, scope_stack, tables, filename.c_str());
        input_stream.destroy();
}

int main(int argc, char *argv[])
{

        uint64_t start = set_marker();
        if (argc != 2) {
                perror("Please specify a file");
                return EXIT_FAILURE;
        }

        InputStream builtin_input_stream =
                InputStream::create_from_file("builtins.tpy");
        //initilise input stream
        InputStream input_stream = InputStream::create_from_file(argv[1]);
        if (!input_stream.contents) {
                return EXIT_FAILURE;
        }

        PythonPath path = {};
        SearchPathA(0, "python.exe", 0, sizeof(path.path_buffer),
                    path.path_buffer, &path.file_part);

        path.length_from_file_part =
                path.path_buffer + sizeof(path.path_buffer) - path.file_part;

        // write over the pymthon.exe part with zeros
        memset(path.file_part, 0, path.length_from_file_part);
        char lib_dir[] = "Lib\\";
        for (int i = 0;
             i < sizeof(lib_dir) - 1 && i < path.length_from_file_part; ++i) {
                *(path.file_part++) = lib_dir[i];
        }
        // recompute length
        path.length_from_file_part =
                path.path_buffer + sizeof(path.path_buffer) - path.file_part;

        Arena parse_arena = Arena::init(GIGABYTES(8));
        Arena symbol_table_arena = Arena::init(GIGABYTES(2));
        Tables tables = Tables::init(&symbol_table_arena);
        SymbolTableValue main_symbol_value = {};
        main_symbol_value.static_type.type = TypeInfoType::INTEGER;

        // all entries in symbol table require a reference to a node
        AstNode main_node = {};
        main_symbol_value.node = &main_node;

        SymbolTableEntry *main_scope = tables.symbol_table->insert(
                &symbol_table_arena, "main", 0, &main_symbol_value);

        // initilise a stack for scopes when typing
        // parse builtin definitions to pull into symbol table
        TokenArray builtin_token_array = token_array_create_from_input_stream(
                &parse_arena, &builtin_input_stream);

        Parser parser = {};
        parser.token_arr = &builtin_token_array;
        parser.ast_arena = &parse_arena;
        parser.symbol_table_arena = &symbol_table_arena;
        parser.scope = main_scope;
        parser.tables = &tables;

        ParseResult builtin_result = parse_statements(&parser);
        AstNode *builtin_root = builtin_result.node;

        Arena scope_stack = Arena::init(sizeof(void *) * 1000);
        scope_stack_push(&scope_stack, main_scope);
        type_parse_tree(builtin_root, &parse_arena, &scope_stack, &tables,
                        input_stream.filename);

        // ==== BUILTIN TYPES ====
        SymbolTableValue builtin_value = {};
        builtin_value.node = &main_node;
        builtin_value.static_type.type = TypeInfoType::INTEGER;
        tables.symbol_table->insert(&symbol_table_arena, "int", main_scope,
                                    &builtin_value);
        builtin_value.static_type.type = TypeInfoType::FLOAT;
        tables.symbol_table->insert(&symbol_table_arena, "float", main_scope,
                                    &builtin_value);
        builtin_value.static_type.type = TypeInfoType::STRING;
        tables.symbol_table->insert(&symbol_table_arena, "str", main_scope,
                                    &builtin_value);
        builtin_value.static_type.type = TypeInfoType::BOOLEAN;
        tables.symbol_table->insert(&symbol_table_arena, "bool", main_scope,
                                    &builtin_value);
        builtin_value.static_type.type = TypeInfoType::COMPLEX;
        tables.symbol_table->insert(&symbol_table_arena, "complex", main_scope,
                                    &builtin_value);
        builtin_value.static_type.type = TypeInfoType::LIST;
        tables.symbol_table->insert(&symbol_table_arena, "list", main_scope,
                                    &builtin_value);
        builtin_value.static_type.type = TypeInfoType::DICT;
        tables.symbol_table->insert(&symbol_table_arena, "dict", main_scope,
                                    &builtin_value);
        builtin_value.static_type.type = TypeInfoType::NONE;
        tables.symbol_table->insert(&symbol_table_arena, "None", main_scope,
                                    &builtin_value);

        builtin_value.static_type.type = TypeInfoType::ANY;

        uint64_t py_init_mark = set_marker();

#if 1
        Py_Initialize();
        printf("Finished initilising Python API took: %fs\n", get_time_in_seconds_from_marker(py_init_mark));
        uint64_t py_biz_mark = set_marker();
        const char *buffer;
        long long buffer_size = sizeof(buffer);
        PyObject *module_dict = PyImport_GetModuleDict();
        PyObject *module_dict_str = PyObject_Str(module_dict);
        PyObject *module_dict_ascii = PyUnicode_AsASCIIString(module_dict);
        PyObject *builtins_dict = PyEval_GetBuiltins();
        PyObject *builtins_list = PyDict_Items(builtins_dict);

        printf("Finished biz in Python API took: %fs\n", get_time_in_seconds_from_marker(py_biz_mark));
        if (!module_dict) {
                printf("nah");
                exit(1);
        }

        for (int i = 0; i < PyList_Size(builtins_list); ++i) {
                PyObject *builtin_item = PyList_GetItem(builtins_list, i);
                PyObject *builtin_name = PyUnicode_AsASCIIString(
                        PyTuple_GetItem(builtin_item, 0));

                PyObject *builtin_object = PyTuple_GetItem(builtin_item, 1);
                PyTypeObject *object_type = Py_TYPE(builtin_object);
                PyObject *object_py_string = PyType_GetName(object_type);
                PyObject *object_py_bytes = PyUnicode_AsASCIIString(object_py_string);

                char *type_string = PyBytes_AsString(object_py_bytes);

                SymbolTableValue value = {};
                TypeInfo type = {};

                if (PyObject_TypeCheck(builtin_object, &PyLong_Type)) {
                        value.static_type.type = TypeInfoType::INTEGER;
                } else if (PyObject_TypeCheck(builtin_object, &PyFloat_Type)) {
                        value.static_type.type = TypeInfoType::FLOAT;
                } else if (PyObject_TypeCheck(builtin_object, &PyUnicode_Type)) {
                        value.static_type.type = TypeInfoType::STRING;
                } else if (PyObject_TypeCheck(builtin_object, &PyBool_Type)) {
                        value.static_type.type = TypeInfoType::BOOLEAN;
                } else if (PyObject_TypeCheck(builtin_object, &PyComplex_Type)) {
                        value.static_type.type = TypeInfoType::COMPLEX;
                } else if (PyObject_TypeCheck(builtin_object, &PyList_Type)) {
                        value.static_type.type = TypeInfoType::LIST;
                } else if (PyObject_TypeCheck(builtin_object, &PyDict_Type)) {
                        value.static_type.type = TypeInfoType::DICT;
                } else if (PyObject_TypeCheck(builtin_object, &PyType_Type)) {
                        value.static_type.type = TypeInfoType::CLASS;
                } else if (PyObject_TypeCheck(builtin_object, &PyCFunction_Type)) {
                        value.static_type.type = TypeInfoType::CLASS;
                }



                Py_DecRef(object_py_string);
                Py_DecRef(object_py_bytes);


        }


        //CLEANUP

        Py_DecRef(module_dict_str);
        Py_DecRef(module_dict_ascii);
        Py_DecRef(builtins_list);
        Py_DecRef(builtins_list);
#endif
        uint64_t parser_mark = set_marker();
        TokenArray token_array = token_array_create_from_input_stream(
                &parse_arena, &input_stream);
        parser.token_arr = &token_array;

        ParseResult result = parse_statements(&parser);
        AstNode *root = result.node;

        for (int i = 0; i < tables.import_list->list_index; ++i) {
                AstNode **node = &tables.import_list->list[i];
                parse_and_type_import_files_recursively(&path, &parse_arena,
                                                        node, &tables,
                                                        &symbol_table_arena,
                                                        &scope_stack);
        }

        printf("Finished Parsing, time elasped: %fs\n",
               get_time_in_seconds_from_marker(parser_mark));


        debug_print_parse_tree(root, 0);

        //type
        uint64_t type_checking_mark = set_marker();
        type_parse_tree(root, &parse_arena, &scope_stack, &tables,
                        input_stream.filename);

        printf("Finished Type checking, time elasped: %fs\n",
               get_time_in_seconds_from_marker(type_checking_mark));

        debug_print_parse_tree(root, 0);

        //FILE *output_f;
        //fopen_s(&output_f, "out.c", "w");
        //generate_code(output_f, root, 0, true);
        //fprintf_s(output_f, "return 0;}");
        //fclose(output_f);

        printf("Finished Parsing & Typechecking %d lines time elasped: %fs",
               input_stream.line, get_time_in_seconds_from_marker(start));
        // free
        symbol_table_arena.destroy();
        parse_arena.destroy();

        return 0;
}
