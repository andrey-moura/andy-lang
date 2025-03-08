#include <iostream>

#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/extension.hpp>

std::shared_ptr<andy::lang::structure> create_std_class(andy::lang::interpreter* interpreter)
{
    auto StdClass = std::make_shared<andy::lang::structure>("Standard");

    StdClass->class_methods = {
        { "print", andy::lang::method("print",andy::lang::method_storage_type::class_method, {"message"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::shared_ptr<andy::lang::object> obj = params[0];
            if(obj->cls == interpreter->StringClass) {
                std::cout << obj->as<std::string>();
            } else {
                std::string s = obj->cls->instance_methods["to_string"].call(obj)->as<std::string>();
                std::cout << s;
            }

            return nullptr;
        })},

        { "puts", andy::lang::method("puts",andy::lang::method_storage_type::class_method, {"message"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::shared_ptr<andy::lang::object> obj = params[0];
            if(obj->cls == interpreter->StringClass) {
                std::cout << obj->as<std::string>() << std::endl;
            } else {
                std::string s = obj->cls->instance_methods["to_string"].call(obj)->as<std::string>();
                std::cout << s << std::endl;
            }

            return nullptr;
        })},

        { "gets", andy::lang::method("gets",andy::lang::method_storage_type::class_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string line;
            std::getline(std::cin, line);

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(line));
        })},

        { "system", andy::lang::method("system",andy::lang::method_storage_type::class_method, {"command"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::shared_ptr<andy::lang::object> command = params[0]->cls->instance_methods["to_string"].call(params[0]);
            int code = ((std::system(command->as<std::string>().c_str())) & 0xff00) >> 8;

            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, code);
        })},

        { "import", andy::lang::method("import",andy::lang::method_storage_type::class_method, {"module"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string module = params[0]->as<std::string>();
            andy::lang::extension::import(interpreter, module);
            return nullptr;
        })},
    };
    
    return StdClass;
}