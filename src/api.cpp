#include <andy/lang/api.hpp>
#include <andy/lang/preprocessor.hpp>
#include <andy/lang/lexer.hpp>
#include <andy/lang/parser.hpp>

#include <uva/file.hpp>

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
        
                andy::lang::interpreter interpreter;
                interpreter.input_file_path = path;
                std::shared_ptr<andy::lang::object> ret = interpreter.execute_all(root_node);
        
                interpreter.start_extensions();
        
                return ret;
            }
        };
    };
};