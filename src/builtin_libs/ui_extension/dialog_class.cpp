#include <uva/file.hpp>
#include <andy/ui/dialog.hpp>

#include <uva/widgets.hpp>
#include <uva/xml.hpp>

#include "toplevel_class.hpp"

std::shared_ptr<andy::lang::structure> create_dialog_class(andy::lang::interpreter* interpreter)
{
    return andy::lang::toplevel_class<andy::ui::dialog>::create(interpreter, "Dialog");
}