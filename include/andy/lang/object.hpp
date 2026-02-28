#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "andy/lang/function.hpp"
#include "andy/lang/class.hpp"
#include "andy/lang/interpreter_context.hpp"

namespace andy
{
    namespace lang {
        class object;
        class structure;
        class interpreter;
        constexpr size_t max_native_size = 40;
        class object : public std::enable_shared_from_this<object>, public interpreter_context
        {
        public:
            object(std::shared_ptr<andy::lang::structure> c);
            ~object();
        public:
            std::shared_ptr<object> base_instance = nullptr;
            std::shared_ptr<object> derived_instance = nullptr;
            // #ifdef __UVA_DEBUG__
            // andy::lang::object* debug_object = this;

            // __attribute__((noinline)) __attribute__((used)) std::string debug_string()
            // {
            //     return to_var().to_s();
            // }
            // #endif
        protected:
            // A pointer to the native object
            void* native_ptr = nullptr;
            // The native object
            uint8_t native[max_native_size] = {0};
        private:
            // The native object destructor function ptr.
            void (*native_destructor)(object* obj) = nullptr;
            // The native move function ptr.
            void (*native_move_ptr)(object* dest, object* src) = nullptr;
            // The native object copy function ptr.
            std::shared_ptr<object> (*native_copy_ptr)(andy::lang::object* obj) = nullptr;
#ifdef __UVA_DEBUG__
            int* native_int = (int*)native;
#endif
        public:
            void initialize(andy::lang::interpreter* interpreter);
            void initialize(andy::lang::interpreter* interpreter, andy::lang::function_call new_call);
        public:
            /// @brief Initialize the object with a value.
            /// @param cls The class of the object.
            /// @return Returns a shared pointer to the object.
            static auto instantiate(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::structure> cls, andy::lang::function_call new_call = {})
            {
                auto obj = std::make_shared<andy::lang::object>(cls);
                obj->initialize(interpreter, std::move(new_call));

                return obj;
            }
            /// @brief Initialize the object with a value.
            /// @tparam T The type of the value.
            /// @param cls The class of the object.
            /// @param value The pointer to the value. This will be deleted when the object is destroyed.
            /// @return Returns a shared pointer to the object.
            template<typename T>
            static std::shared_ptr<andy::lang::object> instantiate(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::structure> cls, T* value)
            {
                auto obj = std::make_shared<andy::lang::object>(cls);
                obj->set_native_ptr<T>(obj.get(), value);

                obj->initialize(interpreter);

                return obj;
            }
            /// @brief Initialize the object with a value.
            /// @tparam T The type of the value.
            /// @param cls The class of the object.
            /// @param value The value.
            /// @return Returns a shared pointer to the object.
            template<typename T>
            static std::enable_if<!std::is_pointer<T>::value, std::shared_ptr<andy::lang::object>>::type instantiate(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::structure> cls, T value, std::vector<std::shared_ptr<andy::lang::object>> params = {})
            {
                auto obj = std::make_shared<andy::lang::object>(cls);

                if(!std::is_same_v<T, std::nullptr_t>) {
                    obj->set_native<T>(std::move(value));
                }

                andy::lang::function_call new_call;
                new_call.positional_params = std::move(params);

                obj->initialize(interpreter, std::move(new_call));

                return obj;
            }
            static std::shared_ptr<andy::lang::object> create(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::structure> cls)
            {
                auto obj = std::make_shared<andy::lang::object>(cls);
                obj->initialize(interpreter);
                return obj;
            }
            /// @brief Creates the object with a value.
            /// @tparam T The type of the value.
            /// @param cls The class of the object.
            /// @param value The value.
            /// @return Returns a shared pointer to the object.
            template<typename T>
            static std::enable_if<!std::is_pointer<T>::value, std::shared_ptr<andy::lang::object>>::type create(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::structure> cls, T value)
            {
                auto obj = std::make_shared<andy::lang::object>(cls);
                obj->set_native<T>(std::move(value));
                obj->initialize(interpreter);

                return obj;
            }
            /// @brief Creates the object with a pointer to a value.
            /// @tparam T The type of the value.
            /// @param value The pointer to the value.
            /// @return Returns a shared pointer to the object.
            template<typename T>
            static std::shared_ptr<andy::lang::object> create(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::structure> cls, T* value)
            {
                auto obj = std::make_shared<andy::lang::object>(cls);
                obj->set_native_ptr(value);
                return obj;
            }
            template<typename T>
            void set_native(T value) {
                if(native_destructor) {
                    native_destructor(this);
                }

                bool should_destroy = false;

                // Under GCC, even if the constexpr is false, the code still generates warning when 
                // sizoef(T) > max_native_size. So we need silence the warning.
                if constexpr(sizeof(T) <= max_native_size) {
                    if constexpr(std::is_arithmetic<T>::value) {
                        // Boolean, integer, float, etc.
                        *((T*)(&this->native)) = value;
                    } else {
                        new ((T*)(&this->native)) T(std::move(value));
                        should_destroy = true;
                    }
                } else {
                    this->native_ptr = new T(std::move(value));
                    should_destroy = true;
                }
                set_destructor<T>(this);
            }

