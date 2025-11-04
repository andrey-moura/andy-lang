#ifdef _WIN32
#   include <Windows.h>
#endif

#include <iostream>
#include <random>

#include <andy/lang/lang.hpp>
#include <andy/lang/api.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/extension.hpp>

std::shared_ptr<andy::lang::structure> create_std_class(andy::lang::interpreter* interpreter)
{
    auto RandomClass = std::make_shared<andy::lang::structure>("Random");
    RandomClass->class_functions = {
        { "integer", andy::lang::function("integer",andy::lang::function_storage_type::class_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dis(INT_MIN, INT_MAX);
            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, dis(gen));
        })},
    };

    interpreter->load(RandomClass);

    auto StdClass = std::make_shared<andy::lang::structure>("Standard");

    StdClass->class_functions = {
        { "print", andy::lang::function("print",andy::lang::function_storage_type::class_function, {"message"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::shared_ptr<andy::lang::object> obj = params[0];
            if(obj->cls == interpreter->StringClass) {
                std::cout << obj->as<std::string>();
            } else {
                std::string s = obj->cls->instance_functions["to_string"]->call(obj)->as<std::string>();
                std::cout << s;
            }

            return nullptr;
        })},

        { "out", andy::lang::function("out",andy::lang::function_storage_type::class_function, {"message"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
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
        })},

        { "gets", andy::lang::function("gets",andy::lang::function_storage_type::class_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string line;
            std::getline(std::cin, line);

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(line));
        })},

        { "system", andy::lang::function("system",andy::lang::function_storage_type::class_function, {"command"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::shared_ptr<andy::lang::object> command = params[0]->cls->instance_functions["to_string"]->call(params[0]);
            int code = std::system(command->as<std::string>().c_str());
#ifdef __linux__
            code = WEXITSTATUS(code);
#endif
            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, code);
        })},

        { "import", andy::lang::function("import",andy::lang::function_storage_type::class_function, {"module"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string module = params[0]->as<std::string>();
            andy::lang::extension::import(interpreter, module);
            return nullptr;
        })},
    };
    
    return StdClass;
}