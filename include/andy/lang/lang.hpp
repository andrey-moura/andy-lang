#pragma once

#include <andy/lang/class.hpp>
#include <andy/lang/method.hpp>
#include <andy/lang/object.hpp>

namespace andy {
    namespace lang {
        // This is temporary. It will be removed in the future.
        using dictionary = std::vector<std::pair<std::shared_ptr<andy::lang::object>, std::shared_ptr<andy::lang::object>>>;
    };
};