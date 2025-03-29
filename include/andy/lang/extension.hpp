#pragma once

#include <string>
#include <vector>

#include <uva.hpp>
#include <andy/lang/parser.hpp>

#ifdef _WIN32
    #define ANDY_EXTENSION(name) \
    extern "C" __declspec(dllexport) andy::lang::extension* create_extension()\
    {\
        return new name ();\
    }
#else
    #define ANDY_EXTENSION(name) \
    extern "C" andy::lang::extension* create_extension()\
    {\
        return new name ();\
    }
#endif

namespace andy {
    namespace lang {
        class interpreter;
        class extension{
        protected:
            ANDY_EXPORT_SYMBOL extension(const std::string& name);
        public:
            static void add_builtin(std::shared_ptr<andy::lang::extension> extension);
            static void import(andy::lang::interpreter* interpreter, std::string_view module);
        public:
            const std::string& name() const { return m_name; }
        protected:
            static std::map<std::string_view, std::shared_ptr<andy::lang::extension>> builtins;
        public:
            /// @brief Load the extension in the interpreter. Good for loading your custom classes. Called BEFORE the source code is executed.
            virtual void load(andy::lang::interpreter* interpreter) { }
            virtual void start(andy::lang::interpreter* interpreter) { }
        private:
            std::string m_name;
        };
    }
};