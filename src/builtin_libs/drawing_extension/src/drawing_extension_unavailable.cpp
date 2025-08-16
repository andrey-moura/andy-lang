#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "andy/lang/api.hpp"
#include "andy/lang/extension.hpp"

class drawing_extension : public andy::lang::extension
{
public:
    drawing_extension()
        : andy::lang::extension("drawing")
    {
    }
protected:
    std::shared_ptr<andy::lang::object> application_instance;
public:
    virtual void load(andy::lang::interpreter* interpreter) override
    {
        throw std::runtime_error("This distribution of Andy was built without drawing support. Please, recompile it with the drawing support or download the official release.");
    }
};

std::shared_ptr<andy::lang::extension> create_drawing_extension()
{
    return std::make_shared<drawing_extension>();
}