#pragma once
#include <andy/lang/class.hpp>
#include <andy/lang/method.hpp>
#include <andy/lang/object.hpp>

namespace andy
{
    namespace lang
    {
        template<typename T>
        void add_operators(std::shared_ptr<andy::lang::structure> cls, interpreter* interpreter)
        {
            cls->instance_methods["+"] = andy::lang::method("+",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    return andy::lang::object::instantiate(interpreter, interpreter->DoubleClass, value + other->as<double>());
                } else if(other->cls == interpreter->IntegerClass) {
                    return andy::lang::object::instantiate(interpreter, cls, value + other->as<int>());
                } else if(other->cls == interpreter->FloatClass) {
                    return andy::lang::object::instantiate(interpreter, interpreter->FloatClass, value + other->as<float>());
                }

                throw std::runtime_error("undefined operator+(" + object->cls->name + ", " + other->cls->name + ")");
            });
            cls->instance_methods["-"] = andy::lang::method("-",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    return andy::lang::object::instantiate(interpreter, interpreter->DoubleClass, value - other->as<double>());
                } else if(other->cls == interpreter->IntegerClass) {
                    return andy::lang::object::instantiate(interpreter, cls, value - other->as<int>());
                } else if(other->cls == interpreter->FloatClass) {
                    return andy::lang::object::instantiate(interpreter, interpreter->FloatClass, value - other->as<float>());
                }

                throw std::runtime_error("undefined operator-(" + object->cls->name + ", " + other->cls->name + ")");
            });
            cls->instance_methods["*"] = andy::lang::method("*",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    return andy::lang::object::instantiate(interpreter, interpreter->DoubleClass, value * other->as<double>());
                } else if(other->cls == interpreter->IntegerClass) {
                    return andy::lang::object::instantiate(interpreter, cls, value * other->as<int>());
                } else if(other->cls == interpreter->FloatClass) {
                    return andy::lang::object::instantiate(interpreter, interpreter->FloatClass, value * other->as<float>());
                }

                throw std::runtime_error("undefined operator*(" + object->cls->name + ", " + other->cls->name + ")");
            });
            cls->instance_methods["/"] = andy::lang::method("/",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    return andy::lang::object::instantiate(interpreter, interpreter->DoubleClass, value / other->as<double>());
                } else if(other->cls == interpreter->IntegerClass) {
                    return andy::lang::object::instantiate(interpreter, cls, value / other->as<int>());
                } else if(other->cls == interpreter->FloatClass) {
                    return andy::lang::object::instantiate(interpreter, interpreter->FloatClass, value / other->as<float>());
                }

                throw std::runtime_error("undefined operator/(" + object->cls->name + ", " + other->cls->name + ")");
            });
            if constexpr (std::is_integral_v<T>) {
                cls->instance_methods["%"] = andy::lang::method("%",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                    T& value = object->as<T>();
                    std::shared_ptr<andy::lang::object> other = params[0];
                    if(other->cls == interpreter->IntegerClass) {
                        return andy::lang::object::instantiate(interpreter, cls, value % other->as<int>());
                    }

                    throw std::runtime_error("undefined operator%(" + object->cls->name + ", " + other->cls->name + ")");
                });
            }
            cls->instance_methods["++"] = andy::lang::method("+",andy::lang::method_storage_type::instance_method, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                value++;

                return nullptr;
            });
            cls->instance_methods["!="] = andy::lang::method("!=",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    return value != other->as<double>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                } else if(other->cls == interpreter->IntegerClass) {
                    return value != other->as<int>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                } else if(other->cls == interpreter->FloatClass) {
                    return value != other->as<float>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                }

                throw std::runtime_error("undefined operator!=(" + object->cls->name + ", " + other->cls->name + ")");
            });
            cls->instance_methods["=="] = andy::lang::method("==",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    return value == other->as<double>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                } else if(other->cls == interpreter->IntegerClass) {
                    return value == other->as<int>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                } else if(other->cls == interpreter->FloatClass) {
                    return value == other->as<float>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                }

                throw std::runtime_error("undefined operator==(" + object->cls->name + ", " + other->cls->name + ")");
            });
            cls->instance_methods["<"] = andy::lang::method("<",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    return value < other->as<double>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                } else if(other->cls == interpreter->IntegerClass) {
                    return value < other->as<int>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                } else if(other->cls == interpreter->FloatClass) {
                    return value < other->as<float>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                }

                throw std::runtime_error("undefined operator<(" + object->cls->name + ", " + other->cls->name + ")");
            });
            cls->instance_methods[">"] = andy::lang::method(">",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    return value > other->as<double>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                } else if(other->cls == interpreter->IntegerClass) {
                    return value > other->as<int>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                } else if(other->cls == interpreter->FloatClass) {
                    return value > other->as<float>() ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
                }

                throw std::runtime_error("undefined operator>(" + object->cls->name + ", " + other->cls->name + ")");
            });
            cls->instance_methods["+="] = andy::lang::method("+=",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    value += other->as<double>();
                } else if(other->cls == interpreter->IntegerClass) {
                    value += other->as<int>();
                } else if(other->cls == interpreter->FloatClass) {
                    value += other->as<float>();
                } else {
                    throw std::runtime_error("undefined operator+=(" + object->cls->name + ", " + other->cls->name + ")");
                }

                return nullptr;
            });
            cls->instance_methods["-="] = andy::lang::method("-=",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    value -= other->as<double>();
                } else if(other->cls == interpreter->IntegerClass) {
                    value -= other->as<int>();
                } else if(other->cls == interpreter->FloatClass) {
                    value -= other->as<float>();
                } else {
                    throw std::runtime_error("undefined operator-=(" + object->cls->name + ", " + other->cls->name + ")");
                }

                return nullptr;
            });
            cls->instance_methods["*="] = andy::lang::method("*=",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    value *= other->as<double>();
                } else if(other->cls == interpreter->IntegerClass) {
                    value *= other->as<int>();
                } else if(other->cls == interpreter->FloatClass) {
                    value *= other->as<float>();
                } else {
                    throw std::runtime_error("undefined operator*=(" + object->cls->name + ", " + other->cls->name + ")");
                }

                return nullptr;
            });
            cls->instance_methods["/="] = andy::lang::method("/=",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                T& value = object->as<T>();
                std::shared_ptr<andy::lang::object> other = params[0];
                if(other->cls == interpreter->DoubleClass) {
                    value /= other->as<double>();
                } else if(other->cls == interpreter->IntegerClass) {
                    value /= other->as<int>();
                } else if(other->cls == interpreter->FloatClass) {
                    value /= other->as<float>();
                } else {
                    throw std::runtime_error("undefined operator/=(" + object->cls->name + ", " + other->cls->name + ")");
                }

                return nullptr;
            });
            if constexpr (std::is_integral_v<T>) {
                cls->instance_methods["%="] = andy::lang::method("%=",andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, cls](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                    T& value = object->as<T>();
                    std::shared_ptr<andy::lang::object> other = params[0];
                    if(other->cls == interpreter->IntegerClass) {
                        value %= other->as<int>();
                    } else {
                        throw std::runtime_error("undefined operator%=(" + object->cls->name + ", " + other->cls->name + ")");
                    }

                    return nullptr;
                });
            }
        }
    };
};