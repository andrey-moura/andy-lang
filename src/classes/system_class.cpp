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

    SystemClass->variables["OS"] = andy::lang::object::create(interpreter, interpreter->StringClass, std::string(current_os_name));

    SystemClass->variables["windows?"]     = std::make_shared<andy::lang::object>(interpreter->FalseClass);
    SystemClass->variables["linux?"]       = std::make_shared<andy::lang::object>(interpreter->FalseClass);
    SystemClass->variables["web_assembly?"] = std::make_shared<andy::lang::object>(interpreter->FalseClass);

#ifdef _WIN32
    SystemClass->variables["windows?"] = std::make_shared<andy::lang::object>(interpreter->TrueClass);
#elif __linux__
    SystemClass->variables["linux?"] = std::make_shared<andy::lang::object>(interpreter->TrueClass);
#elif __wasm__
    SystemClass->variables["web_assembly?"] = std::make_shared<andy::lang::object>(interpreter->TrueClass);
#endif

    return SystemClass;
}