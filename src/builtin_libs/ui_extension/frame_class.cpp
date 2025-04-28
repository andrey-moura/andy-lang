#include <andy/ui/frame.hpp>

#include <uva/widgets.hpp>
#include <uva/xml.hpp>

#include "toplevel_class.hpp"

std::shared_ptr<andy::lang::structure> create_frame_class(andy::lang::interpreter* interpreter)
{
    return andy::lang::toplevel_class<andy::ui::frame>::create(interpreter, "Frame");
}