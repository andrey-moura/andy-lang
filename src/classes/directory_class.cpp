#include <filesystem>

#include <uva/file.hpp>

#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_directory_class(andy::lang::interpreter* interpreter)
{
    auto DirectoryClass = std::make_shared<andy::lang::structure>("Directory");

    DirectoryClass->class_methods = {
        { "exists?", andy::lang::method("exists?", andy::lang::method_storage_type::class_method, {"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
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
        })},
        { "create", andy::lang::method("create",andy::lang::method_storage_type::class_method, {"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
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
        })},
        { "home", andy::lang::method("home",andy::lang::method_storage_type::class_method, {}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::filesystem::path path = std::filesystem::path(std::getenv("HOME"));
            if(path.empty()) {
                throw std::runtime_error("Unable to retrieve home directory");
            }
            return andy::lang::object::instantiate(interpreter, interpreter->PathClass, std::move(path));
        })},
    };
    
    return DirectoryClass;
}