#include "andy/lang/error.hpp"
#include <iostream>
#include <cstdlib>

void andy::lang::error::internal(std::string_view message)
{
    std::cerr
        << "ANDY LANGUAGE INTERNAL ERROR!!!" << std::endl
        << std::endl
        << message << std::endl << std::endl
        << "You have encountered an internal error in the Andy Language." << std::endl
        << "This is likely a bug in the language itself." << std::endl
        << "Please report this issue to the Andy Language developers at" << std::endl
        << "https://github.com/Moonslate/andy-lang/issues"
        << std::endl;

    std::abort();
}