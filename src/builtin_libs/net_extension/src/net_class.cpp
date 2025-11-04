#include <andy/lang/api.hpp>

std::shared_ptr<andy::lang::structure> create_net_class(andy::lang::interpreter* interpreter)
{
    auto net_class = std::make_shared<andy::lang::structure>("Net");
    return net_class;
}