            template<typename T>
            void set_native_ptr(T* ptr) {
                this->native_ptr = (void*)ptr;
                set_destructor<T>(this);
            }

            std::shared_ptr<object> native_copy() {
                if(native_copy_ptr) {
                    return native_copy_ptr(this);
                }

                return nullptr;
            }
            void reset(std::shared_ptr<andy::lang::structure> new_cls) {
                if(native_destructor) {
                    native_destructor(this);
                    native_destructor = nullptr;
                }
                native_ptr = nullptr;
                native_copy_ptr = nullptr;
                native_move_ptr = nullptr;
                std::memset(native, 0, max_native_size);
                variables.clear();
                functions.clear();
                inline_functions.clear();
                base_instance = nullptr;
                derived_instance = nullptr;
                cls = new_cls;
            }
        private:
            template <typename T>
            static void set_destructor(object* obj) {
                if constexpr(!std::is_arithmetic<T>::value) {
                    obj->native_destructor = [](object* obj) {
                        obj->log_native_destructor();

                        if(obj->native_ptr) {
                            delete (T*)obj->native_ptr;
                        } else {
                            ((T*)(&obj->native))->~T();
                        }
                    };
                }

                obj->native_copy_ptr = [](andy::lang::object* obj) -> std::shared_ptr<andy::lang::object> {
                    std::shared_ptr<andy::lang::object> other = std::make_shared<andy::lang::object>(obj->cls);
                    other->self = other.get();

                    if constexpr(std::is_copy_constructible<T>::value)
                    {
                        other->set_native<T>(obj->as<T>());
                    } else {
                        throw std::runtime_error("Type " + std::string(obj->cls->name) + " does not support copy construction");
                    }

                    for(const auto& [var_name, var_value] : obj->variables) {
                        other->variables[var_name] = var_value->native_copy();
                    }

                    other->functions = obj->functions;
                    other->inline_functions = obj->inline_functions;

                    if(obj->base_instance) {
                        other->base_instance = obj->base_instance->native_copy();
                    }
                    if(obj->derived_instance) {
                        other->derived_instance = obj->derived_instance->native_copy();
                    }

                    return other;
                };

                obj->native_move_ptr = [](object* dest, object* src) {
                    dest->self = dest;
                    dest->cls = std::move(src->cls);
                    dest->base_instance = std::move(src->base_instance);
                    dest->derived_instance = std::move(src->derived_instance);

                    dest->variables = std::move(src->variables);
                    dest->functions = std::move(src->functions);
                    dest->inline_functions = std::move(src->inline_functions);

                    dest->native_destructor = src->native_destructor;
                    dest->native_copy_ptr = src->native_copy_ptr;
                    dest->native_ptr = src->native_ptr;

                    T value = std::move(src->as<T>());
                    dest->set_native<T>(std::move(value));

                    src->native_destructor = nullptr;
                    src->native_copy_ptr = nullptr;
                    src->native_ptr = nullptr;
                    std::memset(src->native, 0, max_native_size);
                };
            }

            void log_native_destructor();
        public:
            bool is_present() const;

            template<typename T>
            const T& as() const {
                if(base_instance) {
                    return base_instance->as<T>();
                }
                if(native_ptr) {
                    return *static_cast<T*>(native_ptr);
                }

                return *static_cast<T*>((void*)native);
            }
            template<typename T>
            T& as() {
                if(base_instance) {
                    return base_instance->as<T>();
                }
                if(native_ptr) {
                    return *static_cast<T*>(native_ptr);
                }

                return *static_cast<T*>((void*)native);
            }
            public:
                object& operator=(object&& other) noexcept
                {
                    if(other.native_move_ptr) {
                        other.native_move_ptr(this, &other);
                    } else {
                        this->cls = std::move(other.cls);
                        this->base_instance = std::move(other.base_instance);
                        this->derived_instance = std::move(other.derived_instance);
                        this->variables = std::move(other.variables);
                        this->functions = std::move(other.functions);
                        this->inline_functions = std::move(other.inline_functions);
                        this->self = this;

                        other.native_destructor = nullptr;
                        other.native_copy_ptr = nullptr;
                        other.native_ptr = nullptr;
                    }
                    return *this;
                }
        };
    };
};