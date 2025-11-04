#include "andy/lang/lang.hpp"
#include "andy/lang/interpreter.hpp"

#include <andy/console.hpp>

extern std::shared_ptr<andy::lang::structure> create_false_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_andy_config_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_array_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_class_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_dictionary_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_directory_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_double_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_false_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_file_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_float_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_integer_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_null_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_path_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_std_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_string_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_system_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_true_class(andy::lang::interpreter*);
extern std::shared_ptr<andy::lang::structure> create_function_class(andy::lang::interpreter*);

void andy::lang::structure::create_structures(andy::lang::interpreter* interpreter)
{
    interpreter->load(interpreter->FalseClass       = create_false_class       (interpreter) );
    interpreter->load(interpreter->TrueClass        = create_true_class        (interpreter) );
    interpreter->load(interpreter->StringClass      = create_string_class      (interpreter) );
    interpreter->load(interpreter->IntegerClass     = create_integer_class     (interpreter) );
    interpreter->load(interpreter->DoubleClass      = create_double_class      (interpreter) );
    interpreter->load(interpreter->FloatClass       = create_float_class       (interpreter) );
    interpreter->load(interpreter->FileClass        = create_file_class        (interpreter) );
    interpreter->load(interpreter->StdClass         = create_std_class         (interpreter) );
    interpreter->load(interpreter->ArrayClass       = create_array_class       (interpreter) );
    interpreter->load(interpreter->NullClass        = create_null_class        (interpreter) );
    interpreter->load(interpreter->DictionaryClass  = create_dictionary_class  (interpreter) );
    interpreter->load(interpreter->SystemClass      = create_system_class      (interpreter) );
    interpreter->load(interpreter->PathClass        = create_path_class        (interpreter) );
    interpreter->load(interpreter->AndyConfigClass  = create_andy_config_class (interpreter) );
    interpreter->load(interpreter->ClassClass       = create_class_class       (interpreter) );
    interpreter->load(interpreter->FunctionClass    = create_function_class    (interpreter) );

    // These are not named on Interpreter because they are not used too often
    // Some of the one which are named should be moved to here soon.
    interpreter->load(create_directory_class(interpreter));
}

andy::lang::structure::structure(std::string_view __name, std::vector<andy::lang::function> __methods)
    : name(std::move(__name))
{
    for(auto& method : __methods) {
        functions[method.name] = std::make_shared<andy::lang::function>(std::move(method));
    }

    andy::console::log_debug("{}#Class created", name);
}

andy::lang::structure::~structure()
{
    andy::console::log_debug("{}#Class destroyed", name);
}