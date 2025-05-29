#pragma once

#include <vector>
#include <memory>

#include <uva/var.hpp>
#include <andy/lang/parser.hpp>
#include <andy/lang/class.hpp>
#include <andy/lang/method.hpp>
#include <andy/lang/object.hpp>

namespace andy
{
    namespace lang
    {
        class extension;
        // The context of the interpreter execution. It is relative to a block.
        struct interpreter_context
        {
            std::shared_ptr<andy::lang::structure> cls;
            std::shared_ptr<andy::lang::object> self;
            std::map<std::string_view, std::shared_ptr<andy::lang::object>> variables;
            std::map<std::string_view, andy::lang::method> functions;
            const andy::lang::parser::ast_node* given_block = nullptr;

            bool has_returned = false;
            std::shared_ptr<andy::lang::object> return_value;
        };
        // This class is responsible of storing all resources needed by an andylang program.
        // It will store all classes, objects, methods, variables, call stack, etc.
        class interpreter
        {
        public:
            /// @brief Construct a new interpreter object. When the interpreter object is constructed, it will
            // initialize all resources needed to run the interpreter. If you want to declare the interpreter
            // but not initialize it, you can use a interpreter pointer and initialize it later.
            interpreter();
            ~interpreter() = default;
        public:
            std::filesystem::path input_file_path;
        public:
            /// @brief Load a class into the vm. The class is kept alive by the vm untill it is destroyed.
            /// @param cls The class to be loaded. It is kept alive by the vm untill it is destroyed. It is globally accessible.
            void load(std::shared_ptr<andy::lang::structure> cls);

            /// @brief Exeuctes a syntax tree into the interpreter. Note that if the code has while loops with no exit condition, this method will never return.
            /// @param cls The syntax tree to exeuctes. All its childs (not recursively) will be executed.
            std::shared_ptr<andy::lang::object> execute(const andy::lang::parser::ast_node& source_code, std::shared_ptr<andy::lang::object>& object);

            /// @brief Exeuctes a class declaration into the interpreter.
            /// @param source_code The class declaration.
            std::shared_ptr<andy::lang::structure> execute_classdecl(const andy::lang::parser::ast_node& source_code);

            std::shared_ptr<andy::lang::object> execute_all(std::vector<andy::lang::parser::ast_node>::const_iterator begin, std::vector<andy::lang::parser::ast_node>::const_iterator end, std::shared_ptr<andy::lang::object>& object);
            std::shared_ptr<andy::lang::object> execute_all(const andy::lang::parser::ast_node& source_code, std::shared_ptr<andy::lang::object>& object);

            std::shared_ptr<andy::lang::object> execute_all(const andy::lang::parser::ast_node& source_code)
            {
                std::shared_ptr<andy::lang::object> tmp;
                return execute_all(source_code, tmp);
            }

            /// @brief The global false class.
            std::shared_ptr<andy::lang::structure> FalseClass;
            /// @brief The global true class.
            std::shared_ptr<andy::lang::structure> TrueClass;

            /// @brief The global std class.
            std::shared_ptr<andy::lang::structure> StdClass;

            /// @brief The global string class.
            std::shared_ptr<andy::lang::structure> StringClass;

            /// @brief The global integer class.
            std::shared_ptr<andy::lang::structure> IntegerClass;

            /// @brief The global double class.
            std::shared_ptr<andy::lang::structure> DoubleClass;

            /// @brief The global float class.
            std::shared_ptr<andy::lang::structure> FloatClass;

            /// @brief The global file class.
            std::shared_ptr<andy::lang::structure> FileClass;

            /// @brief The global array class.
            std::shared_ptr<andy::lang::structure> ArrayClass;

            /// @brief The global null class.
            std::shared_ptr<andy::lang::structure> NullClass;

            /// @brief The global dictionary class.
            std::shared_ptr<andy::lang::structure> DictionaryClass;

            /// @brief The global system class.
            std::shared_ptr<andy::lang::structure> SystemClass;

            /// @brief The global path class.
            std::shared_ptr<andy::lang::structure> PathClass;

            /// @brief The global andy config class.
            std::shared_ptr<andy::lang::structure> AndyConfigClass;

            /// @brief The global class class.
            std::shared_ptr<andy::lang::structure> ClassClass;

            std::shared_ptr<andy::lang::object> call(function_call& call);

            std::shared_ptr<andy::lang::structure> find_class(const std::string_view& name) {
                for(auto& cls : classes) {
                    if(cls->name == name) {
                        return cls;
                    }
                }

                return nullptr;
            }

            const std::shared_ptr<andy::lang::object> try_object_from_declname(const andy::lang::parser::ast_node& node, std::shared_ptr<andy::lang::structure> cls = nullptr, std::shared_ptr<andy::lang::object> object = nullptr);
            const std::shared_ptr<andy::lang::object> node_to_object(const andy::lang::parser::ast_node& node, std::shared_ptr<andy::lang::structure> cls = nullptr, std::shared_ptr<andy::lang::object> object = nullptr);
            std::shared_ptr<andy::lang::object> var_to_object(var v);

            void load_extension(andy::lang::extension* extension);

            void start_extensions();
        protected:
            /// @brief The global context stack.
            interpreter_context global_context;

            /// @brief The call stack.
            std::vector<interpreter_context> stack;

            std::vector<andy::lang::extension*> extensions;

            bool is_global_context() const
            {
                return stack.empty();
            }

            interpreter_context& current_context()
            {
                if(stack.empty()) {
                    return global_context;
                }

                return stack.back();
            }

            void push_context(bool inherit = false) {
                auto ccontext = current_context();
                stack.push_back(interpreter_context());

                if(inherit) {
                    stack.back() = ccontext;
                }
            }

            void pop_context() { 
                if(stack.empty()) {
                    throw std::runtime_error("interpreter: unexpected end of file");
                }

                stack.pop_back();
            }
        protected:
            /// @brief Initialize the interpreter. This method will create the global classes and objects. It also load extensions.
            void init();
        public:
            std::vector<std::shared_ptr<andy::lang::structure>> classes;
        };
    }  
}; // namespace andy