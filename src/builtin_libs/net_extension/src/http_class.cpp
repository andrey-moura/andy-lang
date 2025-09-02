#include <andy/lang/api.hpp>
#include <andy/net/http.hpp>

#include <iostream>

std::shared_ptr<andy::lang::structure> create_http_class(andy::lang::interpreter* interpreter)
{
    auto response_class = std::make_shared<andy::lang::structure>("Response");
    response_class->instance_methods["text"] = andy::lang::method("text", andy::lang::method_storage_type::instance_method, {  }, [=](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        auto& response = object->as<andy::net::http::response>();
        std::string_view text = response.text();
        return andy::lang::api::to_object(interpreter, text);
    });
    response_class->instance_methods["text?"] = andy::lang::method("text?", andy::lang::method_storage_type::instance_method, {  }, [=](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        auto& response = object->as<andy::net::http::response>();
        return andy::lang::api::to_object(interpreter, response.is_text());
    });
    response_class->instance_methods["json"] = andy::lang::method("json", andy::lang::method_storage_type::instance_method, {  }, [=](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        auto& response = object->as<andy::net::http::response>();
        andy::lang::dictionary json;
        std::string_view content_type = response.headers["Content-Type"];
        if (!content_type.starts_with("application/json")) {
            throw std::runtime_error("Trying to access response body as JSON, but Content-Type is not application/json");
        }

        std::string_view json_text(reinterpret_cast<const char*>(response.raw_body.data()), response.raw_body.size());

        andy::lang::lexer l("", json_text);

        andy::lang::parser p;
        auto node = p.parse_identifier_or_literal(l);

        return interpreter->node_to_object(node);
    });
    response_class->instance_methods["json?"] = andy::lang::method("json?", andy::lang::method_storage_type::instance_method, {  }, [=](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        auto& response = object->as<andy::net::http::response>();
        return andy::lang::api::to_object(interpreter, response.headers["Content-Type"].starts_with("application/json"));
    });
    auto http_class = std::make_shared<andy::lang::structure>("HTTP");
    http_class->class_methods["get"] = andy::lang::method("get", andy::lang::method_storage_type::class_method, { "url" }, [=](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        const auto& url = params[0]->as<std::string>();

        auto response = andy::net::http::get(url);

        auto response_object = andy::lang::object::create(interpreter, response_class, std::move(response));
        response_object->instance_variables["status_code"] = andy::lang::api::to_object(interpreter, response_object->as<andy::net::http::response>().status_code);
        return response_object;
    });
    andy::lang::api::contained_class(interpreter, http_class, response_class);
    return http_class;
}