#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "andy/lang/api.hpp"
#include "andy/lang/extension.hpp"

class net_extension : public andy::lang::extension
{
public:
    net_extension()
        : andy::lang::extension("net")
    {
    }
protected:
    std::shared_ptr<andy::lang::object> application_instance;
public:
    virtual void load(andy::lang::interpreter* interpreter) override
    {
        throw std::runtime_error("This distribution of Andy was built without networking support. Please, recompile it with the networking support or download the official release.");
    }
};

std::shared_ptr<andy::lang::extension> create_net_extension()
{
    return std::make_shared<net_extension>();
}