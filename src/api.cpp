#include <andy/lang/api.hpp>
#include <andy/lang/preprocessor.hpp>
#include <andy/lang/lexer.hpp>
#include <andy/lang/parser.hpp>

#include <uva/file.hpp>

extern void create_builtin_libs();

namespace andy
{
    namespace lang
    {
        namespace api
        {
            std::shared_ptr<andy::lang::object> evaluate(std::filesystem::path path)
            {
                std::string source = uva::file::read_all_text<char>(path);

                std::string path_str = path.string();

                andy::lang::lexer l(path_str, source);
        
                andy::lang::preprocessor preprocessor;
                preprocessor.process(path_str, l);
        
                andy::lang::parser p;
                andy::lang::parser::ast_node root_node = p.parse_all(l);

                create_builtin_libs();
        
                andy::lang::interpreter interpreter;
                interpreter.input_file_path = path;
                std::shared_ptr<andy::lang::object> ret = interpreter.execute_all(root_node);
        
                interpreter.start_extensions();
        
                return ret;
            }

            void contained_class(andy::lang::interpreter *interpreter, std::shared_ptr<andy::lang::structure> cls, std::shared_ptr<andy::lang::structure> contained) {
                auto cls_obj = andy::lang::object::create(interpreter, interpreter->ClassClass, contained);
                cls_obj->cls->instance_methods["new"].call(cls_obj);
                cls->class_variables[contained->name] = cls_obj;
            }

            std::shared_ptr<andy::lang::object> call(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::object> object, std::string_view fn) {
                auto method = object->cls->instance_methods.find(fn);

                if(method == object->cls->instance_methods.end()) {
                    throw std::runtime_error("Class " + object->cls->name + " does not have a instance funtion called '" + std::string(fn) + "'");
                }

                andy::lang::function_call call = {
                    method->second.name,
                    object->cls,
                    object,
                    &method->second,
                    {},
                    {},
                    nullptr
                };

                return interpreter->call(call);
            }
        };
    }; // namespace lang
};