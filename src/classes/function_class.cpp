#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

#include "add_operators.hpp"

std::shared_ptr<andy::lang::structure> create_function_class(andy::lang::interpreter* interpreter)
{
    std::shared_ptr<andy::lang::structure> functionClass = std::make_shared<andy::lang::structure>("Function");

    functionClass->instance_functions = {

    };

    andy::lang::add_operators<float>(functionClass, interpreter);

    return functionClass;
}