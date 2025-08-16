#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "andy/lang/api.hpp"
#include "andy/lang/extension.hpp"
#include "andy/file.hpp"
#include "andy/xml.hpp"

extern std::shared_ptr<andy::lang::structure> create_app_class(andy::lang::interpreter* interpreter);

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
        auto UIClass = std::make_shared<andy::lang::structure>("UI");

        auto classes = {
            create_app_class(interpreter),
        };
    
        for (auto& cls : classes) {
            andy::lang::api::contained_class(interpreter, UIClass, cls);
        }

        interpreter->load(UIClass);
    }
};

std::shared_ptr<andy::lang::extension> create_ui_extension()
{
    return std::make_shared<ui_extension>();
}