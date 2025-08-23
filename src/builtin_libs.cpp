#include <memory>

#include "andy/lang/extension.hpp"

extern std::shared_ptr<andy::lang::extension> create_tests_extension();
extern std::shared_ptr<andy::lang::extension> create_ui_extension();
extern std::shared_ptr<andy::lang::extension> create_drawing_extension();
extern std::shared_ptr<andy::lang::extension> create_net_extension();

void create_builtin_libs() {
    andy::lang::extension::add_builtin(create_tests_extension());
    andy::lang::extension::add_builtin(create_ui_extension());
    andy::lang::extension::add_builtin(create_drawing_extension());
    andy::lang::extension::add_builtin(create_net_extension());
}