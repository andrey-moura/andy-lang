#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "andy/lang/api.hpp"
#include "andy/lang/extension.hpp"

class ui_extension : public andy::lang::extension
{
public:
    ui_extension()
        : andy::lang::extension("ui")
    {
    }
protected:
    std::shared_ptr<andy::lang::object> application_instance;
public:
    virtual void load(andy::lang::interpreter* interpreter) override
    {
        throw std::runtime_error("This distribution of Andy was built without UI support. Please, recompile it with the UI support or download the official release.");
    }

    virtual void start(andy::lang::interpreter* interpreter) override
    {
    
    }
};

std::shared_ptr<andy::lang::extension> create_ui_extension()
{
    return std::make_shared<ui_extension>();
}