#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/extension.hpp>

std::shared_ptr<andy::lang::structure> create_system_class(andy::lang::interpreter* interpreter)
{
    auto SystemClass = std::make_shared<andy::lang::structure>("System");
    std::string_view current_os_name;

    #ifdef _WIN32
        current_os_name = "Windows";
    #elif __linux__
        current_os_name = "Linux";
    #elif __wasm__
        current_os_name = "WebAssembly";
    #else
        throw std::runtime_error("unsupported OS");
    #endif

    SystemClass->class_variables["OS"] = andy::lang::object::create(interpreter, interpreter->StringClass, std::string(current_os_name));

    SystemClass->class_variables["Windows?"]     = std::make_shared<andy::lang::object>(interpreter->FalseClass);
    SystemClass->class_variables["Linux?"]       = std::make_shared<andy::lang::object>(interpreter->FalseClass);
    SystemClass->class_variables["WebAssembly?"] = std::make_shared<andy::lang::object>(interpreter->FalseClass);

    SystemClass->class_variables[std::string(current_os_name) + "?"] = std::make_shared<andy::lang::object>(interpreter->TrueClass);

    return SystemClass;
}