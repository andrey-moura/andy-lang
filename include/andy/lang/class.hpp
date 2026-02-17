#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <andy/lang/function.hpp>
#include <andy/lang/interpreter_context.hpp>

namespace andy {
    namespace lang {
        class object;
        class method;
        class interpreter;
        class structure : public std::enable_shared_from_this<structure>, public interpreter_context
        {
        public:
            //for user code, use create
            structure(std::string_view __name, std::vector<andy::lang::function> __methods = {});
            ~structure();
        public:
            std::string_view name;
            std::shared_ptr<andy::lang::structure> base;
            std::vector<std::shared_ptr<andy::lang::structure>> deriveds;

            std::map<std::string_view, std::shared_ptr<andy::lang::function>> instance_functions;
            std::map<std::string_view, const andy::lang::parser::ast_node*> instance_variables;
            std::map<std::string_view, std::shared_ptr<andy::lang::inline_function>> instance_inline_functions;
        public:
            static void create_structures(andy::lang::interpreter* interpreter);
        };
    };
};