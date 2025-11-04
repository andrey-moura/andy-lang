#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <andy/lang/function.hpp>
#include <andy/lang/context.hpp>

namespace andy {
    namespace lang {
        class object;
        class method;
        class interpreter;
        class structure : public context
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
            std::map<std::string_view, std::shared_ptr<andy::lang::function>> class_functions;

            std::map<std::string_view, const andy::lang::parser::ast_node*> instance_variables;
            std::map<std::string_view, std::shared_ptr<andy::lang::object>> class_variables;
        public:
            static void create_structures(andy::lang::interpreter* interpreter);
        };
    };
};