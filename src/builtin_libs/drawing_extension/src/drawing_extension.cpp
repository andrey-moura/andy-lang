#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "andy/lang/api.hpp"
#include "andy/lang/extension.hpp"
#include "andy/file.hpp"
#include "andy/xml.hpp"

extern std::shared_ptr<andy::lang::structure> create_window_class(andy::lang::interpreter* interpreter);
extern std::shared_ptr<andy::lang::structure> create_dialog_class(andy::lang::interpreter* interpreter);
extern std::shared_ptr<andy::lang::structure> create_page_class(andy::lang::interpreter* interpreter);

// extern std::shared_ptr<andy::lang::structure> create_xml_page_class(andy::lang::interpreter* interpreter);

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
        auto drawing_class = std::make_shared<andy::lang::structure>("Drawing");

        auto classes = {
            create_window_class(interpreter),
            create_dialog_class(interpreter),
            create_page_class(interpreter),
        };
    
        for (auto& cls : classes) {
            andy::lang::api::contained_class(interpreter, drawing_class, cls);
        }

        interpreter->load(drawing_class);
    }
};

std::shared_ptr<andy::lang::extension> create_drawing_extension()
{
    return std::make_shared<drawing_extension>();
}