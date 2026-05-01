#pragma once

#include <vector>
#include <memory>

#include <andy/lang/parser.hpp>
#include <andy/lang/class.hpp>
#include <andy/lang/function.hpp>
#include <andy/lang/object.hpp>
#include <andy/lang/interpreter_context.hpp>

struct andy_lang_runtime_exception {
    andy_lang_runtime_exception(std::shared_ptr<andy::lang::object> exception_object)
        : exception_object(std::move(exception_object))
    {
        
    }
    std::shared_ptr<andy::lang::object> exception_object;
};

namespace andy
{
    namespace lang
    {
        class extension;
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
            andy::lang::lexer* main_lexer = nullptr;
            const andy::lang::parser::ast_node* current_node = nullptr;
        public:
            /// @brief Load a class into the vm. The class is kept alive by the vm untill it is destroyed.
            /// @param cls The class to be loaded. It is kept alive by the vm untill it is destroyed. It is globally accessible.
            void load(std::shared_ptr<andy::lang::structure> cls);

            /// @brief Exeuctes a syntax tree into the interpreter. Note that if the code has while loops with no exit condition, this method will never return.
            /// @param cls The syntax tree to exeuctes. All its childs (not recursively) will be executed.
            std::shared_ptr<andy::lang::object> execute(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_all(
                std::vector<andy::lang::parser::ast_node>::const_iterator begin,
                std::vector<andy::lang::parser::ast_node>::const_iterator end
            );
            std::shared_ptr<andy::lang::object> execute_all(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_context(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_classdecl(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_classdecl_base(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_fn_decl(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_fn_return(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_fn_call(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_fn_params(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_fn_object(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_valuedecl(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_pair(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_interpolated_string(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_arraydecl(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_dictionarydecl(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_vardecl(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_decltype(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_declname(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_declstatic(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_conditional(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_while(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_for(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_for_start(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_for_step(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_for_end(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_foreach(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_break(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_else(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_condition(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_yield(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_try(const andy::lang::parser::ast_node& source_code);
            std::shared_ptr<andy::lang::object> execute_throw(const andy::lang::parser::ast_node& source_code);
            /// @brief The global false class.
            std::shared_ptr<andy::lang::structure> FalseClass;
            /// @brief The global true class.
            std::shared_ptr<andy::lang::structure> TrueClass;

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

            /// @brief The global function class.
            std::shared_ptr<andy::lang::structure> FunctionClass;

            std::shared_ptr<andy::lang::structure> find_class(const std::string_view& name);

            const std::shared_ptr<andy::lang::object> try_object_from_declname
            (
                const andy::lang::parser::ast_node& node,
                std::shared_ptr<andy::lang::structure> cls = nullptr,
                std::shared_ptr<andy::lang::object> object = nullptr
            );
            const std::shared_ptr<andy::lang::object> node_to_object(const andy::lang::parser::ast_node& node, std::shared_ptr<andy::lang::structure> cls = nullptr, std::shared_ptr<andy::lang::object> object = nullptr);

            void load_extension(andy::lang::extension* extension);

            std::shared_ptr<andy::lang::interpreter_context> current_context = nullptr;
            std::shared_ptr<andy::lang::interpreter_context> previous_context = nullptr;
            std::shared_ptr<andy::lang::interpreter_context> global_context = nullptr;

            /// @brief The call stack.
            std::vector<std::shared_ptr<interpreter_context>> stack;
            std::vector<std::string_view> args;
        protected:
            std::vector<andy::lang::extension*> extensions;

            bool is_global_context() const
            {
                return stack.size() == 1;
            }
            public:

            void update_current_context();
            void push_context();
            void push_block_context();
            void push_context(std::shared_ptr<andy::lang::object> object);
            void pop_context();
            void push_context_with_object(std::shared_ptr<andy::lang::object> object);
        protected:
            /// @brief Initialize the interpreter. This method will create the global classes and objects. It also load extensions.
            void init();
        };
    }  
}; // namespace andy