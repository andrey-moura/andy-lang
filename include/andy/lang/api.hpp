#pragma once

#include <filesystem>

#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/config.hpp>

namespace andy
{
    namespace lang
    {
        namespace api
        {
            /// @brief Executes the code in a file and return the result.
            /// @param path The path to the source code.
            /// @return Returns a shared pointer to the object.
            std::shared_ptr<andy::lang::object> evaluate(std::filesystem::path path);
            /// @brief Creates the object with a value and automatically determines the class.
            /// @tparam T The type of the value.
            /// @param interpreter The interpreter.
            /// @param value The value.
            /// @return Returns a shared pointer to the object.
            template<typename T>
            inline std::shared_ptr<andy::lang::object> to_object(andy::lang::interpreter* interpreter, T value)
            {
                if constexpr(std::is_same_v<T, int>) {
                    auto obj = std::make_shared<andy::lang::object>(interpreter->IntegerClass);
                    obj->set_native<int>(value);
                    return obj;
                } else if constexpr(std::is_same_v<T, std::string>) {
                    auto obj = std::make_shared<andy::lang::object>(interpreter->StringClass);
                    obj->set_native<std::string>(std::move(value));
                    return obj;
                } else if constexpr(std::is_same_v<T, double>) {
                    auto obj = std::make_shared<andy::lang::object>(interpreter->DoubleClass);
                    obj->set_native<double>(value);
                    return obj;
                } else if constexpr(std::is_same_v<T, float>) {
                    auto obj = std::make_shared<andy::lang::object>(interpreter->DoubleClass);
                    obj->set_native<double>(value);
                    return obj;
                }
            }
            /// @brief Adds a class to another class.
            /// @param interpreter The interpreter.
            /// @param cls The class.
            /// @param contained The contained class.
            void contained_class(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::structure> cls, std::shared_ptr<andy::lang::structure> contained);
        };
    };
};