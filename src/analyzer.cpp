#include <filesystem>
#include <chrono>

#include <uva/console.hpp>
#include <uva/file.hpp>

#include <andy/lang/parser.hpp>
#include <andy/lang/lexer.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/extension.hpp>
#include <andy/lang/preprocessor.hpp>

size_t num_linter_warnings = 0;

void write_path(std::string_view path)
{
#ifdef __UVA_WIN__
    for(const auto& c : path) {
        std::cout << c;
        if(c == '\\') {
            std::cout << "\\";
        }
    }
#else
    std::cout << path;
#endif
}

void write_linter_warning(std::string_view type, std::string_view message, std::string_view file_name, andy::lang::lexer::token_position start, size_t length)
{
    if(num_linter_warnings) {
        std::cout << ",\n";
    }
    std::cout << "\t\t{\n\t\t\t\"type\": \"";
    std::cout << type;
    std::cout << "\",\n\t\t\t\"message\": \"";
    std::cout << message;
    std::cout << "\",\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
    std::cout << file_name;
    std::cout << "\",\n\t\t\t\t\"line\": ";
    std::cout << start.line;
    std::cout << ",\n\t\t\t\t\"column\": ";
    std::cout << start.column;
    std::cout << ",\n\t\t\t\t\"offset\": ";
    std::cout << start.offset;
    std::cout << ",\n\t\t\t\t\"length\": ";
    std::cout << length;
    std::cout << "\n\t\t\t}\n\t\t}";
    ++num_linter_warnings;
}

