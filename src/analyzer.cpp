#include <filesystem>
#include <chrono>

#include "andy/console.hpp"
#include "andy/file.hpp"
#include "andy/binary.hpp"

#include "andy/lang/parser.hpp"
#include "andy/lang/lexer.hpp"
#include "andy/lang/interpreter.hpp"
#include "andy/lang/extension.hpp"
#include "andy/lang/preprocessor.hpp"

std::string buffer;
size_t num_linter_warnings = 0;
extern void create_builtin_libs();
void write_path(std::string_view path)
{
#ifdef __UVA_WIN__
    for(const auto& c : path) {
        buffer.push_back(c);
        if(c == '\\') {
            buffer.push_back('\\');
            buffer.push_back('\\');
        }
    }
#else
    buffer += path;
#endif
}

void write_linter_warning(std::string_view type, std::string_view message, std::string_view file_name, andy::lang::lexer::token_position start, size_t length)
{
    if(num_linter_warnings) {
        buffer += ",\n";
    }
    buffer += "\t\t{\n\t\t\t\"type\": \"";
    buffer += type;
    buffer += "\",\n\t\t\t\"message\": \"";
    buffer += message;
    buffer += "\",\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
    buffer += file_name;
    buffer += "\",\n\t\t\t\t\"line\": ";
    buffer += std::to_string(start.line);
    buffer += ",\n\t\t\t\t\"column\": ";
    buffer += std::to_string(start.column);
    buffer += ",\n\t\t\t\t\"offset\": ";
    buffer += std::to_string(start.offset);
    buffer += ",\n\t\t\t\t\"length\": ";
    buffer += std::to_string(length);
    buffer += "\n\t\t\t}\n\t\t}";
    ++num_linter_warnings;
}


void print_help(std::string_view program_name) {
    std::cout << "Usage:\n"
                << "  " << program_name << " <path> [--stdin | --temp <temp-path>]\n\n"
                << "Arguments:\n"
                << "  <path>                 Logical file path. Required.\n"
                << "                         Used for resolving relative #includes and identifying the source logically.\n\n"
                << "Options:\n"
                << "  --stdin                Read the file contents from standard input (stdin),\n"
                << "                         but treat the content as if it came from <path>.\n\n"
                << "  --temp <temp-path>     Read file contents from the specified temporary file,\n"
                << "                         but treat it as if it was read from <path>.\n\n"
                << "Description:\n"
                << "  The <path> argument is mandatory and always defines the logical origin of the file.\n"
                << "  This affects relative #include resolution and error messages.\n\n"
                << "  You must use only one of the following input methods:\n"
                << "    - Read content directly from <path> (default if no option is used).\n"
                << "    - Read content from stdin using --stdin.\n"
                << "    - Read content from a temporary file using --temp <temp-path>.\n\n"
                << "Examples:\n"
                << "  " << program_name << " main.andy\n"
                << "      Reads content from main.andy and uses it as the logical path.\n\n"
                << "  " << program_name << " main.andy --stdin\n"
                << "      Reads content from standard input, but treats it as if it came from main.andy.\n\n"
                << "  " << program_name << " main.andy --temp temp_file.andy\n"
                << "      Reads content from temp_file.andy, but treats it as if it came from main.andy.\n";
}

