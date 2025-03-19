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

#include <andy/lang/api.h>

std::vector<std::shared_ptr<andy::lang::object>> objects;
andy::lang::interpreter* interpreter;

void andy_lang_api_init()
{
    interpreter = new andy::lang::interpreter();
    objects.resize(32);
}

andy_lang_object andy_lang_api_create_object(void* obj)
{
    andy_lang_object object = objects.size();
    if(obj) {
        objects[object] = ((andy::lang::object*)obj)->shared_from_this();
    } else {
        objects[object] = andy::lang::object::create(interpreter, nullptr, nullptr);
    }
    return object;
}

andy_lang_object andy_lang_api_evaluate_file(const char* path)
{
    std::filesystem::path path_fs(path);
    std::shared_ptr<andy::lang::object> obj = andy::lang::api::evaluate(path_fs);
    
    return andy_lang_api_create_object(obj.get());
}

void andy_lang_api_destroy()
{
    objects.clear();
    delete interpreter;
}