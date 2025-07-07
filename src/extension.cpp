#include <andy/lang/extension.hpp>

#include <iostream>
#include <string>

#include <uva.hpp>

#include <andy/lang/interpreter.hpp>

#ifdef __linux__
#   include <dlfcn.h>
#elif defined(_WIN32)
#   include <Windows.h>
#endif

#include <uva/file.hpp>

std::map<std::string_view, std::shared_ptr<andy::lang::extension>> andy::lang::extension::builtins;

andy::lang::extension::extension(const std::string &name)
    : m_name(name)
{
    
}

void andy::lang::extension::add_builtin(std::shared_ptr<andy::lang::extension> extension)
{
    builtins.insert({ extension->name(), extension });
}

andy::lang::extension* find_builtin(std::map<std::string_view, std::shared_ptr<andy::lang::extension>>& builtins, std::string_view module)
{
    auto it = builtins.find(module);
    if(it != builtins.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::filesystem::path find_module_path(
    std::map<std::string_view, std::shared_ptr<andy::lang::extension>>& builtins,
    std::string_view module,
    std::filesystem::path& current_path
)
{
    std::filesystem::path executable_path = uva::file::executable_path();
    std::filesystem::path module_path = executable_path;

    std::string library_name = std::string(module);

    std::string_view module_extension;

#if defined(__linux__)
    library_name = "libandylang-" + library_name;
    module_extension = ".so";
#elif defined(_WIN32)
    library_name = "andylang-" + library_name;
    module_extension = ".dll";
#else
    throw std::runtime_error("unsupported platform");
#endif

    module_path.replace_filename(library_name);
    module_path.replace_extension(module_extension);

    if(!std::filesystem::exists(module_path)) {
        std::filesystem::path provided_extension_path = current_path;
        provided_extension_path /= "lib";
        provided_extension_path /= "bin";
        provided_extension_path /= library_name;
        provided_extension_path.replace_extension(module_extension);

        return provided_extension_path;
    }

    return module_path;
}

bool andy::lang::extension::exists(std::filesystem::path current_dir, std::string_view module)
{
    auto it = builtins.find(module);
    if(it != builtins.end()) {
        return true;
    }

    std::filesystem::path module_path = find_module_path(builtins, module, current_dir);

    if(!std::filesystem::exists(module_path)) {
        return false;
    }

    return true;
}

void andy::lang::extension::import(andy::lang::interpreter* interpreter, std::string_view module)
{
    andy::lang::extension* builtin = find_builtin(builtins, module);
    if(builtin) {
        interpreter->load_extension(builtin);
        return;
    }

    std::filesystem::path module_path = find_module_path(builtins, module, interpreter->input_file_path);

    if(!std::filesystem::exists(module_path)) {
        throw std::runtime_error("Module " + std::string(module) + " not found. Expect it to be builtin or to be located at " + module_path.string());
    }

    std::string module_path_str = module_path.string();
    const char* module_path_c_str = module_path_str.c_str();

    andy::lang::extension* (*create_extension)();

#ifdef __linux__
    void* handle = dlopen(module_path_c_str, RTLD_LAZY | RTLD_GLOBAL);

    if(!handle) {
        throw std::runtime_error(dlerror());
    }

    create_extension = (andy::lang::extension*(*)())dlsym(handle, "create_extension");

    if(!create_extension) {
        throw std::runtime_error(dlerror());
    }
#elif defined(_WIN32)
    HMODULE handle = LoadLibrary(module_path_c_str);

    if(!handle) {
        throw std::runtime_error("Failed to load library");
    }

    create_extension = (andy::lang::extension*(*)())GetProcAddress(handle, "create_extension");

    if(!create_extension) {
        throw std::runtime_error("Failed to load symbol");
    }
#else
    throw std::runtime_error("unsupported platform");
#endif

    andy::lang::extension* extension = create_extension();

    interpreter->load_extension(extension);
}