int main(int argc, char** argv) {
    struct switch_option {
        bool enabled;
        bool need_argument;
        std::string_view argument;

        switch_option& operator=(bool value) {
            enabled = value;
            return *this;
        }

        operator bool() const {
            return enabled;
        }
    };

    switch_option read_from_stdin { false, false };
    switch_option read_from_temp { false, true };
    switch_option is_server { false, false };

    std::string tokenization_error;
    std::string parse_error;

    std::map<std::string_view, switch_option*> options = {
        { "--stdin",  &read_from_stdin },
        { "--temp",   &read_from_temp },
        { "--server", &is_server }
    };

    if(argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    if(strncmp(argv[1], "--server", 8) == 0) {
        if(argc > 2) {
            std::cout << "andy-analyzer --server takes no arguments. Write <input-file>\\n<temp-file-path>\\n to stdin" << std::endl;
            return 1;
        }
        is_server = true;
    }

    for(int i = 2; i < argc; i++) {
        std::string_view s = argv[i];

        auto it = options.find(s);

        if(it != options.end()) {
            it->second->enabled = true;
            if(!it->second->need_argument) {
                if(argc > i + 1) {
                    std::string_view next_arg = argv[i + 1];
                    if(!next_arg.starts_with("--")) {
                        throw std::runtime_error("andy-analyzer: option '" + std::string(s) + "' does not take an argument");
                    }
                }
                continue;
            }
            if(argc > i + 1) {
                it->second->argument = argv[i + 1];
                ++i;
            } else {
                std::cout << "andy-analyzer: missing argument for '" << s << "'" << std::endl;
                return 1;
            }

        } else if(i != 1) {
            throw std::runtime_error("andy-analyzer: unknown option '" + std::string(s) + "'");
        }
    }

    bool run = true;

    buffer.reserve(10 * 1024);

    std::string source;

    while(run) {
        buffer.clear();
        num_linter_warnings = 0;

        std::filesystem::path file_path;
        std::filesystem::path file_directory;

        if(is_server) {
            std::string temp;
            std::getline(std::cin, temp);
            file_path = std::filesystem::absolute(temp);
            std::getline(std::cin, temp);
            source = andy::file::read_all_text<char>(temp);
        } else {
            run = false;
            file_path = std::filesystem::absolute(argv[1]);
            if(read_from_stdin) {
                // Read the file len (hexadecimal 32 bits) from stdin
                while(true) {
                    char c = fgetc(stdin);
                    if(c == EOF) {
                        break;
                    }
                    source.push_back(c);
                }
            } else if(read_from_temp) {
                std::filesystem::path temp_file_path = std::filesystem::absolute(read_from_temp.argument);
                source = andy::file::read_all_text<char>(temp_file_path);
            } else {
                source = andy::file::read_all_text<char>(file_path);
            }
        }

        file_directory = file_path.parent_path();

        auto start = std::chrono::high_resolution_clock::now();

        struct declaration {
            std::string_view name;
            std::string_view type;
            std::string_view file;
            size_t line;
            size_t column;
            size_t offset;
        };

        struct error {
            std::string_view type;
            std::string message;
            std::string_view file;
            size_t line;
            size_t column;
            size_t offset;
            size_t length;
        };

        using reference = declaration;
        using linter_warning = error;

        std::vector<declaration> declarations;
        std::vector<reference> references;
        std::vector<error> errors;
        std::vector<linter_warning> linter_warnings;

        declarations.reserve(16);
        references.reserve(16);
        errors.reserve(16);
        linter_warnings.reserve(16);

        if(!std::filesystem::exists(file_path)) {
            std::cerr << "input file '" << file_path.string() << "' does not exist" << std::endl;
            exit(1);
        }

        if(!std::filesystem::is_regular_file(file_path)) {
            std::cerr << "input file '" << file_path.string() << "' is not a regular file" << std::endl;
            exit(1);
        }

        andy::lang::interpreter interpreter;
        create_builtin_libs();
        andy::lang::lexer l;
        std::string file_path_str = file_path.string();

        try {
            l.tokenize(file_path_str, source);
        } catch (const std::exception& e) {
            (void)e;
        }
        struct analyzer_token {
            std::string_view type;
            const andy::lang::lexer::token* token;
            std::string_view modifier;
        };
        std::vector<analyzer_token> tokens_to_write;

        // Preprocessor must be processed before the preprocessor run
        for(const auto& token : l.tokens()) {
            switch(token.type()) {    
                case andy::lang::lexer::token_type::token_preprocessor:
                    tokens_to_write.push_back({ "preprocessor", &token });
                break;
            }
        }
        
        andy::lang::preprocessor preprocessor;
        preprocessor.process(file_path, l);

        buffer += "{\n";

        andy::lang::parser p;
        andy::lang::parser::ast_node root_node;
        
        try {
            root_node = p.parse_all(l);
        } catch (const std::exception& e) {
            (void)e;
        }

        std::vector<analyzer_token> tokens_declarations;
        size_t i = 0;
        std::function<void(const andy::lang::parser::ast_node& node)> inspect_node_for_errors;
        inspect_node_for_errors = [&](const andy::lang::parser::ast_node& node) {
            if(node.type() == andy::lang::parser::ast_node_type::ast_node_fn_call) {
                const andy::lang::parser::ast_node* fn_declname_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                const andy::lang::lexer::token& fn_declname_token = fn_declname_node->token();
                std::string_view fn_declname = fn_declname_token.content();
                
                if(fn_declname != "import") {
                    return;
                }

                auto* params_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params);

                if(!params_node || params_node->childrens().size() != 1) {
                    return;
                }

                auto* import_node = params_node->childrens().data();

                if(import_node->type() != andy::lang::parser::ast_node_type::ast_node_valuedecl) {
                    return;
                }

                const andy::lang::lexer::token& import_declname_token = import_node->token();
                std::string_view import_declname = import_declname_token.content();

                if(andy::lang::extension::exists(file_directory, import_declname)) {
                    andy::lang::extension::import(&interpreter, import_declname);
                    return;
                }

                errors.push_back(error{
                    "missing-import",
                    "Cannot find import " + std::string(import_declname),
                    import_declname_token.m_file_name,
                    import_declname_token.start.line,
                    import_declname_token.start.column,
                    import_declname_token.start.offset,
                    import_declname_token.end.offset - import_declname_token.start.offset
                });
            }
        };
        std::function<void(const andy::lang::parser::ast_node& node)> recursive_inspect_node;
        std::function<void(const andy::lang::parser::ast_node& node)> switch_type;
        recursive_inspect_node = [&](const andy::lang::parser::ast_node& node) {
            switch_type = [&](const andy::lang::parser::ast_node& child) {
                switch(child.type()) {
                    case andy::lang::parser::ast_node_type::ast_node_classdecl: {
                        interpreter.load(interpreter.execute_classdecl(child));
                        auto dectype_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_decltype);
                        auto* declname_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                        auto* declname_token = &declname_node->token();
                        tokens_declarations.push_back({ "class", declname_token, "declaration" });
                        auto* base_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_classdecl_base);
                        if(base_node) {
                            auto* dectype_node = base_node->child_from_type(andy::lang::parser::ast_node_type::ast_node_decltype);
                            if(dectype_node) {
                                auto* base_declname_node = base_node->child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                                if(base_declname_node) {
                                    auto* base_declname_token = &base_declname_node->token();
                                    tokens_to_write.push_back({ "keyword", &dectype_node->token() });
                                }
                            }
                        }
                    }
                    break;
                    case andy::lang::parser::ast_node_type::ast_node_fn_decl: {
                        auto* declname_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                        auto* declname_token = &declname_node->token();
                        tokens_declarations.push_back({ "function", declname_token });
                    }
                    break;
                    case andy::lang::parser::ast_node_type::ast_node_fn_call: {
                        auto* declname_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                        auto* declname_token = &declname_node->token();
                        auto* params = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params);
                        if(params) {
                            for(const auto& param : params->childrens())
                            {
                                if(param.type() == andy::lang::parser::ast_node_type::ast_node_pair) {
                                    switch_type(param);
                                }
                            }
                        }
                        tokens_to_write.push_back({ "function", declname_token });
                    }
                    break;
                    case andy::lang::parser::ast_node_type::ast_node_vardecl: {
                        auto* declname_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                        auto* declname_token = &declname_node->token();
                        auto& value_node = child.childrens()[2];
                        switch_type(value_node);
                        tokens_declarations.push_back({ "variable", declname_token });
                    }
                    break;
                    case andy::lang::parser::ast_node_type::ast_node_declname: {
                        if(auto object_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object)) {
                            auto* declname_token = &child.token();
                            tokens_to_write.push_back({ "function", declname_token });
                        }
                    }
                    break;
                    case andy::lang::parser::ast_node_type::ast_node_for:
                    case andy::lang::parser::ast_node_type::ast_node_while:
                    case andy::lang::parser::ast_node_type::ast_node_foreach: {
                        auto* vardecl_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_vardecl);
                        if(vardecl_node) {
                            // Foreach only
                            auto* declname_node = vardecl_node->child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                            if(declname_node) {
                                auto* declname_token = &declname_node->token();
                                tokens_declarations.push_back({ "variable", declname_token });
                                
                                auto& tokens = l.tokens();
                                auto& next_token = tokens[declname_token->index + 1];
                                if(next_token.type() == andy::lang::lexer::token_type::token_identifier &&
                                   next_token.content() == "in") {
                                    tokens_to_write.push_back({ "keyword", &next_token });
                                }
                            }
                        }
                        auto decltype_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_decltype);
                        if(decltype_node) {
                            tokens_to_write.push_back({ "keyword", &decltype_node->token() });
                        }
                    }
                    break;
                    case andy::lang::parser::ast_node_type::ast_node_pair: {
                        auto* key_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                        if(key_node) {
                            auto* key_token = &key_node->token();
                            tokens_to_write.push_back({ "variable", key_token });
                        }
                    }
                    break;
                }
                if(auto context = child.context()) {
                    recursive_inspect_node(*context);
                }
            };
            for(auto const & child : node.childrens()) {
                switch_type(child);
                inspect_node_for_errors(child);
            }
        };

        recursive_inspect_node(root_node);

        for(const auto& token : tokens_declarations) {
            tokens_to_write.push_back({ token.type, token.token, "declaration" });
        }

        for(size_t i = 0; i < l.tokens().size(); ++i) {
            const auto& token = l.tokens()[i];
            switch(token.type()) {
                case andy::lang::lexer::token_type::token_comment:
                    tokens_to_write.push_back({ "comment", &token });
                break;
                case andy::lang::lexer::token_type::token_keyword:
                    tokens_to_write.push_back({ "keyword", &token });
                break;
                case andy::lang::lexer::token_type::token_identifier: {
                    if(token.content() == "super") {
                        tokens_to_write.push_back({ "keyword", &token });
                    }
                    auto it = std::find_if(tokens_declarations.begin(), tokens_declarations.end(),
                        [&token](const analyzer_token& t) { return t.token->content() == token.content(); });
                    if(it != tokens_declarations.end()) {
                        tokens_to_write.push_back({ it->type, &token });
                    }
                }
                break;
                case andy::lang::lexer::token_type::token_literal:
                    switch(token.kind()) {
                        case andy::lang::lexer::token_kind::token_integer:
                        case andy::lang::lexer::token_kind::token_float:
                        case andy::lang::lexer::token_kind::token_double:
                            tokens_to_write.push_back({ "number", &token });
                        break;
                        case andy::lang::lexer::token_kind::token_string:
                            tokens_to_write.push_back({ "string", &token });
                        break;
                        case andy::lang::lexer::token_kind::token_boolean:
                            tokens_to_write.push_back({ "keyword", &token });
                        break;
                    }
                break;
                case andy::lang::lexer::token_type::token_delimiter:
                    if(token.content() == "end") {
                        tokens_to_write.push_back({ "keyword", &token });
                    }
                break;
            }
        }

        buffer += "\t\"tokens\": [\n";
        for(auto token : tokens_to_write) {
            if(i) {
                buffer += ",\n";
            }
            ++i;
            buffer += "\t\t{\n\t\t\t\"type\": \"";
            buffer += token.type;
            buffer += "\",\n\t\t\t\"modifier\": \"";
            buffer += token.modifier;
            buffer += "\",\n\t\t\t\"content\": \"";
            buffer += token.token->content();
            buffer += "\",\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
            write_path(token.token->m_file_name);
            buffer += "\",\n\t\t\t\t\"start\": {\n\t\t\t\t\t\"line\": ";
            buffer += std::to_string(token.token->start.line);
            buffer += ",\n\t\t\t\t\t\"column\": ";
            buffer += std::to_string(token.token->start.column);
            buffer += ",\n\t\t\t\t\t\"offset\": ";
            buffer += std::to_string(token.token->start.offset);
            buffer += "\n\t\t\t\t},\n\t\t\t\t\"end\": {\n\t\t\t\t\t\"line\": ";
            buffer += std::to_string(token.token->end.line);
            buffer += ",\n\t\t\t\t\t\"column\": ";
            buffer += std::to_string(token.token->end.column);
            buffer += ",\n\t\t\t\t\t\"offset\": ";
            buffer += std::to_string(token.token->end.offset);
            buffer += "\n\t\t\t\t}\n\t\t\t}\n\t\t}";
        }

        buffer += "\n\t],\n";

        // size_t declaration_it = 0;
        // std::vector<std::string_view> class_names;
        // std::vector<std::string_view> variable_names;
        // std::vector<std::string_view> function_names;

        // std::function<void(const andy::lang::parser::ast_node& node)> inspect_node_for_declarations;
        // inspect_node_for_declarations = [&](const andy::lang::parser::ast_node& node) {
        //     switch(node.type()) {
        //         case andy::lang::parser::ast_node_type::ast_node_classdecl:
        //         case andy::lang::parser::ast_node_type::ast_node_fn_decl:
        //         case andy::lang::parser::ast_node_type::ast_node_vardecl: {
        //             std::string_view decname = node.decname();
        //             std::string_view decl_type = node.decl_type();
        //             const andy::lang::lexer::token& decname_token = *node.child_token_from_type(andy::lang::parser::ast_node_type::ast_node_declname);

        //             declarations.push_back(declaration{
        //                 decname_token.content(),
        //                 decl_type,
        //                 decname_token.m_file_name,
        //                 decname_token.start.line,
        //                 decname_token.start.column,
        //                 decname_token.start.offset
        //             });

        //             if(auto context = node.context()) {
        //                 for(const auto& child : context->childrens()) {
        //                     inspect_node_for_declarations(child);
        //                 }
        //             }
        //             break;
        //         }
        //     }
        // };

  

        // std::function<void(const andy::lang::parser::ast_node& node)> inspect_node_for_references;
        // inspect_node_for_references = [&](const andy::lang::parser::ast_node& node) {
        //     std::vector<declaration> current_declarations;
        //     if(node.type() == andy::lang::parser::ast_node_type::ast_node_fn_call) {
        //         const andy::lang::parser::ast_node* fn_declname_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
        //         const andy::lang::lexer::token& fn_declname_token = fn_declname_node->token();
        //         std::string_view fn_declname = fn_declname_token.content();

        //         auto it = std::find_if(declarations.begin(), declarations.end(), [&](const declaration& d) {
        //             return d.name == fn_declname;
        //         });

        //         if(it != declarations.end()) {
        //             references.push_back(reference{
        //                 it->name,
        //                 it->type,
        //                 it->file,
        //                 it->line,
        //                 it->column,
        //                 it->offset
        //             });
        //         }
        //     }
        // };

        // // First we have to find all the declarations
        // for(const auto& node : root_node.childrens()) {
        //     inspect_node_for_declarations(node);

        //     // Also uses the loop to find erros
        //     inspect_node_for_errors(node);

        //     // And references
        //     inspect_node_for_references(node);
        // }

        // // Token level linting/references
        // for(size_t i = 0; i < l.tokens().size(); i++) {
        //     const auto& token = l.tokens()[i];
            //std::string_view content = token.content();

            // if(token.type() == andy::lang::lexer::token_type::token_identifier) {
            //     if(auto it = std::find_if(declarations.begin(), declarations.end(), [&](const declaration& d) {
            //         return d.name == content;
            //     }); it != declarations.end()) {
            //         references.push_back({
            //             it->name,
            //             it->type,
            //             token.m_file_name,
            //             token.start.line,
            //             token.start.column,
            //             token.start.offset
            //         });
            //     } else if(auto it = std::find_if(interpreter.classes.begin(), interpreter.classes.end(), [&](const std::shared_ptr<andy::lang::structure>& cls) {
            //         return cls->name == content;
            //     }); it != interpreter.classes.end()) {
            //         references.push_back({
            //             (*it)->name,
            //             "class",
            //             token.m_file_name,
            //             token.start.line,
            //             token.start.column,
            //             token.start.offset
            //         });
            //     }
            // }

            // size_t offset = token.start.offset;
            // size_t end_offset = token.end.offset;
            // std::string_view source = l.source(token);
            // int has_whitespace = 0;
            // const char* end_it = source.data() + end_offset;

            // Check if the token is before a \n
            // while(end_it != source.data()) {
            //     if(*end_it == '\n' || *end_it == 0) {
            //         break;
            //     } else if(isspace(*end_it)) {
            //         has_whitespace++;
            //     } else {
            //         break;
            //     }
            //     end_it++;
            // }

            // if(has_whitespace && (*end_it == '\n' || *end_it == 0)) {
            //     write_linter_warning("trailing-whitespace", "Trailing whitespace", token.m_file_name, token.end, has_whitespace);
            // }

        //     switch(token.type()) {
        //         case andy::lang::lexer::token_type::token_literal:
        //             switch(token.kind()) {
        //                 case andy::lang::lexer::token_kind::token_string:
        //                     char c = source[offset];

        //                     switch(c)
        //                     {
        //                         case '\"':
        //                             if(token.content().find("${") == std::string::npos) {
        //                                 write_linter_warning("string-default-single-quotes", "String literal without interpolation should use single quotes", token.m_file_name, token.start, token.content().size() + 2 /* 2 for the quotes */);
        //                             }
        //                         break;
        //                     }
        //                 break;
        //             }
        //         break;
        //     }
        // }

        // buffer += "\t\"declarations\": [";

        // for(size_t i = 0; i < declarations.size(); i++) {
        //     const auto& dec = declarations[i];
        //     if(i) {
        //         buffer += ",";
        //     }
        //     buffer += "\n\t\t{\n\t\t\t\"type\": ";
        //     buffer += "\"";
        //     buffer += dec.type;
        //     buffer += "\",\n\t\t\t\"name\": \"";
        //     buffer += dec.name;
        //     buffer += "\",\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
        //     write_path(dec.file);
        //     buffer += "\",\n";
        //     buffer += "\t\t\t\t\"line\": ";
        //     buffer += std::to_string(dec.line);
        //     buffer += ",\n\t\t\t\t\"column\": ";
        //     buffer += std::to_string(dec.column);
        //     buffer += ",\n\t\t\t\t\"offset\": ";
        //     buffer += std::to_string(dec.offset);
        //     buffer += "\n\t\t\t}";
        //     buffer += "\n\t\t}";
        // }
        
        // buffer += "\n\t],\n\t\"references\": [";

        // for(size_t i = 0; i < references.size(); i++) {
        //     const auto& reference = references[i];
        //     if(i) {
        //         buffer += ",";
        //     }

        //     buffer += "\n\t\t{\n\t\t\t\"type\": \"";
        //     buffer += reference.type;
        //     buffer += "\",\n\t\t\t\"name\": \"";
        //     buffer += reference.name;
        //     buffer += "\",\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
        //     write_path(reference.file);
        //     buffer += "\",\n\t\t\t\t\"line\": ";
        //     buffer += std::to_string(reference.line);
        //     buffer += ",\n\t\t\t\t\"column\": ";
        //     buffer += std::to_string(reference.column);
        //     buffer += ",\n\t\t\t\t\"offset\": ";
        //     buffer += std::to_string(reference.offset);
        //     buffer += "\n\t\t\t}";
        //     buffer += "\n\t\t}";
        // }

        // buffer +="\n\t],\n";

        buffer += "\t\"errors\": [\n";

        for(size_t i = 0; i < errors.size(); i++) {
            const auto& error = errors[i];
            buffer += "\t\t{\n\t\t\t\"message\": \"";
            buffer += error.message;
            buffer += "'\",";
            buffer += "\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
            write_path(error.file);
            buffer += "\",\n";
            buffer += "\t\t\t\t\"line\": ";
            buffer += std::to_string(error.line);
            buffer += ",\n\t\t\t\t\"column\": ";
            buffer += std::to_string(error.column);
            buffer += ",\n\t\t\t\t\"offset\": ";
            buffer += std::to_string(error.offset);
            buffer += ",\n\t\t\t\t\"length\": ";
            buffer += std::to_string(error.length);
            buffer += "\n\t\t\t}";
            buffer += "\n\t\t}";
        }

        buffer += "\n\t],\n";

        // for(size_t i = 0; i < linter_warnings.size(); i++) {
        //     const auto& warning = linter_warnings[i];
        //     buffer += "\t\t{\n\t\t\t\"type\": \"";
        //     buffer += warning.type;
        //     buffer += "\",\n\t\t\t\"message\": \"";
        //     buffer += warning.message;
        //     buffer += "\",\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
        //     write_path(warning.file);
        //     buffer += "\",\n";
        //     buffer += "\t\t\t\t\"line\": ";
        //     buffer += std::to_string(warning.line);
        //     buffer += ",\n\t\t\t\t\"column\": ";
        //     buffer += std::to_string(warning.column);
        //     buffer += ",\n\t\t\t\t\"offset\": ";
        //     buffer += std::to_string(warning.offset);
        //     buffer += ",\n\t\t\t\t\"length\": ";
        //     buffer += std::to_string(warning.length);
        //     buffer += "\n\t\t\t}";
        //     buffer += "\n\t\t}";
        // }

        // buffer += "\n\t],\n";

        // buffer += "\t\"linter\": [\n";

        // for(size_t i = 0; i < linter_warnings.size(); i++) {
        //     const auto& warning = linter_warnings[i];
        //     buffer += "\t\t{\n\t\t\t\"type\": \"";
        //     buffer += warning.type;
        //     buffer += "\",\n\t\t\t\"message\": \"";
        //     buffer += warning.message;
        //     buffer += "\",\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
        //     write_path(warning.file);
        //     buffer += "\",\n";
        //     buffer += "\t\t\t\t\"line\": ";
        //     buffer += std::to_string(warning.line);
        //     buffer += ",\n\t\t\t\t\"column\": ";
        //     buffer += std::to_string(warning.column);
        //     buffer += ",\n\t\t\t\t\"offset\": ";
        //     buffer += std::to_string(warning.offset);
        //     buffer += ",\n\t\t\t\t\"length\": ";
        //     buffer += std::to_string(warning.length);
        //     buffer += "\n\t\t\t}";
        //     buffer += "\n\t\t}";
        // }

        // buffer += "\n\t],\n";

        auto end = std::chrono::high_resolution_clock::now();

        buffer += "\t\"elapsed\": \"";
        buffer += std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        buffer += "ms\"\n";

        buffer += "}";

        //if(is_server) {
            // uint32_t size = (uint32_t)buffer.size();
            // std::string size_str = andy::binary::to_hex_string(size);

            // std::cout << size_str;
            std::cout << buffer;
        //} else {
            //std::cout << buffer << std::endl;
        //}
    }

    return 0;
}