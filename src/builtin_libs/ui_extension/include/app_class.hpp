#pragma once

#include "andy/lang/api.hpp"
#include "andy/ui/app.hpp"

class andylang_ui_app : public andy::ui::app
{
public:
    andylang_ui_app(andy::lang::interpreter* __interpreter, std::shared_ptr<andy::lang::object> __app_instance);
protected:
    andy::lang::interpreter* interpreter = nullptr;
    // Do not use shared_ptr here, as it will cause a circular reference and app_instance will never be released.
    andy::lang::object* app_instance;
public:
    virtual void on_init() override;
};
