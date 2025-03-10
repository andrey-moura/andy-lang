#include <andy/lang/class.hpp>

#include <uva/console.hpp>

#include "classes/false_class.cpp"
#include "classes/true_class.cpp"
#include "classes/string_class.cpp"
#include "classes/integer_class.cpp"
#include "classes/double_class.cpp"
#include "classes/float_class.cpp"
#include "classes/file_class.cpp"
#include "classes/std_class.cpp"
#include "classes/array_class.cpp"
#include "classes/null_class.cpp"
#include "classes/dictionary_class.cpp"
#include "classes/system_class.cpp"
#include "classes/path_class.cpp"
#include "classes/andy_config_class.cpp"
#include "classes/class_class.cpp"

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
}

andy::lang::structure::structure(const std::string& __name, std::vector<andy::lang::method> __methods)
    : name(__name)
{
    for(auto& method : __methods) {
        if(method.storage_type == method_storage_type::class_method) {
            class_methods[method.name] = std::move(method);
        } else {
            instance_methods[method.name] = std::move(method);
        }
    }

    uva::console::log_debug("{}#Class created", name);
}

andy::lang::structure::~structure()
{
    uva::console::log_debug("{}#Class destroyed", name);
}