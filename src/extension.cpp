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

//std::vector<andy::lang::extension*> extensions;

andy::lang::extension::extension(const std::string &name)
    : m_name(name)
{
    //extensions.push_back(this);
}

void andy::lang::extension::import(andy::lang::interpreter* interpreter, std::string_view module)
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
        std::filesystem::path provided_extension_path = interpreter->input_file_path.parent_path();
        provided_extension_path /= "lib";
        provided_extension_path /= "bin";
        provided_extension_path /= library_name;
        provided_extension_path.replace_extension(module_extension);

        if(!std::filesystem::exists(provided_extension_path)) {
            throw std::runtime_error("module " + std::string(module) + " not found. Expect it to be at " + module_path.string() + " or " + provided_extension_path.string());
        }

        module_path = provided_extension_path;
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