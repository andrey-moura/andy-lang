#include <filesystem>

#include <uva/file.hpp>

#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_file_class(andy::lang::interpreter* interpreter)
{
    auto FileClass = std::make_shared<andy::lang::structure>("File");

    FileClass->class_methods = {
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
            if(std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            } else {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }
        })},
        { "read", andy::lang::method("read",andy::lang::method_storage_type::class_method, {"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::filesystem::path path;
            std::shared_ptr<andy::lang::object> path_object = params[0];
            if(path_object->cls == interpreter->StringClass) {
                path = path_object->as<std::string>();
            } else if(path_object->cls == interpreter->PathClass) {
                path = path_object->as<std::filesystem::path>();
            } else {
                throw std::runtime_error("invalid path");
            }
            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(uva::file::read_all_text<char>(path)));
        })},
        { "read_all_lines", andy::lang::method("read_all_lines",andy::lang::method_storage_type::class_method, {"path"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& input_path = params[0]->as<std::string>();
            std::filesystem::path path = std::filesystem::absolute(input_path);

            if(!std::filesystem::exists(path)) {
                throw std::runtime_error("file '" + path.string() + "' does not exist");
            }

            std::vector<std::string> file = andy::file::read_all_lines<char>(path);

            std::vector<std::shared_ptr<andy::lang::object>> lines;

            for(auto& line : file) {
                lines.push_back(andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(line)));
            }

            return andy::lang::object::instantiate(interpreter, interpreter->ArrayClass, std::move(lines));
        })},
    };
    
    return FileClass;
}