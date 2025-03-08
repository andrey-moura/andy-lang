#include <andy/lang/lang.hpp>

#include <andy/lang/interpreter.hpp>
#include <andy/lang/extension.hpp>

std::shared_ptr<andy::lang::structure> create_system_class(andy::lang::interpreter* interpreter)
{
    auto SystemClass = std::make_shared<andy::lang::structure>("System");
    SystemClass->methods = {
        {"Windows?", andy::lang::method("Windows?", andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        })},
        {"Linux?", andy::lang::method("Linux?", andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        })},
        {"WebAssembly?", andy::lang::method("WebAssembly?", andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        })},
    };
#ifdef _WIN32
        SystemClass->class_variables["OS"] = andy::lang::object::create(interpreter, interpreter->StringClass, std::move(std::string("Windows")));
        SystemClass->methods["Windows?"] = andy::lang::method("Windows?",andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });
#elif __linux__
        SystemClass->class_variables["OS"] = andy::lang::object::create(interpreter, interpreter->StringClass, std::move(std::string("Linux")));
        SystemClass->methods["Linux?"] = andy::lang::method("Linux?", andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });
#elif __wasm__
        SystemClass->class_variables["OS"] = andy::lang::object::create(interpreter, interpreter->StringClass, std::move(std::string("WebAssembly")));
        SystemClass->methods["WebAssembly?"] = andy::lang::method("WebAssembly?",andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });
#else
        throw std::runtime_error("unsupported OS");
#endif
    
    return SystemClass;
}