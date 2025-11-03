#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include <andy/lang/parser.hpp>

namespace andy {
    namespace lang {
        class object;
        class structure;
        class function;
        enum class function_storage_type {
            instance_function,
            class_function,
        };
        struct fn_parameter
        {
            fn_parameter() = default;
            fn_parameter(std::string_view __name);
            fn_parameter(std::string __name, bool __named, const andy::lang::parser::ast_node* __default_value_node)
                : name(std::move(__name)), named(__named), default_value_node(__default_value_node), has_default_value(true) {
            }
            std::string name;
            bool has_default_value = false;
            const andy::lang::parser::ast_node* default_value_node = nullptr;
            bool named = false;
        };
        struct function_call
        {
            function_call() = default;
            function_call(
                std::string_view __name,
                std::shared_ptr<andy::lang::object> __object = nullptr,
                std::vector<std::shared_ptr<andy::lang::object>> __positional_params = {},
                std::map<std::string, std::shared_ptr<andy::lang::object>> __named_params = {}
            );
            function_call(
                std::string_view __name,
                std::shared_ptr<andy::lang::structure> __cls,
                std::shared_ptr<andy::lang::object> __object,
                const andy::lang::function* method = nullptr,
                std::vector<std::shared_ptr<andy::lang::object>> __positional_params = {},
                std::map<std::string, std::shared_ptr<andy::lang::object>> __named_params = {},
                const andy::lang::parser::ast_node* __given_block = nullptr
            );
            std::string_view                                           name;
            std::shared_ptr<andy::lang::structure>                     cls;
            std::shared_ptr<andy::lang::object>                        object;
            const andy::lang::function*                                  method = nullptr;
            std::vector<std::shared_ptr<andy::lang::object>>           positional_params;
            std::map<std::string, std::shared_ptr<andy::lang::object>> named_params = {};
            const andy::lang::parser::ast_node*                        given_block = nullptr;
        };
        class function
        {
        public:
            std::string_view name;
            andy::lang::parser::ast_node block_ast;
            function_storage_type storage_type;
            std::vector<fn_parameter> positional_params;
            std::vector<fn_parameter> named_params;
            std::function<std::shared_ptr<andy::lang::object>(andy::lang::function_call&)> native_function;

            function() = default;

            function(std::string_view __name, function_storage_type __storage_type, std::vector<std::string> __params, andy::lang::parser::ast_node __block)
                : name(__name), block_ast(std::move(__block)), storage_type(__storage_type) {
                init_params(__params);
            };

            function(std::string_view __name, function_storage_type __storage_type, std::vector<fn_parameter> __params, andy::lang::parser::ast_node __block)
                : name(__name), block_ast(std::move(__block)), storage_type(__storage_type) {
                for(auto& param : __params) {
                    if(param.named) {
                        named_params.push_back(std::move(param));
                    } else {
                        positional_params.push_back(std::move(param));
                    }
                }
            };

            function(std::string_view name, function_storage_type __storage_type, std::initializer_list<std::string> __params, std::function<std::shared_ptr<andy::lang::object>(std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params)> fn)
                : name(name), storage_type(__storage_type) {
                init_params(__params);

                native_function = [fn](andy::lang::function_call& call) {
                    return fn(call.object, call.positional_params);
                };
            }

            function(std::string_view name, function_storage_type __storage_type, std::vector<fn_parameter> __params, std::function<std::shared_ptr<andy::lang::object>(std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> positional_params, std::map<std::string, std::shared_ptr<andy::lang::object>> named_params)> fn)
                : name(name), storage_type(__storage_type) {
                positional_params.reserve(__params.size());
                named_params.reserve(__params.size());

                for(auto& param : __params) {
                    if(param.named) {
                        named_params.push_back(std::move(param));
                    } else {
                        positional_params.push_back(std::move(param));
                    }
                }

                native_function = [fn](andy::lang::function_call& call) {
                    return fn(call.object, call.positional_params, call.named_params);
                };
            }

            function(std::string_view name, function_storage_type __storage_type, std::function<std::shared_ptr<andy::lang::object>(std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params)> fn)
                : name(name), storage_type(__storage_type) {
                native_function = [fn](andy::lang::function_call& call) {
                    return fn(call.object, call.positional_params);
                };
            }

            function(std::string_view name, function_storage_type __storage_type, std::vector<std::string> __params, std::function<std::shared_ptr<andy::lang::object>(andy::lang::function_call& call)> fn)
                : name(name), storage_type(__storage_type), native_function(fn) {
                init_params(__params);
            }

            std::shared_ptr<andy::lang::object> call(std::shared_ptr<andy::lang::object> o);
            std::shared_ptr<andy::lang::object> call(andy::lang::structure* c);

            protected:
                void init_params(std::vector<std::string> __params);
        };
    }
}