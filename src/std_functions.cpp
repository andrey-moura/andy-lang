#ifdef _WIN32
#   include <Windows.h>
#endif

#include <iostream>
#include <random>
#include <filesystem>
#include <fstream>

#include <andy/lang/lang.hpp>
#include <andy/lang/error.hpp>
#include <andy/lang/api.hpp>
#include <andy/lang/preprocessor.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/extension.hpp>

void create_std_functions(andy::lang::interpreter* interpreter)
{
    auto RandomClass = std::make_shared<andy::lang::structure>("Random");
    RandomClass->functions["integer"] = std::make_shared<andy::lang::function>("integer", [](andy::lang::interpreter* interpreter) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(INT_MIN, INT_MAX);
        return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, dis(gen));
    });

    interpreter->load(RandomClass);

    interpreter->global_context->functions["print"] = std::make_shared<andy::lang::function>("print",std::initializer_list<std::string>{"message"}, [](andy::lang::interpreter* interpreter) {
        std::shared_ptr<andy::lang::object> obj = interpreter->current_context->positional_params[0];
        if(obj->cls == interpreter->StringClass) {
            std::cout << obj->as<std::string>();
        } else {
            //std::string s = obj->cls->instance_functions["to_string"]->call(obj)->as<std::string>();
            //std::cout << s;
            andy::lang::error::internal("Disabled code reached");
        }

        return nullptr;
    });

    interpreter->global_context->functions["out"] = std::make_shared<andy::lang::function>("out",std::initializer_list<std::string>{"message"}, [](andy::lang::interpreter* interpreter) {
        std::shared_ptr<andy::lang::object> obj = interpreter->current_context->positional_params[0];
#ifdef _WIN32
        static bool have_console_have_been_set = false;
        if (!have_console_have_been_set) {
            SetConsoleOutputCP(CP_UTF8);
            have_console_have_been_set = true;
        }
#endif
        if(obj->cls == interpreter->StringClass) {
            std::cout << obj->as<std::string>() << std::endl;
        } else {
            auto string_object = andy::lang::api::call(interpreter, "to_string", obj);
            std::cout << string_object->as<std::string>() << std::endl;
        }

        return nullptr;
    });

    interpreter->global_context->functions["gets"] = std::make_shared<andy::lang::function>("gets", [](andy::lang::interpreter* interpreter) {
        std::string line;
        std::getline(std::cin, line);

        return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(line));
    });

    interpreter->global_context->functions["system"] = std::make_shared<andy::lang::function>("system",std::initializer_list<std::string>{"command"}, [](andy::lang::interpreter* interpreter) {
        auto argument = interpreter->current_context->positional_params[0];
//        std::shared_ptr<andy::lang::object> command = argument->cls->instance_functions["to_string"]->call(argument);
        andy::lang::error::internal("Temporary disabled code reached at " + std::string(__FILE__) + ":" + std::to_string(__LINE__));
        auto command = std::make_shared<andy::lang::object>(interpreter->StringClass);
        int code = std::system(command->as<std::string>().c_str());
#ifdef __linux__
        code = WEXITSTATUS(code);
#endif
        return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, code);
    });

    interpreter->global_context->functions["import"] = std::make_shared<andy::lang::function>("import",std::initializer_list<std::string>{"module"}, [](andy::lang::interpreter* interpreter) {
        std::string module = interpreter->current_context->positional_params[0]->as<std::string>();

        interpreter->stack.push_back(interpreter->global_context);
        andy::lang::extension::import(interpreter, module);
        interpreter->stack.pop_back();

        return nullptr;
    });

    interpreter->global_context->functions["require"] = std::make_shared<andy::lang::function>("require",std::initializer_list<std::string>{"module"}, [](andy::lang::interpreter* interpreter) {
        // yep, the code is kept in memory until the program ends

        const std::filesystem::path& file_path = interpreter->current_context->positional_params[0]->as<std::filesystem::path>();
        std::string file_path_str = file_path.string();

        std::ifstream f(file_path, std::ios::binary);
        if(!f.is_open()) {
            throw std::runtime_error("Cannot open file: " + file_path_str);
        }

        size_t size = 0;
        f.seekg(0, std::ios::end);
        size = f.tellg();
        f.seekg(0);

        std::string code;
        code.resize(size);
        f.read(code.data(), size);
        code.resize(f.gcount());

        andy::lang::lexer lexer(std::move(file_path_str), std::move(code));

        for(const auto& include : interpreter->main_lexer->includes()) {
            lexer.include_from_parent(include);
        }

        lexer.tokenize();

        andy::lang::preprocessor preprocessor;
        preprocessor.process(lexer.path(), lexer);

        andy::lang::parser parser;
        auto* ast = new andy::lang::parser::ast_node(std::move(parser.parse_all(lexer)));

        auto ret = interpreter->execute_all(*ast);

        for(auto& cls : interpreter->current_context->classes) {
            interpreter->previous_context->classes[cls.first] = cls.second;
        }

        for(auto& fn : interpreter->current_context->functions) {
            interpreter->previous_context->functions[fn.first] = fn.second;
        }

        for(auto& var : interpreter->current_context->variables) {
            interpreter->previous_context->variables[var.first] = var.second;
        }

        return ret;
    });

    interpreter->global_context->functions["__file__"] = std::make_shared<andy::lang::function>("__file__", [](andy::lang::interpreter* interpreter) {
        auto current_context = interpreter->current_context;
        auto caller_node = current_context->caller_node;
        if(caller_node == nullptr) {
            return andy::lang::api::to_object(interpreter, "<interactive>");
        }
        const andy::lang::parser::ast_node* ast_node_declname = nullptr;
        if(caller_node->type() == andy::lang::parser::ast_node_type::ast_node_fn_call) {
            ast_node_declname = caller_node->child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
        } else if (caller_node->type() == andy::lang::parser::ast_node_type::ast_node_declname) {
            ast_node_declname = caller_node;
        }
        if(ast_node_declname == nullptr) {
            return andy::lang::api::to_object(interpreter, "<unamed>");
        }
        auto file_name = ast_node_declname->token().file_name;
        if(file_name == nullptr) {
            return andy::lang::api::to_object(interpreter, "<unknown>");
        }
        return andy::lang::api::to_object(interpreter, *file_name);
    });

    interpreter->global_context->functions["__dir__"] = std::make_shared<andy::lang::function>("__dir__", [](andy::lang::interpreter* interpreter) {
        auto current_context = interpreter->current_context;
        auto caller_node = current_context->caller_node;
        if(caller_node == nullptr) {
            return andy::lang::api::to_object(interpreter, "<interactive>");
        }
        const andy::lang::parser::ast_node* ast_node_declname = nullptr;
        if(caller_node->type() == andy::lang::parser::ast_node_type::ast_node_fn_call) {
            ast_node_declname = caller_node->child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
        } else if (caller_node->type() == andy::lang::parser::ast_node_type::ast_node_declname) {
            ast_node_declname = caller_node;
        }
        if(ast_node_declname == nullptr) {
            return andy::lang::api::to_object(interpreter, "<unamed>");
        }
        auto file_name = ast_node_declname->token().file_name;
        if(file_name == nullptr) {
            return andy::lang::api::to_object(interpreter, "<unknown>");
        }
        std::filesystem::path path(*file_name);
        if(path.has_parent_path()) {
            return andy::lang::api::to_object(interpreter, path.parent_path().string());
        }
        return andy::lang::api::to_object(interpreter, ".");
    });

    interpreter->global_context->functions["__argv__"] = std::make_shared<andy::lang::function>("__argv__", [](andy::lang::interpreter* interpreter) {
        return andy::lang::api::to_object(interpreter, interpreter->args);
    });
}