#pragma once

#include <andy/lang/lexer.hpp>

#include <string>
#include <vector>
#include <string_view>
#include <functional>
#include <filesystem>
#include <regex>

namespace andy
{
    namespace lang
    {
        class preprocessor
        {
        public:
            preprocessor();
            ~preprocessor();
        public:
            void process(const std::filesystem::path& __file_name, andy::lang::lexer& __lexer);
        public:
            void process_token(const std::filesystem::path& __file_name, andy::lang::lexer& __lexer, andy::lang::lexer::token& token);
            void process_include(const std::filesystem::path& __file_name, andy::lang::lexer& __lexer);
            void process_compile(const std::filesystem::path& __file_name, andy::lang::lexer& __lexer);
            void process_if(const std::filesystem::path& __file_name, andy::lang::lexer& __lexer);
        };
    };
};