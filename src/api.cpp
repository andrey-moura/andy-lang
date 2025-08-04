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
                auto cls_obj = to_object(interpreter, contained);
                cls->class_variables[contained->name] = cls_obj;
            }

            std::shared_ptr<andy::lang::object> call(andy::lang::interpreter *interpreter, andy::lang::function_call __call) {
                auto method = __call.object->cls->instance_methods.find(__call.name);

                if(method == __call.object->cls->instance_methods.end()) {
                    throw std::runtime_error("Class " + std::string(__call.object->cls->name) + " does not have an instance function called '" + std::string(__call.name) + "'");
                }

                __call.method = &method->second;

                return interpreter->call(__call);
            }
        };
    }; // namespace lang
};