#pragma once

#include <filesystem>

#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/config.hpp>
#include <andy/lang/function.hpp>

namespace andy
{
    namespace lang
    {
        namespace api
        {
            /// @brief Executes the code in a file and return the result.
            /// @param path The path to the source code.
            /// @param argc The number of arguments to pass to the code. This is optional and defaults to 0.
            /// @param argv The arguments to pass to the code. This is optional and defaults to nullptr.
            /// @return Returns a shared pointer to the object.
            std::shared_ptr<andy::lang::object> evaluate(std::filesystem::path path, int argc = 0, char** argv = nullptr);
            /// @brief Executes the code in a file and return the result.
            /// @param interpreter The interpreter to execute the code in. If the interpreter is not initialized, it will be initialized with the code in the file.
            /// @param path The path to the source code.
            /// @param argc The number of arguments to pass to the code. This is optional and defaults to 0.
            /// @param argv The arguments to pass to the code. This is optional and defaults to nullptr.
            /// @return Returns a shared pointer to the object.
            std::shared_ptr<andy::lang::object> evaluate(
                andy::lang::interpreter* interpreter,
                std::filesystem::path path,
                int argc = 0,
                char** argv = nullptr
            );
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
            /// @brief Call a function.
            /// @param interpreter The interpreter.
            /// @param object The object.
            /// @param fn The function name.
            /// @return Returns a shared pointer to the object.
            std::shared_ptr<andy::lang::object> call(andy::lang::interpreter* interpreter, std::string_view function_name, std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> positional_params = {});
            /// @brief Creates the object with a value and automatically determines the class.
            /// @tparam T The type of the value.
            /// @param interpreter The interpreter.
            /// @param value The value.
            /// @return Returns a shared pointer to the object.
            template<typename T>
            inline std::shared_ptr<andy::lang::object> to_object(andy::lang::interpreter* interpreter, T value)
            {
                if constexpr(std::is_same_v<T, int>) {
                    auto obj = andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, value);
                    return obj;
                } else if constexpr(std::is_same_v<T, std::string>) {
                    auto obj = andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(value));
                    return obj;
                } else if constexpr(std::is_same_v<T, double>) {
                    auto obj = andy::lang::object::instantiate(interpreter, interpreter->DoubleClass, value);
                    return obj;
                } else if constexpr(std::is_same_v<T, float>) {
                    auto obj = andy::lang::object::instantiate(interpreter, interpreter->FloatClass, value);
                    return obj;
                } else if constexpr(std::is_same_v<T, std::vector<std::shared_ptr<andy::lang::object>>>) {
                    auto obj = andy::lang::object::instantiate(interpreter, interpreter->ArrayClass, std::move(value));
                    return obj;
                } else if constexpr(std::is_same_v<T, andy::lang::dictionary>) {
                    auto obj = andy::lang::object::instantiate(interpreter, interpreter->DictionaryClass, std::move(value));
                    return obj;
                } else if constexpr(std::is_same_v<T, std::map<std::string, std::shared_ptr<andy::lang::object>>> || std::is_same_v<T, std::map<std::string_view, std::shared_ptr<andy::lang::object>>>) {
                    andy::lang::dictionary dict;
                    for(auto& [key, val] : value) {
                        dict.emplace_back(to_object(interpreter, key), std::move(val));
                    }
                    auto obj = andy::lang::object::instantiate(interpreter, interpreter->DictionaryClass, std::move(dict));
                    return obj;
                } else if constexpr(std::is_same_v<T, const char*> || std::is_same_v<T, char*> || std::is_same_v<T, std::string_view>) {
                    return to_object(interpreter, std::string(value));
                }
                else if constexpr(std::is_same_v<T, std::shared_ptr<andy::lang::structure>>) {
                    auto class_object = std::make_shared<andy::lang::object>(interpreter->ClassClass);
                    class_object->set_native<std::shared_ptr<andy::lang::structure>>(std::move(value));
                    class_object->initialize(interpreter);

                    return class_object;
                } else if constexpr(std::is_same_v<T, bool>) {
                    if(value) {
                        return andy::lang::object::instantiate(interpreter, interpreter->TrueClass);
                    } else {
                        return andy::lang::object::instantiate(interpreter, interpreter->FalseClass);
                    }
                } else if constexpr(std::is_same_v<T, std::shared_ptr<andy::lang::function>>) {
                    auto function_object = andy::lang::object::instantiate(
                        interpreter,
                        interpreter->FunctionClass,
                        std::move(value)
                    );
                    return function_object;
                } else if constexpr(std::is_same_v<T, std::filesystem::path>) {
                    auto obj = andy::lang::object::create(interpreter, interpreter->PathClass, std::move(value));
                    obj->initialize(interpreter);
                    return obj;
                } else if constexpr(std::is_same_v<T, std::vector<std::string_view>>) {
                    std::vector<std::shared_ptr<andy::lang::object>> arr;
                    for(auto& str : value) {
                        arr.push_back(to_object(interpreter, str));
                    }
                    auto obj = andy::lang::object::instantiate(interpreter, interpreter->ArrayClass, std::move(arr));
                    return obj;
                 }
                else {
                    throw std::runtime_error("Unsupported type for to_object: " + std::string(typeid(T).name()));
                }
            }
            /// @brief Checks if the object is present (not null, not false, not empty string, etc.).
            /// @param interpreter The interpreter.
            /// @param obj The object to check.
            /// @return Returns true if the object is present, false otherwise.
            bool is_present(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::object> obj);
            /// @brief Checks if the object is truthy (not null and not false).
            /// @param interpreter The interpreter.
            /// @param obj The object to check.
            /// @return Returns true if the object is truthy, false otherwise.
            bool is_truthy(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::object> obj);
            /// @brief Adds a class to another class.
            /// @param interpreter The interpreter.
            /// @param cls The class.
            /// @param contained The contained class.
            void contained_class(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::structure> cls, std::shared_ptr<andy::lang::structure> contained);
        };
    };
};