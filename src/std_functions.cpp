#ifdef _WIN32
#   include <Windows.h>
#endif

#include <iostream>
#include <random>

#include <andy/lang/lang.hpp>
#include <andy/lang/api.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/extension.hpp>

void create_std_functions(andy::lang::interpreter* interpreter)
{
    auto RandomClass = std::make_shared<andy::lang::structure>("Random");
    RandomClass->functions["integer"] = std::make_shared<andy::lang::function>("integer",andy::lang::function_storage_type::class_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(INT_MIN, INT_MAX);
        return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, dis(gen));
    });

    interpreter->load(RandomClass);

    interpreter->global_context->functions["print"] = std::make_shared<andy::lang::function>("print",andy::lang::function_storage_type::class_function,std::initializer_list<std::string>{"message"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::shared_ptr<andy::lang::object> obj = params[0];
        if(obj->cls == interpreter->StringClass) {
            std::cout << obj->as<std::string>();
        } else {
            std::string s = obj->cls->instance_functions["to_string"]->call(obj)->as<std::string>();
            std::cout << s;
        }

        return nullptr;
    });

    interpreter->global_context->functions["out"] = std::make_shared<andy::lang::function>("out",andy::lang::function_storage_type::class_function,std::initializer_list<std::string>{"message"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::shared_ptr<andy::lang::object> obj = params[0];
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
            std::string s = andy::lang::api::call<std::string>(interpreter, andy::lang::function_call{
                "to_string",
                obj->cls,
                obj,
            });
            std::cout << s << std::endl;
        }

        return nullptr;
    });

    interpreter->global_context->functions["gets"] = std::make_shared<andy::lang::function>("gets",andy::lang::function_storage_type::class_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::string line;
        std::getline(std::cin, line);

        return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(line));
    });

    interpreter->global_context->functions["system"] = std::make_shared<andy::lang::function>("system",andy::lang::function_storage_type::class_function,std::initializer_list<std::string>{"command"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::shared_ptr<andy::lang::object> command = params[0]->cls->instance_functions["to_string"]->call(params[0]);
        int code = std::system(command->as<std::string>().c_str());
#ifdef __linux__
        code = WEXITSTATUS(code);
#endif
        return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, code);
    });

    interpreter->global_context->functions["import"] = std::make_shared<andy::lang::function>("import",andy::lang::function_storage_type::class_function,std::initializer_list<std::string>{"module"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        auto current_context = interpreter->current_context;
        interpreter->pop_context();

        std::string module = params[0]->as<std::string>();
        andy::lang::extension::import(interpreter, module);

        interpreter->stack.push_back(current_context);
        interpreter->update_current_context();

        return nullptr;
    });
    interpreter->global_context->functions["__file__"] = std::make_shared<andy::lang::function>("__file__",andy::lang::function_storage_type::class_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
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
}