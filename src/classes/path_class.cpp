#include <andy/lang/lang.hpp>

#include <andy/lang/interpreter.hpp>
#include <andy/lang/extension.hpp>

#include <filesystem>
#include <iostream>

std::shared_ptr<andy::lang::structure> create_path_class(andy::lang::interpreter* interpreter)
{
    auto PathClass = std::make_shared<andy::lang::structure>("Path");
    PathClass->class_variables["temp"] = andy::lang::object::create(interpreter, PathClass, std::move(std::filesystem::temp_directory_path()));
    PathClass->methods = {
        {"new", andy::lang::method("new",andy::lang::method_storage_type::instance_method, {"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            object->set_native<std::filesystem::path>(std::move(std::filesystem::path(params[0]->as<std::string>())));

            return nullptr;
        })},
        {"to_string", andy::lang::method("to_string",andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, object->as<std::filesystem::path>().string());
        })},
        {"exists?", andy::lang::method("exists?",andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::filesystem::path& path = object->as<std::filesystem::path>();

            if(std::filesystem::exists(path)) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        })},
        {"set_current", andy::lang::method("set_current",andy::lang::method_storage_type::instance_method, {"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
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
        })},
        {"/=", andy::lang::method("/=",andy::lang::method_storage_type::instance_method, {"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::filesystem::path& path = object->as<std::filesystem::path>();
            path /= params[0]->as<std::string>();

            return nullptr;
        })},
        {"/", andy::lang::method("/",andy::lang::method_storage_type::instance_method, {"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::filesystem::path path = object->as<std::filesystem::path>() / params[0]->as<std::string>();
            
            return andy::lang::object::create(interpreter, interpreter->PathClass, std::move(path));
        })},
    };
    
    return PathClass;
}