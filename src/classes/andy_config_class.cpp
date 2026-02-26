#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/extension.hpp>
#include <andy/lang/config.hpp>

#include <filesystem>

std::shared_ptr<andy::lang::structure> create_andy_config_class(andy::lang::interpreter* interpreter)
{
    auto AndyConfigClass = std::make_shared<andy::lang::structure>("AndyConfig");

    AndyConfigClass->variables["src_dir"]  = andy::lang::object::create(interpreter, interpreter->PathClass, std::move(andy::lang::config::src_dir()));
    AndyConfigClass->variables["version"]  = andy::lang::object::create(interpreter, interpreter->StringClass, std::string(andy::lang::config::version));
    AndyConfigClass->variables["build"]    = andy::lang::object::create(interpreter, interpreter->StringClass, std::string(andy::lang::config::build));
    AndyConfigClass->variables["cpp"]      = andy::lang::object::create(interpreter, interpreter->StringClass, std::string(andy::lang::config::cpp));
    AndyConfigClass->variables["compiler"] = andy::lang::object::create(interpreter, interpreter->StringClass, std::string(andy::lang::config::compiler));
    return AndyConfigClass;
}