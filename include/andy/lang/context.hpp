#pragma once

#include <map>
#include <memory>
#include <string_view>

#include <andy/lang/parser.hpp>

namespace andy
{
    namespace lang
    {
        class structure;
        class object;
        class function;
        class interpreter;
        
        using inline_function = std::shared_ptr<andy::lang::object>(*)(andy::lang::interpreter*, std::shared_ptr<andy::lang::object>&, const andy::lang::parser::ast_node&);
        
        // The base context for both classes and objects
        class context
        {
        public:
            std::shared_ptr<andy::lang::structure> cls;
            std::shared_ptr<andy::lang::object> self;
            std::map<std::string_view, std::shared_ptr<andy::lang::object>> variables;
            std::map<std::string_view, std::shared_ptr<andy::lang::function>> functions;
            std::map<std::string_view, std::shared_ptr<inline_function>> inline_functions;
            const andy::lang::parser::ast_node* given_block = nullptr;

            bool has_returned = false;
            std::shared_ptr<andy::lang::object> return_value;
            bool inherited = false;
        };
    }
}