int main(int argc, char** argv) {
    std::vector<std::string_view> args;
    args.reserve(argc);

    bool is_server = false;

    for(int i = 1; i < argc; i++) {
        std::string_view arg = argv[i];

        if(arg.starts_with("--")) {
            if(arg == "--server") {
                is_server = true;
            }
        } else {
            args.push_back(arg);
        }
    }

    if(is_server) {
        if(args.size() > 0) {
            std::cerr << "andy-analyzer --server takes no arguments. Write <input-file>\\n<temp-file>\\n to stdin" << std::endl;
            return 1;
        }
    } else {
        if(args.size() < 1) {
            std::cerr << "andy-analyzer <input-file> [temp-file] or andy-analyzer --server" << std::endl;
            return 1;
        }
    }

    std::string arg0;
    std::string arg1;

    if(!is_server) {
        arg0 = args[0];

        if(args.size() > 1) {
            arg1 = args[1];
        } else {
            arg1 = arg0;
        }
    }

    bool run = true;

    while(run) {
        num_linter_warnings = 0;
        std::filesystem::path uva_executable_path = argv[0];

        if(is_server) {
            std::getline(std::cin, arg0);
            std::getline(std::cin, arg1);
        } else {
            run = false;
        }

        std::filesystem::path file_path = std::filesystem::absolute(arg0);
        std::filesystem::path temp_file_path = std::filesystem::absolute(arg1);

        auto start = std::chrono::high_resolution_clock::now();

        if(!std::filesystem::exists(file_path)) {
            std::cerr << "input file '" << file_path.string() << "' does not exist" << std::endl;
            exit(1);
        }

        if(!std::filesystem::is_regular_file(file_path)) {
            std::cerr << "input file '" << file_path.string() << "' is not a regular file" << std::endl;
            exit(1);
        }

        std::string source = uva::file::read_all_text<char>(temp_file_path);

        andy::lang::lexer l;
        std::string file_path_str = file_path.string();

        try {
            l.tokenize(file_path_str, source);
        } catch (const std::exception& e) {
            (void)e;
        }
        
        andy::lang::preprocessor preprocessor;
        preprocessor.process(file_path, l);

        // Note we are writing directly to the cout instead of saving and encoding the output

        std::cout << "{\n";

        std::cout << "\t\"tokens\": [\n";

        for(size_t i = 0; i < l.tokens().size(); i++) {
            const auto& token = l.tokens()[i];

            if(i) {
                std::cout << ",\n";
            }

            std::cout << "\t\t{\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
            write_path(token.m_file_name);
            std::cout << "\",\n\t\t\t\t\"line\": ";
            std::cout << token.start.line;
            std::cout << ",\n\t\t\t\t\"column\": ";
            std::cout << token.start.column;
            std::cout << ",\n\t\t\t\t\"offset\": ";
            std::cout << token.start.offset;
            std::cout << ",\n\t\t\t\t\"length\": ";
            std::cout << token.end.offset - token.start.offset;
            std::cout << "\n\t\t\t}";
            std::cout << ",\n\t\t\t\"type\": \"";
            std::cout << token.human_type();
            std::cout << "\"\n";
            std::cout << "\t\t}";
        }

        std::cout << "\n\t],\n";

        std::cout << "\t\"linter\": [\n";

        // Token level linting
        for(size_t i = 0; i < l.tokens().size(); i++) {
            const auto& token = l.tokens()[i];

            size_t offset = token.start.offset;
            size_t end_offset = token.end.offset;
            std::string_view source = l.source(token);
            int has_whitespace = 0;
            const char* end_it = source.data() + end_offset;

            // Check if the token is before a \n
            while(end_it != source.data()) {
                if(*end_it == '\n' || *end_it == 0) {
                    break;
                } else if(isspace(*end_it)) {
                    has_whitespace++;
                } else {
                    break;
                }
                end_it++;
            }

            if(has_whitespace && (*end_it == '\n' || *end_it == 0)) {
                write_linter_warning("trailing-whitespace", "Trailing whitespace", token.m_file_name, token.end, has_whitespace);
            }

            switch(token.type()) {
                case andy::lang::lexer::token_type::token_literal:
                    switch(token.kind()) {
                        case andy::lang::lexer::token_kind::token_string:
                            char c = source[offset];

                            switch(c)
                            {
                                case '\"':
                                    if(token.content().find("${") == std::string::npos) {
                                        write_linter_warning("string-default-single-quotes", "String literal without interpolation should use single quotes", token.m_file_name, token.start, token.content().size() + 2 /* 2 for the quotes */);
                                    }
                                break;
                            }
                        break;
                    }
                break;
            }
        }

        std::cout << "\n\t],\n";

        std::cout << "\t\"declarations\": [";

        andy::lang::parser p;
        andy::lang::parser::ast_node root_node;
        
        try {
            root_node = p.parse_all(l);
        } catch (const std::exception& e) {
            (void)e;
        }

        size_t node_i = 0;

        for(const auto& node : root_node.childrens()) {
            
            if(node.type() == andy::lang::parser::ast_node_type::ast_node_classdecl) {
                if(node_i) {
                    std::cout << ",";
                }

                node_i++;

                std::cout << "\n";

                const andy::lang::parser::ast_node* decname_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                const andy::lang::lexer::token& decname_token = decname_node->token();

                std::cout << "\t\t{\n\t\t\t\"type\": \"class\",\n\t\t\t\"name\": \"";
                std::cout << decname_token.content();
                std::cout << "\",\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
                write_path(decname_token.m_file_name);
                std::cout << "\",\n";
                std::cout << "\t\t\t\t\"line\": ";
                std::cout << decname_token.start.line;
                std::cout << ",\n\t\t\t\t\"column\": ";
                std::cout << decname_token.start.column;
                std::cout << ",\n\t\t\t\t\"offset\": ";
                std::cout << decname_token.start.offset;
                std::cout << "\n\t\t\t}";
                std::cout << ",\n\t\t\t\"references\": [";

                size_t token_i = 0;

                for(const auto& token : l.tokens()) {
                    if(token.type() == andy::lang::lexer::token_type::token_identifier) {
                        if(token.content() == decname_token.content()) {
                            if(token_i) {
                                std::cout << ",";
                            }

                            token_i++;

                            std::cout << "\n\t\t\t\t{\n\t\t\t\t\t\"file\": \"";
                            write_path(token.m_file_name);
                            std::cout << "\",\n\t\t\t\t\t\"line\": ";
                            std::cout << token.start.line;
                            std::cout << ",\n\t\t\t\t\t\"column\": ";
                            std::cout << token.start.column;
                            std::cout << ",\n\t\t\t\t\t\"offset\": ";
                            std::cout << token.start.offset;
                            std::cout << "\n\t\t\t\t}";
                        }
                    }
                }

                std::cout << "\n\t\t\t]";
                std::cout << "\n\t\t}";
            }
        }

        std::cout << "\n\t],\n";

        auto end = std::chrono::high_resolution_clock::now();

        std::cout << "\t\"elapsed\": \"" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\"\n";

        std::cout << "}";
    }

    return 0;
}