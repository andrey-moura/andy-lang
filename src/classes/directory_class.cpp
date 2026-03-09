#include <filesystem>

#include <andy/file.hpp>
#include <andy/lang/api.hpp>
#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_directory_class(andy::lang::interpreter* interpreter)
{
    auto DirectoryClass = std::make_shared<andy::lang::structure>("Dir");

    DirectoryClass->functions["new"] = std::make_shared<andy::lang::function>("new",andy::lang::function_storage_type::class_function, std::initializer_list<std::string>{"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        object->set_native<std::filesystem::path>(std::move(std::filesystem::path(params[0]->as<std::string>())));

        return nullptr;
    });

    DirectoryClass->instance_functions["glob"] = std::make_shared<andy::lang::function>("glob", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"pattern"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::filesystem::path& path = object->as<std::filesystem::path>();
#ifdef _WIN32
        std::string pattern = params[0]->as<std::string>();
        std::replace(pattern.begin(), pattern.end(), '/', '\\');
#else
        const std::string& pattern = params[0]->as<std::string>();
#endif

        std::vector<std::shared_ptr<andy::lang::object>> results;

        std::string_view pattern_view(pattern);
        size_t star_pos = pattern_view.find('*');
        std::string_view dir_part = pattern_view.substr(0, star_pos);
        std::string_view suffix_part = pattern_view.substr(star_pos + 1);

        for(auto& entry : std::filesystem::directory_iterator(path / dir_part)) {
            const std::filesystem::path& entry_path = entry.path();
            if(std::filesystem::is_regular_file(entry_path)) {
                std::filesystem::path relative_entry_path = std::filesystem::relative(entry_path, path);
                std::string entry_path_str = relative_entry_path.string();
                if(entry_path_str.starts_with(dir_part) && entry_path_str.ends_with(suffix_part)) {
                    results.push_back(andy::lang::api::to_object(interpreter, std::move(entry_path)));
                }
            }
        }

        return andy::lang::object::instantiate(interpreter, interpreter->ArrayClass, std::move(results));
    });

    DirectoryClass->functions["exists?"] = std::make_shared<andy::lang::function>("exists?", andy::lang::function_storage_type::class_function, std::initializer_list<std::string>{"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::filesystem::path path;
        std::shared_ptr<andy::lang::object> path_object = params[0];
        if(path_object->cls == interpreter->StringClass) {
            path = path_object->as<std::string>();
        } else if(path_object->cls == interpreter->PathClass) {
            path = path_object->as<std::filesystem::path>();
        } else {
            throw std::runtime_error("invalid path");
        }
        if(std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        } else {
            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        }
    });

    DirectoryClass->functions["create"] = std::make_shared<andy::lang::function>("create",andy::lang::function_storage_type::class_function, std::initializer_list<std::string>{"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::filesystem::path path;
        std::shared_ptr<andy::lang::object> path_object = params[0];
        if(path_object->cls == interpreter->StringClass) {
            path = path_object->as<std::string>();
        } else if(path_object->cls == interpreter->PathClass) {
            path = path_object->as<std::filesystem::path>();
        } else {
            throw std::runtime_error("invalid path");
        }
        std::filesystem::create_directory(path);
        return nullptr;
    });

    DirectoryClass->functions["home"] = std::make_shared<andy::lang::function>("home",andy::lang::function_storage_type::class_function,std::initializer_list<std::string>{}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::filesystem::path path = std::filesystem::path(std::getenv("HOME"));
        if(path.empty()) {
            throw std::runtime_error("Unable to retrieve home directory");
        }
        return andy::lang::object::instantiate(interpreter, interpreter->PathClass, std::move(path));
    });

    
    return DirectoryClass;
}