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
            /// @brief Convert or cast the object to a specific type.
            /// @tparam T The type to convert to.
            /// @param interpreter The interpreter.
            /// @param object The object to convert.
            /// @return Returns a shared pointer to the object.
            template<typename T>
            T cast_object_to(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::object>&& object)
            {
                if constexpr(std::is_same_v<T, std::string>) {
                    if(object->cls == interpreter->StringClass) {
                        return object->as<std::string>();
                    }
                    throw std::runtime_error("Cannot cast " + std::string(object->cls->name) + " to string");
                } else if constexpr(std::is_same_v<T, bool>) {
                    if(object->cls == interpreter->TrueClass) {
                        return true;
                    } else if(object->cls == interpreter->FalseClass) {
                        return false;
                    }
                    throw std::runtime_error("Cannot cast " + std::string(object->cls->name) + " to bool");
                } else {
                    throw std::runtime_error("Unsupported type for to_object: " + std::string(typeid(T).name()));
                }
            }
            /// @brief Call a function.
            /// @param interpreter The interpreter.
            /// @param object The object.
            /// @param fn The function name.
            /// @return Returns a shared pointer to the object.
            std::shared_ptr<andy::lang::object> call(andy::lang::interpreter* interpreter, andy::lang::function_call __call);
            template<typename T>
            T call(andy::lang::interpreter* interpreter, andy::lang::function_call __call)
            {
                std::shared_ptr<andy::lang::object> obj = call(interpreter, std::move(__call));

                return cast_object_to<T>(interpreter, std::move(obj));
            }
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
                } else if constexpr(std::is_same_v<T, std::vector<std::shared_ptr<andy::lang::object>>>) {
                    auto obj = std::make_shared<andy::lang::object>(interpreter->ArrayClass);
                    obj->set_native<std::vector<std::shared_ptr<andy::lang::object>>>(std::move(value));
                    return obj;
} else if constexpr(std::is_same_v<T, andy::lang::dictionary>) {
                    auto obj = std::make_shared<andy::lang::object>(interpreter->DictionaryClass);
                    obj->set_native<andy::lang::dictionary>(std::move(value));
                    return obj;
                } else if constexpr(std::is_same_v<T, const char*> || std::is_same_v<T, char*> || std::is_same_v<T, std::string_view>) {
                    return to_object(interpreter, std::string(value));
                }
                else if constexpr(std::is_same_v<T, std::shared_ptr<andy::lang::structure>>) {
                    auto class_object = andy::lang::object::create(interpreter, interpreter->ClassClass, std::move(value));
                    class_object->cls->instance_methods["new"].call(class_object);
                    return class_object;
                } else if constexpr(std::is_same_v<T, bool>) {
                    if(value) {
                        return std::make_shared<andy::lang::object>(interpreter->TrueClass);
                    } else {
                        return std::make_shared<andy::lang::object>(interpreter->FalseClass);
                    }
                }
                else {
                    throw std::runtime_error("Unsupported type for to_object: " + std::string(typeid(T).name()));
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