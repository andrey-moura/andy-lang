#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <uva/var.hpp>

#include <andy/lang/method.hpp>

namespace andy {
    namespace lang {
        class object;
        class method;
        class structure
        {
        public:
            //for user code, use create
            structure(const std::string& __name, std::vector<andy::lang::method> __methods = {});
            ~structure();
        public:
            std::string name;
            std::map<std::string, andy::lang::method> methods;
            std::shared_ptr<andy::lang::structure> base;
            std::vector<std::shared_ptr<andy::lang::structure>> deriveds;
            std::map<std::string, std::shared_ptr<andy::lang::structure>> instance_variables;
            std::map<std::string, std::shared_ptr<andy::lang::object>> class_variables;
            std::string source_content;

            var(*object_to_var)(std::shared_ptr<const andy::lang::object> obj) = nullptr;

            std::shared_ptr<andy::lang::object> call(const andy::lang::method& method, const var& params= null);
            std::shared_ptr<andy::lang::object> call(const std::string& method, const var& params = null)
            {
                return call(methods[method], params);
            }
        };
    };
};