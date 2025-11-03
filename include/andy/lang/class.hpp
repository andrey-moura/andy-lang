#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <andy/lang/function.hpp>

namespace andy {
    namespace lang {
        class object;
        class method;
        class interpreter;
        using inline_function = std::shared_ptr<andy::lang::object>(*)(andy::lang::interpreter*, std::shared_ptr<andy::lang::object>&, const andy::lang::parser::ast_node&);
        class structure
        {
        public:
            //for user code, use create
            structure(std::string_view __name, std::vector<andy::lang::function> __methods = {});
            ~structure();
        public:
            std::string_view name;
            std::shared_ptr<andy::lang::structure> base;
            std::vector<std::shared_ptr<andy::lang::structure>> deriveds;

            std::map<std::string_view, andy::lang::function> instance_functions;
            std::map<std::string_view, andy::lang::function> class_functions;
            std::map<std::string_view, inline_function> inline_functions;

            std::map<std::string_view, const andy::lang::parser::ast_node*> instance_variables;
            std::map<std::string_view, std::shared_ptr<andy::lang::object>> class_variables;
        public:
            static void create_structures(andy::lang::interpreter* interpreter);
        };
    };
};