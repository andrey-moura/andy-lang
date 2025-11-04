#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include <andy/lang/api.hpp>
#include <andy/lang/extension.hpp>

extern std::shared_ptr<andy::lang::structure> create_net_class(andy::lang::interpreter* interpreter);
extern std::shared_ptr<andy::lang::structure> create_http_class(andy::lang::interpreter* interpreter);

class net_extension : public andy::lang::extension
{
public:
    net_extension() : andy::lang::extension("net")
    {
    }
public:
    virtual void load(andy::lang::interpreter* interpreter) override
    {
        auto net_class = create_net_class(interpreter);
        auto http_class = create_http_class(interpreter);

        andy::lang::api::contained_class(interpreter, net_class, http_class);

        interpreter->load(net_class);
    }
};

std::shared_ptr<andy::lang::extension> create_net_extension()
{
    return std::make_shared<net_extension>();
}