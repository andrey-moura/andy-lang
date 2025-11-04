#include <filesystem>
#include <iostream>

#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/extension.hpp>

std::shared_ptr<andy::lang::structure> create_path_class(andy::lang::interpreter* interpreter)
{
    auto PathClass = std::make_shared<andy::lang::structure>("Path");
    PathClass->class_variables["temp"] = andy::lang::object::create(interpreter, PathClass, std::move(std::filesystem::temp_directory_path()));
        PathClass->instance_functions["new"] = std::make_shared<andy::lang::function>("new",andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            object->set_native<std::filesystem::path>(std::move(std::filesystem::path(params[0]->as<std::string>())));

            return nullptr;
        });

    PathClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string",andy::lang::function_storage_type::instance_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, object->as<std::filesystem::path>().string());
        });

    PathClass->instance_functions["exists?"] = std::make_shared<andy::lang::function>("exists?",andy::lang::function_storage_type::instance_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::filesystem::path& path = object->as<std::filesystem::path>();

            if(std::filesystem::exists(path)) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

    PathClass->instance_functions["/="] = std::make_shared<andy::lang::function>("/=",andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::filesystem::path& path = object->as<std::filesystem::path>();
            path /= params[0]->as<std::string>();

            return nullptr;
        });

    PathClass->instance_functions["/"] = std::make_shared<andy::lang::function>("/",andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::filesystem::path path = object->as<std::filesystem::path>() / params[0]->as<std::string>();
            
            return andy::lang::object::create(interpreter, interpreter->PathClass, std::move(path));
        });

    
        PathClass->class_functions["set_current"] = std::make_shared<andy::lang::function>("set_current",andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::filesystem::path path;
            std::shared_ptr<andy::lang::object> path_object = params[0];

            if(path_object->cls == interpreter->StringClass) {
                path = path_object->as<std::string>();
            } else if(path_object->cls == interpreter->PathClass) {
                path = path_object->as<std::filesystem::path>();
            } else {
                throw std::runtime_error("invalid path");
            }

            std::filesystem::current_path(path);

            return nullptr;
        });


    return PathClass;
}