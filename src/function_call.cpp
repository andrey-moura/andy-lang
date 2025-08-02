#include <andy/lang/method.hpp>
#include <andy/lang/object.hpp>

namespace andy {
    namespace lang {
        function_call::function_call(
            std::string_view __name,
            std::shared_ptr<andy::lang::object> __object,
            std::vector<std::shared_ptr<andy::lang::object>> __positional_params,
            std::map<std::string, std::shared_ptr<andy::lang::object>> __named_params
        ) : name(std::move(__name)), object(std::move(__object)), cls(__object ? __object->cls : nullptr), positional_params(std::move(__positional_params)), named_params(std::move(__named_params))
        {

        }

        function_call::function_call(
            std::string_view __name,
            std::shared_ptr<andy::lang::structure> __cls,
            std::shared_ptr<andy::lang::object> __object,
            const andy::lang::method* method,
            std::vector<std::shared_ptr<andy::lang::object>> __positional_params,
            std::map<std::string, std::shared_ptr<andy::lang::object>> __named_params,
            const andy::lang::parser::ast_node* __given_block
        ) : name(std::move(__name)), cls(std::move(__cls)), object(std::move(__object)), method(method), positional_params(std::move(__positional_params)), named_params(std::move(__named_params)), given_block(__given_block)
        {

        }
    };
};