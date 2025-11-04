#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "andy/lang/function.hpp"
#include "andy/lang/class.hpp"
#include "andy/lang/context.hpp"

namespace andy
{
    namespace lang {
        class object;
        class structure;
        class interpreter;
        constexpr size_t max_native_size = 40;
        class object : public std::enable_shared_from_this<object>, public context
        {
        public:
            object(std::shared_ptr<andy::lang::structure> c);
            ~object();
        public:
            std::shared_ptr<object> base_instance = nullptr;
            std::shared_ptr<object> derived_instance = nullptr;

            std::map<std::string_view, std::shared_ptr<andy::lang::object>> instance_variables;
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
            // The native object copy function ptr.
            void (*native_copy_to_ptr)(object* obj, object* other) = nullptr;
            // The native object move function ptr.
            void (*native_move_to_ptr)(object* obj, object* other) = nullptr;
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

            template<typename T>
            void native_move_to(object* other) {
                if(native_move_to_ptr) {
                    native_move_to_ptr(this, other);
                }
            }

            void native_copy_to(object* other) {
                if(native_copy_to_ptr) {
                    native_copy_to_ptr(this, other);
                }
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

                obj->native_copy_to_ptr = [](object* obj, object* other) {
                    if(other->native_destructor) {
                        other->native_destructor(other);
                    }

                    if constexpr(std::is_copy_constructible<T>::value)
                    {
                        other->set_native<T>(obj->as<T>());
                    } else {
                        throw std::runtime_error("Type " + std::string(obj->cls->name) + " does not support copy construction");
                    }
                };

                obj->native_move_to_ptr = [](object* obj, object* other) {
                    if(other->native_destructor) {
                        other->native_destructor(other);
                    }

                    if constexpr(std::is_move_constructible<T>::value)
                    {
                        other->set_native<T>(std::move(obj->as<T>()));
                    } else {
                        obj->native_copy_to(other);
                    }
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
        };
    };
};