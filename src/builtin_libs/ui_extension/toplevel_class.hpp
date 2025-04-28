#pragma once

#include <andy/ui/toplevel.hpp>

#include <uva/file.hpp>

namespace andy
{
    namespace lang
    {
        template<typename T>
        class toplevel_class : public T
        {
        public:
            toplevel_class(andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::object> object, std::string_view title)
                : T(title), object(object.get()), interpreter(interpreter)
            {

            }
        protected:
            uva::widgets::widget* focused_widget = nullptr;
            uva::widgets::layout root_layout;
            std::map<std::string, std::vector<std::pair<std::string, std::string>>> bindings;
            andy::lang::object* object;
            andy::lang::interpreter* interpreter;
        protected:
            uva::widgets::widget* find_widget_at(uva::point p)
            {
                uva::widgets::widget* widget = &root_layout;
                for(const auto& child : root_layout.childreans) {
                    if(child->x <= p.x && child->x + child->w >= p.x && child->y <= p.y && child->y + child->h >= p.y) {
                        widget = child.get();

                        if(auto layout = dynamic_cast<uva::widgets::layout*>(widget)) {
                            for(const auto& child : layout->childreans) {
                                if(child->x <= p.x && child->x + child->w >= p.x && child->y <= p.y && child->y + child->h >= p.y) {
                                    widget = child.get();
                                }
                            }
                        }
                    }
                }

                return widget;
            }
        public:
            virtual void motion(uva::point p) override
            {
                uva::widgets::widget* widget = find_widget_at(p);

                if(widget) {
                    ((andy::ui::toplevel*)this)->set_cursor((andy::ui::cursor_type)widget->style.cursor);
                } else {
                    ((andy::ui::toplevel*)this)->set_cursor(andy::ui::cursor_type::pointer);    
                }
            }
            virtual void click(uva::point p) override
            {
                uva::widgets::widget* widget = find_widget_at(p);

                if(widget) {
                    for(const auto& binding : bindings["click"]) {
                        std::string_view selector_view = binding.first;
                        if(selector_view.starts_with("#")) {
                            selector_view.remove_prefix(1);
                            if(selector_view != widget->id) {
                                continue;
                            }
                        } else {
                            continue;
                        }

                        andy::lang::function_call call = {
                            binding.second,
                            object->cls,
                            object->shared_from_this(),
                            &object->cls->instance_methods[binding.second],
                            {},
                            {},
                            nullptr
                        };

                        interpreter->call(call);
                    }
                }

                focused_widget = widget;
            }
            virtual bool on_char(std::string c) override
            {
                if(focused_widget) {
                    return focused_widget->on_char(c);
                }

                return false;
            }
            virtual void draw(uva::drawing::basic_renderer& renderer) override
            {
                //TODO: move to a specific library combining uva-widgets and uva-ui
                if(root_layout.childreans.empty()) {
                    std::string_view class_name = object->cls->name;
                    std::string view_name;
                    view_name.reserve(class_name.size() + 10);

                    for(const char& c : class_name) {
                        if(isupper(c)) {
                            if(view_name.size()) {
                                view_name.push_back('_');
                            }
                            view_name.push_back(tolower(c));
                        } else {
                            view_name.push_back(c);
                        }
                    }
                    std::string_view sufix = "_frame";
                    if(view_name.ends_with(sufix)) {
                        view_name.erase(view_name.size() - sufix.size());
                    } else {
                        sufix = "_dialog";
                        if(view_name.ends_with(sufix)) {
                            view_name.erase(view_name.size() - sufix.size());
                        } else {
                            throw std::runtime_error("invalid class name");
                        }
                    }
                    std::filesystem::path view_folder = std::filesystem::absolute("views");
                    std::filesystem::path view_path = view_folder / (view_name + std::string(sufix));
                    view_path.replace_extension(".xml");

                    std::string content = uva::file::read_all_text<char>(view_path);
                    std::string schema_content = uva::file::read_all_text<char>(view_folder / "schema.xsd");

                    uva::xml xml = uva::xml::decode(std::move(content));
                    uva::xml::schema schema = uva::xml::decode(std::move(schema_content));

                    root_layout.parse(renderer, schema, xml);
                }

                uva::size s = this->size();
                root_layout.w = s.w;
                root_layout.h = s.h;

                root_layout.calculate_layout(0, 0, s.w, s.h);
                root_layout.render(renderer);
            }
        public:
            static std::shared_ptr<andy::lang::structure> create(andy::lang::interpreter* interpreter, std::string cls)
            {
                auto ui_frame_class = std::make_shared<andy::lang::structure>(cls);

                ui_frame_class->instance_methods = {
                    { "new", andy::lang::method("new", andy::lang::method_storage_type::instance_method, {"title"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params){
                        std::string title = params[0]->as<std::string>();

                        toplevel_class<T>* frame = new toplevel_class<T>(interpreter, object, title);

                        object->set_native_ptr(frame);

                        return nullptr;
                    })},
                    { "show", andy::lang::method("show", andy::lang::method_storage_type::instance_method, {andy::lang::fn_parameter("maximized", true, false)}, [](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params, std::map<std::string, std::shared_ptr<andy::lang::object>> named_params){
                        std::shared_ptr<andy::lang::object> maximized = named_params["maximized"];
                        toplevel_class<T>& frame = object->as<toplevel_class<T>>();
                        frame.show(maximized->is_present());

                        return nullptr;
                    })},
                    { "hide", andy::lang::method("hide", andy::lang::method_storage_type::instance_method, [](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params){
                        toplevel_class<T>& frame = object->as<toplevel_class<T>>();
                        //frame.hide();

                        return nullptr;
                    })},
                    { "bind", andy::lang::method("bind", andy::lang::method_storage_type::instance_method, {"event", "selector", "callback"}, [](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params){
                        toplevel_class<T>& frame = object->as<toplevel_class<T>>();

                        std::string event = params[0]->as<std::string>();
                        std::string selector = params[1]->as<std::string>();
                        std::string callback = params[2]->as<std::string>();

                        frame.bindings[event].push_back({selector, callback});

                        return nullptr;
                    })},
                };

                return ui_frame_class;
            }
        };
    };
};