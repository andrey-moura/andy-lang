#pragma once

#include <string_view>
#include <string>
#include <andy/format.hpp>

namespace andy
{
    namespace lang
    {
        namespace error
        {
            [[noreturn]]
            void internal(std::string_view message);

            template<class... Args>
            [[noreturn]]
            void internal(std::string_view fmt, Args... args)
            {
                std::string msg = std::vformat(fmt, std::make_format_args(args...));
                andy::lang::error::internal(std::string_view(msg));
            }
        };
    };
};