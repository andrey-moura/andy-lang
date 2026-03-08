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

struct analyzer_error
{
    std::string_view type;
    std::string message;
    std::string_view file_name;
    andy::lang::lexer::token_position start;
    andy::lang::lexer::token_position end;
};

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
                << "  <path>                   Logical file path. Required.\n"
                << "                           Used for resolving relative #includes and identifying the source logically.\n\n"
                << "Options:\n"
                << "  --stdin                  Read the file contents from standard input (default),\n"
                << "                           but treat the content as if it came from <path>.\n\n"
                << "  --temp <temp-path>       Read file contents from the specified temporary file,\n"
                << "                           but treat it as if it was read from <path>.\n\n"
                << "  --stdout                 Write the output to standard output (default).\n\n"
                << "  --output <output-path>   Write the output to the specified file instead of standard output.\n\n"
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
    switch_option write_to_stdout { true, false };
    switch_option write_to_output { false, true };

    std::string tokenization_error;
    std::string parse_error;

    std::map<std::string_view, switch_option*> options = {
        { "--stdin",  &read_from_stdin },
        { "--temp",   &read_from_temp  },
        { "--server", &is_server       },
        { "--stdout", &write_to_stdout },
        { "--out",    &write_to_output }
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

        struct analyzer_declaration
        {
            std::string_view name;
            std::string_view type;
            std::string_view file;
            andy::lang::lexer::token_position start;
            andy::lang::lexer::token_position end;

            std::vector<analyzer_declaration> functions;
            std::vector<analyzer_declaration> variables;
        };

        struct analyzer_reference : public analyzer_declaration
        {
            analyzer_declaration declaration;
        };

        using linter_warning = analyzer_error;

        std::vector<analyzer_reference> references;
        std::vector<analyzer_error> errors;
        std::vector<linter_warning> linter_warnings;

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
        // create_builtin_libs();
        std::string file_path_str = file_path.string();
        andy::lang::lexer l(file_path_str, source);

        try {
            l.tokenize();
        } catch (const std::exception& e) {
            (void)e;
        }
        struct analyzer_token {
            std::string_view type;
            andy::lang::lexer::token token;
            std::string_view modifier;
        };
        std::vector<analyzer_token> tokens_to_write;

        // Preprocessor must be processed before the preprocessor run
        for(const auto& token : l.tokens()) {
            switch(token.type) {    
                case andy::lang::lexer::token_type::token_preprocessor:
                    tokens_to_write.push_back({ "keyword", token });
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

        size_t i = 0;
        std::function<void(const andy::lang::parser::ast_node& node)> inspect_node_for_errors;
        inspect_node_for_errors = [&](const andy::lang::parser::ast_node& node) {
            if(node.type() == andy::lang::parser::ast_node_type::ast_node_fn_call) {
                const andy::lang::parser::ast_node* fn_declname_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                const andy::lang::lexer::token& fn_declname_token = fn_declname_node->token();
                std::string_view fn_declname = fn_declname_token.content;
                
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
                std::string_view import_declname = import_declname_token.content;

                // if(andy::lang::extension::exists(file_directory, import_declname)) {
                //     andy::lang::extension::import(&interpreter, import_declname);
                //     return;
                // }

                // errors.push_back(error{
                //     "missing-import",
                //     "Cannot find import " + std::string(import_declname),
                //     import_declname_token.m_file_name,
                //     import_declname_token.start.line,
                //     import_declname_token.start.column,
                //     import_declname_token.start.offset,
                //     import_declname_token.end.offset - import_declname_token.start.offset
                // });
            }
        };
        struct analyzer_context
        {
            std::string_view name;

            std::vector<analyzer_declaration> classes;
            std::vector<analyzer_declaration> variables;
            std::vector<analyzer_declaration> functions;
        };
        std::vector<analyzer_context> stack;
        analyzer_context* current_context = nullptr;
        analyzer_context* global_context = nullptr;
        analyzer_context* previous_context = nullptr;
        auto push_context = [&](bool inherit_previous = false) {
            stack.push_back(analyzer_context{});
            previous_context = stack.size() > 1 ? &stack[stack.size() - 2] : nullptr;
            current_context = &stack.back();
            global_context = &stack.front();

            if(inherit_previous && previous_context) {
                // Bring previous declarations to the current_context
                for(const auto& var : previous_context->variables) {
                    current_context->variables.push_back(var);
                }

                for(const auto& func : previous_context->functions) {
                    current_context->functions.push_back(func);
                }

                for(const auto& cls : previous_context->classes) {
                    current_context->classes.push_back(cls);
                }
            }
        };
        auto pop_context = [&]() {
            stack.pop_back();
            current_context = stack.empty() ? nullptr : &stack.back();
            previous_context = stack.empty() ? nullptr : &stack.back();
            global_context = &stack.front();
        };
        push_context();
        for(const auto& var : interpreter.stack[0]->variables) {
            if(var.first.ends_with("?")) {
                analyzer_declaration decl;
                decl.name = var.first;
                decl.type = "function";
                current_context->functions.push_back(std::move(decl));
            } else {
                analyzer_declaration decl;
                decl.name = var.first;
                decl.type = "variable";
                current_context->variables.push_back(std::move(decl));
            }
        }
        for(const auto& func : interpreter.stack[0]->functions) {
            analyzer_declaration decl;
            decl.name = func.first;
            decl.type = "function";
            current_context->functions.push_back(std::move(decl));
        }
        for(const auto& cls : interpreter.stack[0]->classes) {
            analyzer_declaration cls_decl;
            cls_decl.name = cls.first;
            cls_decl.type = "class";

            for(const auto& var : cls.second->variables) {
                if(var.first.ends_with("?")) {
                    analyzer_declaration decl;
                    decl.name = var.first;
                    decl.type = "function";
                    cls_decl.functions.push_back(std::move(decl));
                } else {
                    analyzer_declaration decl;
                    decl.name = var.first;
                    decl.type = "variable";
                    cls_decl.variables.push_back(std::move(decl));
                }
            }
            current_context->classes.push_back(cls_decl);
        }
        std::function<void(const andy::lang::parser::ast_node& node)> inspect_node;
        std::function<void(const andy::lang::parser::ast_node& node)> switch_type;
        auto push_context_from_node_object_if_any = [&](const andy::lang::parser::ast_node& node) {
            const andy::lang::parser::ast_node* object_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object);

            if(object_node) {
                push_context();
                auto& child = object_node->childrens().front();
                std::string_view object_name = child.token().content;
                for(const auto& cls : global_context->classes) {
                    if(cls.name == object_name) {
                        for(const auto& func : cls.functions) {
                            current_context->functions.push_back(func);
                        }
                        for(const auto& var : cls.variables) {
                            if(var.name.ends_with("?")) {
                                current_context->functions.push_back(var);
                            } else {
                                current_context->variables.push_back(var);
                            }
                        }
                        break;
                    }
                }
                inspect_node(child);
                pop_context();
            }
        };
        auto pop_context_from_node_object_if_any = [&](const andy::lang::parser::ast_node& node) {
            const andy::lang::parser::ast_node* object_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object);

            if(object_node) {
                pop_context();
            }
        };
        inspect_node = [&](const andy::lang::parser::ast_node& node) {
            switch(node.type()) {
                case andy::lang::parser::ast_node_type::ast_node_unit:
                    for(const auto& child : node.childrens()) {
                        inspect_node(child);
                    }
                break;
                case andy::lang::parser::ast_node_type::ast_node_classdecl:
                {
                    auto* declname_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                    std::string_view class_name = declname_node->token().content;
                    tokens_to_write.push_back({ "class", declname_node->token() });

                    analyzer_declaration cls;
                    cls.name = class_name;
                    cls.type = "class";
                    current_context->classes.push_back(cls);
                    push_context();
                    for(const auto& child : node.context()->childrens()) {
                        inspect_node(child);
                    }
                    pop_context();
                }
                break;
                case andy::lang::parser::ast_node_type::ast_node_fn_decl:
                {
                    auto* declname_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                    std::string_view function_name = declname_node->token().content;

                    tokens_to_write.push_back({ "function", declname_node->token() });

                    analyzer_declaration decl;
                    decl.name = function_name;
                    decl.type = "function";
                    decl.file = *declname_node->token().file_name;
                    decl.start = declname_node->token().start;
                    decl.end = declname_node->token().end;

                    current_context->functions.push_back(std::move(decl));
                    push_context();

                    auto fn_params = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params);
                    if(fn_params) {
                        for(const auto& param : fn_params->childrens()) {
                            if(param.type() == andy::lang::parser::ast_node_type::ast_node_declname) {
                                analyzer_declaration decl;
                                decl.name = param.token().content;
                                decl.type = "variable";
                                decl.file = *param.token().file_name;
                                decl.start = param.token().start;
                                decl.end = param.token().end;

                                tokens_to_write.push_back({ "variable", param.token() });
                                current_context->variables.push_back(std::move(decl));
                            }
                        }
                    }
                    for(const auto& child : node.context()->childrens()) {
                        inspect_node(child);
                    }
                    pop_context();
                }
                break;
                case andy::lang::parser::ast_node_type::ast_node_fn_call: {
                    auto* declname_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                    std::string_view function_name = declname_node->token().content;
                    if(function_name == "=") {
                        tokens_to_write.push_back({ "function", declname_node->token() });
                    } else {
                        inspect_node(*declname_node);
                    }
                    auto* fn_params_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params);
                    if(fn_params_node) {
                        for(const auto& param : fn_params_node->childrens()) {
                            inspect_node(param);
                        }
                    }
                    auto* fn_object_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object);
                    if(fn_object_node) {
                        inspect_node(fn_object_node->childrens().front());
                    }
                    auto* yield_block_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_context);
                    if(yield_block_node) {
                        const auto& token = yield_block_node->token();
                        tokens_to_write.push_back({ "keyword", token });
                        push_context(true);
                        for(const auto& child : yield_block_node->childrens()) {
                            inspect_node(child);
                        }
                        pop_context();
                    }
                }
                break;
                case andy::lang::parser::ast_node_type::ast_node_vardecl:
                {
                    auto* declname_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                    std::string_view variable_name = declname_node->token().content;
                    tokens_to_write.push_back({ "variable", declname_node->token() });

                    analyzer_declaration decl;
                    decl.name = variable_name;
                    decl.type = "variable";
                    decl.file = *declname_node->token().file_name;
                    decl.start = declname_node->token().start;
                    decl.end = declname_node->token().end;
                    current_context->variables.push_back(std::move(decl));

                    auto* value_node = node.childrens().size() >= 3 ? &node.childrens()[2] : nullptr;
                    if(value_node) {
                        inspect_node(*value_node);
                    }
                }
                break;
                case andy::lang::parser::ast_node_type::ast_node_declname:
                {
                    std::string_view name = node.token().content;

                    push_context_from_node_object_if_any(node);
                    auto search_for_name_in_context = [&](analyzer_context* context) {
                        auto cls_it = std::find_if(context->classes.begin(), context->classes.end(), [&](const analyzer_declaration& cls) { return cls.name == name; });
                        if (cls_it != context->classes.end()) {
                            return &*cls_it;
                        }
                        auto func_it = std::find_if(context->functions.begin(), context->functions.end(), [&](const analyzer_declaration& func) { return func.name == name; });
                        if (func_it != context->functions.end()) {
                            return &*func_it;
                        }
                        auto var_it = std::find_if(context->variables.begin(), context->variables.end(), [&](const analyzer_declaration& var) { return var.name == name; });
                        if (var_it != context->variables.end()) {
                            return &*var_it;
                        }

                        return (analyzer_declaration*)nullptr;
                    };

                    analyzer_declaration* type = search_for_name_in_context(current_context);

                    search_for_name_in_context(current_context);

                    if(type == nullptr && current_context != global_context) {
                        type = search_for_name_in_context(global_context);
                    }

                    if(type == nullptr) {
                        analyzer_error error;
                        error.type = "undefined-symbol";
                        error.message = "Undefined symbol '" + std::string(name) + "'";
                        error.file_name = *node.token().file_name;
                        error.start = node.token().start;
                        error.end = node.token().end;
                        errors.push_back(std::move(error));
                    } else {
                        analyzer_reference reference;
                        reference.name = name;
                        reference.start = node.token().start;
                        reference.end = node.token().end;
                        reference.file = *node.token().file_name;
                        reference.type = type->type;
                        reference.declaration = *type;
                        references.push_back(std::move(reference));

                        tokens_to_write.push_back({ type->type, node.token()});
                    }
                }
                break;
                case andy::lang::parser::ast_node_type::ast_node_conditional:
                {
                    auto* condition = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_condition);
                    auto* condition_child = condition->childrens().data();
                    inspect_node(*condition_child);
                    push_context(true);
                    for(const auto& child : node.context()->childrens()) {
                        inspect_node(child);
                    }
                    pop_context();
                    for(const auto& child : node.childrens()) {
                        switch(child.type()) {
                            case andy::lang::parser::ast_node_type::ast_node_conditional:
                                inspect_node(child);
                            break;
                            case andy::lang::parser::ast_node_type::ast_node_else:
                                for(const auto& else_child : child.context()->childrens()) {
                                    inspect_node(else_child);
                                 }
                            break;
                        }
                    }
                }
                break;
                case andy::lang::parser::ast_node_type::ast_node_foreach:
                    {
                        push_context(true);

                        tokens_to_write.push_back({ "keyword", node.child_from_type(andy::lang::parser::ast_node_type::ast_node_decltype)->token() });
                        auto* vardecl = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_vardecl);
                        if(vardecl) {
                            auto* declname_node = vardecl->child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                            if(declname_node) {
                                std::string_view variable_name = declname_node->token().content;

                                analyzer_declaration decl;
                                decl.name = variable_name;
                                decl.type = "variable";
                                decl.file = *declname_node->token().file_name;
                                decl.start = declname_node->token().start;
                                decl.end = declname_node->token().end;

                                tokens_to_write.push_back({ "variable", declname_node->token() });
                                current_context->variables.push_back(std::move(decl));
                            }
                        }
                        auto* value_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);
                        if(value_node) {
                            inspect_node(value_node->childrens().front());
                            tokens_to_write.push_back({ "keyword", value_node->childrens().back().token() });
                        }
                        for(const auto& child : node.context()->childrens()) {
                            inspect_node(child);
                        }
                        pop_context();
                    }
                break;
                default:
                    // std::cerr << "unhandled node type " << (int)node.type() << std::endl;
                break;
            }
        };

        inspect_node(root_node);

        for(size_t i = 0; i < l.tokens().size(); ++i) {
            const auto& token = l.tokens()[i];
            switch(token.type) {
                case andy::lang::lexer::token_type::token_comment:
                    tokens_to_write.push_back({ "comment", token });
                break;
                case andy::lang::lexer::token_type::token_keyword:
                    tokens_to_write.push_back({ "keyword", token });
                break;
                case andy::lang::lexer::token_type::token_literal:
                    switch(token.kind) {
                        case andy::lang::lexer::token_kind::token_integer:
                        case andy::lang::lexer::token_kind::token_float:
                        case andy::lang::lexer::token_kind::token_double:
                            tokens_to_write.push_back({ "number", token });
                        break;
                        case andy::lang::lexer::token_kind::token_string:
                            tokens_to_write.push_back({ "string", token });
                        break;
                        case andy::lang::lexer::token_kind::token_boolean:
                            tokens_to_write.push_back({ "keyword", token });
                        break;
                    }
                break;
                case andy::lang::lexer::token_type::token_delimiter:
                    if(token.content == "end") {
                        tokens_to_write.push_back({ "keyword", token });
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
            for(const auto& c : token.token.content)
            {
                switch (c)
                {
                    case '\\':
                        buffer += "\\\\";
                        break;
                    default:
                        buffer.push_back(c);
                        break;
                }
            }
            buffer += "\",\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
            write_path(*token.token.file_name);
            buffer += "\",\n\t\t\t\t\"start\": {\n\t\t\t\t\t\"line\": ";
            buffer += std::to_string(token.token.start.line);
            buffer += ",\n\t\t\t\t\t\"column\": ";
            buffer += std::to_string(token.token.start.column);
            buffer += ",\n\t\t\t\t\t\"offset\": ";
            buffer += std::to_string(token.token.start.offset);
            buffer += "\n\t\t\t\t},\n\t\t\t\t\"end\": {\n\t\t\t\t\t\"line\": ";
            buffer += std::to_string(token.token.end.line);
            buffer += ",\n\t\t\t\t\t\"column\": ";
            buffer += std::to_string(token.token.end.column);
            buffer += ",\n\t\t\t\t\t\"offset\": ";
            buffer += std::to_string(token.token.end.offset);
            buffer += "\n\t\t\t\t}\n\t\t\t}\n\t\t}";
        }

        buffer += "\n\t],\n";

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

        //     switch(token.type) {
        //         case andy::lang::lexer::token_type::token_literal:
        //             switch(token.kind()) {
        //                 case andy::lang::lexer::token_kind::token_string:
        //                     char c = source[offset];

        //                     switch(c)
        //                     {
        //                         case '\"':
        //                             if(token.content.find("${") == std::string::npos) {
        //                                 write_linter_warning("string-default-single-quotes", "String literal without interpolation should use single quotes", token.m_file_name, token.start, token.content.size() + 2 /* 2 for the quotes */);
        //                             }
        //                         break;
        //                     }
        //                 break;
        //             }
        //         break;
        //     }
        // }

        buffer += "\t\"references\": [";

        for(size_t i = 0; i < references.size(); i++) {
            const auto& reference = references[i];
            if(i) {
                buffer += ",";
            }

            buffer += "\n\t\t{\n\t\t\t\"type\": \"";
            buffer += reference.type;
            buffer += "\",\n\t\t\t\"name\": \"";
            buffer += reference.name;
            buffer += "\",\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
            write_path(reference.file);
            buffer += "\",\n\t\t\t\t\"start\": {\n\t\t\t\t\t\"line\": ";
            buffer += std::to_string(reference.start.line);
            buffer += ",\n\t\t\t\t\t\"column\": ";
            buffer += std::to_string(reference.start.column);
            buffer += ",\n\t\t\t\t\t\"offset\": ";
            buffer += std::to_string(reference.start.offset);
            buffer += "\n\t\t\t\t},\n\t\t\t\t\"end\": {\n\t\t\t\t\t\"line\": ";
            buffer += std::to_string(reference.end.line);
            buffer += ",\n\t\t\t\t\t\"column\": ";
            buffer += std::to_string(reference.end.column);
            buffer += ",\n\t\t\t\t\t\"offset\": ";
            buffer += std::to_string(reference.end.offset);
            buffer += "\n\t\t\t\t}";
            buffer += "\n\t\t\t},";
            buffer += "\n\t\t\t\"declaration\": {";
            buffer += "\n\t\t\t\t\"type\": \"";
            buffer += reference.declaration.type;
            buffer += "\",\n\t\t\t\t\"name\": \"";
            buffer += reference.declaration.name;
            buffer += "\",\n\t\t\t\t\"location\": {\n\t\t\t\t\t\"file\": \"";
            write_path(reference.declaration.file);
            buffer += "\",\n\t\t\t\t\t\"start\": {\n\t\t\t\t\t\t\"line\": ";
            buffer += std::to_string(reference.declaration.start.line);
            buffer += ",\n\t\t\t\t\t\t\"column\": ";
            buffer += std::to_string(reference.declaration.start.column);
            buffer += ",\n\t\t\t\t\t\t\"offset\": ";
            buffer += std::to_string(reference.declaration.start.offset);
            buffer += "\n\t\t\t\t\t},\n\t\t\t\t\t\"end\": {\n\t\t\t\t\t\t\"line\": ";
            buffer += std::to_string(reference.declaration.end.line);
            buffer += ",\n\t\t\t\t\t\t\"column\": ";
            buffer += std::to_string(reference.declaration.end.column);
            buffer += ",\n\t\t\t\t\t\t\"offset\": ";
            buffer += std::to_string(reference.declaration.end.offset);
            buffer += "\n\t\t\t\t\t}\n\t\t\t\t}";
            buffer += "\n\t\t\t}";
            buffer += "\n\t\t}";
        }

        buffer +="\n\t],\n";

        buffer += "\t\"errors\": [\n";

        for(size_t i = 0; i < errors.size(); i++) {
            const auto& error = errors[i];
            if(i) {
                buffer += ",\n";
            }
            buffer += "\t\t{\n\t\t\t\"message\": \"";
            buffer += error.message;
            buffer += "\",";
            buffer += "\n\t\t\t\"location\": {\n\t\t\t\t\"file\": \"";
            write_path(error.file_name);
            buffer += "\",\n\t\t\t\t\"start\": {\n\t\t\t\t\t\"line\": ";
            buffer += std::to_string(error.start.line);
            buffer += ",\n\t\t\t\t\t\"column\": ";
            buffer += std::to_string(error.start.column);
            buffer += ",\n\t\t\t\t\t\"offset\": ";
            buffer += std::to_string(error.start.offset);
            buffer += "\n\t\t\t\t},\n\t\t\t\t\"end\": {\n\t\t\t\t\t\"line\": ";
            buffer += std::to_string(error.end.line);
            buffer += ",\n\t\t\t\t\t\"column\": ";
            buffer += std::to_string(error.end.column);
            buffer += ",\n\t\t\t\t\t\"offset\": ";
            buffer += std::to_string(error.end.offset);
            buffer += "\n\t\t\t\t}\n\t\t\t}\n\t\t}";
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
        if(write_to_output) {
            std::ofstream output_file(write_to_output.argument.data());
            if(!output_file) {
                std::cerr << "Failed to open output file '" << write_to_output.argument << "' for writing" << std::endl;
                return 1;
            }
            output_file << buffer;
            output_file.close();
        } else if(write_to_stdout) {
            std::cout << buffer;
        }
        //} else {
            //std::cout << buffer << std::endl;
        //}
    }

    return 0;
}