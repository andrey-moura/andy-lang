#include <iostream>

#include "andy/lang/api.hpp"

#include "andy/drawing/page.hpp"

std::shared_ptr<andy::lang::structure> create_page_class(andy::lang::interpreter* interpreter)
{
    auto page_class = std::make_shared<andy::lang::structure>("Page");
    return page_class;
}