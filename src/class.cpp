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
    interpreter->load(create_false_class(interpreter));
    interpreter->load(create_true_class(interpreter));
    interpreter->load(create_string_class(interpreter));
    interpreter->load(create_integer_class(interpreter));
    interpreter->load(create_double_class(interpreter));
    interpreter->load(create_float_class(interpreter));
    interpreter->load(create_file_class(interpreter));
    interpreter->load(create_std_class(interpreter));
    interpreter->load(create_array_class(interpreter));
    interpreter->load(create_null_class(interpreter));
    interpreter->load(create_dictionary_class(interpreter));
    interpreter->load(create_system_class(interpreter));
    interpreter->load(create_path_class(interpreter));
    interpreter->load(create_andy_config_class(interpreter));
    interpreter->load(create_class_class(interpreter));
}

andy::lang::structure::structure(const std::string& __name, std::vector<andy::lang::method> __methods)
    : name(__name)
{
    uva::console::log_debug("{}#Class created", name);
}

andy::lang::structure::~structure()
{
    uva::console::log_debug("{}#Class destroyed", name);
}