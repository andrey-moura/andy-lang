#include <iostream>
#include <filesystem>

#include <andy/lang/api.hpp>

#include <uva/console.hpp>

#ifdef __UVA_DEBUG__
    #define try if(true)
    #define catch(e) if(false)

    std::exception e;
#endif

int main(int argc, char** argv) {
    try {
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
                    uva::console::print_warning("  --help");
                    std::cout << "     Display this information" << std::endl;
                    uva::console::print_warning("  --version");
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
                        throw std::runtime_error("utility does not exist");
                    }

                    if(!std::filesystem::is_regular_file(file_path)) {
                        throw std::runtime_error("utility is not a regular file");
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
                throw std::runtime_error("input file does not exist");
            }

            if(!std::filesystem::is_regular_file(file_path)) {
                throw std::runtime_error("input file is not a regular file");
            }
        }

        std::shared_ptr<andy::lang::object> ret = andy::lang::api::evaluate(file_path);

        if(!ret) {
            return 0;
        }

        if(ret) {
            // TODO: Treat the return value
            int ret_value = ret->as<int>();
            return ret_value;
        }
        
    } catch (const std::exception& e) {
        uva::console::log_error(e.what());
        return 1;
    }

    return 0;
}