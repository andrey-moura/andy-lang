#include <iostream>
#include <filesystem>

#include <andy/lang/api.hpp>

#include <andy/console.hpp>

#ifdef __ANDY_DEBUG__
    #define try_if_not_debug if(true)
    #define catch_if_not_debug(e) if(false)

    std::exception e;
#else
    #define try_if_not_debug try
    #define catch_if_not_debug(e) catch(e)
#endif

andy::lang::interpreter interpreter;

extern "C" const char* andy_interpreter_current_node_path() {
    std::cout << "Debugger requested current node path";
    if(!interpreter.current_node) {
        std::cout << " which is null" << std::endl;
        return nullptr;
    } else {
        std::cout << ": ";
    }
    std::cout << interpreter.current_node->token().file_name << std::endl;
    return interpreter.current_node->token().file_name->c_str();
}

int main(int argc, char** argv) {
    try_if_not_debug {
        std::filesystem::path andy_executable_path = argv[0];
        //vm_instance = std::make_shared<andy::lang::vm>();

        std::filesystem::path file_path;

        if(argc > 1) {
            std::string_view arg = argv[1];

            if(arg.starts_with("--")) {
                if(arg == "--help") {
                    std::cout << "Usage: " << argv[0] << " [file]" << std::endl;
                    std::cout << std::endl;
                    std::cout << "Options: " << std::endl;
                    andy::console::print_warning("  --help");
                    std::cout << "     Display this information" << std::endl;
                    andy::console::print_warning("  --version");
                    std::cout << "  Display the version of the andy language" << std::endl;
                    return 0;
                } else if(arg == "--version") {
                    std::cout << ANDYLANG_VERSION << std::endl;
                    return 0;
                } else {
                    arg.remove_prefix(2);
                    file_path = andy::lang::config::src_dir() / "utility" / arg;
                    file_path.replace_extension(".andy");

                    if(!std::filesystem::exists(file_path)) {
                        throw std::runtime_error("utility does not exist. Searched for '" + file_path.string() + "'");
                    }

                    if(!std::filesystem::is_regular_file(file_path)) {
                        throw std::runtime_error("utility is not a regular file. Searched for '" + file_path.string() + "'");
                    }
                }
            }
        }

        if(file_path.empty()) {
            if(argc > 1) {
                file_path = std::filesystem::absolute(argv[1]);
            } else {
                file_path = std::filesystem::absolute("application.andy");
            }

            if(!std::filesystem::exists(file_path)) {
                if(file_path.extension() != ".andy") {
                    file_path.replace_extension(".andy");
                }
                if(!std::filesystem::exists(file_path)) {
                    throw std::runtime_error("input file does not exist");
                }
            }

            if(!std::filesystem::is_regular_file(file_path)) {
                if(file_path.extension() != ".andy") {
                    file_path.replace_extension(".andy");
                }
                if(!std::filesystem::is_regular_file(file_path)) {
                    throw std::runtime_error("input file is not a regular file");
                }
            }
        }

        std::shared_ptr<andy::lang::object> ret;

        try {
            ret = andy::lang::api::evaluate(&interpreter, file_path, argc, argv);
        } catch(andy_lang_runtime_exception& e) {
            if(e.exception_object->cls == interpreter.StringClass) {
                andy::console::log_error(e.exception_object->as<std::string>());
            } else {
                andy::console::log_error("An exception occurred");
            }
            return 1;
        }

        if(!ret) {
            return 0;
        }

        if(ret) {
            // TODO: Treat the return value
            int ret_value = ret->as<int>();
            return ret_value;
        }
        
    } catch_if_not_debug (const std::exception& e) {
        andy::console::log_error(e.what());
        return 1;
    }

    return 0;
}