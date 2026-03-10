#pragma once

#include "andy/lang/function.hpp"

namespace andy
{
    namespace lang
    {
        class structure;
        class object;
        class parser;
        // The context of the interpreter execution.
        struct interpreter_context
        {
            // Do NOT use shared_ptr here to avoid circular references.
            andy::lang::object* self;
            std::shared_ptr<andy::lang::structure> cls;
            std::map<std::string_view, std::shared_ptr<andy::lang::object>> variables;
            std::map<std::string_view, std::shared_ptr<andy::lang::function>> functions;
            std::map<std::string_view, std::shared_ptr<andy::lang::inline_function>> inline_functions;
            std::map<std::string_view, std::shared_ptr<andy::lang::structure>> classes;
            
            const andy::lang::parser::ast_node* given_block = nullptr;

            bool has_returned = false;
            bool catching_exception = false;
            std::shared_ptr<andy::lang::object> return_value;
            bool is_block_context = false;
            std::shared_ptr<andy::lang::interpreter_context> lexical_parent = nullptr;
            std::shared_ptr<andy::lang::interpreter_context> given_block_lexical_context = nullptr;
            const andy::lang::parser::ast_node* caller_node = nullptr;
        };
    }; // namespace lang
}; // namespace andy
