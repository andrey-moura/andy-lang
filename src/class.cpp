#include <andy/lang/class.hpp>

#include <uva/console.hpp>

#include "false_class.cpp"
#include "true_class.cpp"
#include "string_class.cpp"
#include "integer_class.cpp"
#include "double_class.cpp"
#include "float_class.cpp"
#include "file_class.cpp"
#include "std_class.cpp"
#include "array_class.cpp"
#include "null_class.cpp"
#include "dictionary_class.cpp"
#include "system_class.cpp"
#include "path_class.cpp"
#include "andy_config_class.cpp"
#include "class_class.cpp"

andy::lang::structure::structure(const std::string& __name, std::vector<andy::lang::method> __methods)
    : name(__name)
{
    uva::console::log_debug("{}#Class created", name);
}

andy::lang::structure::~structure()
{
    uva::console::log_debug("{}#Class destroyed", name);
}