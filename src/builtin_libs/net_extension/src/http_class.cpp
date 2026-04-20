#include <andy/lang/api.hpp>
#include <andy/net/http.hpp>

#include <iostream>

std::shared_ptr<andy::lang::structure> create_http_class(andy::lang::interpreter* interpreter)
{
    auto response_class = std::make_shared<andy::lang::structure>("Response");
    response_class->instance_functions["text"] = std::make_shared<andy::lang::function>("text", [](andy::lang::interpreter* interpreter) {
        auto& response = interpreter->current_context->self->as<andy::net::http::response>();
        std::string_view text = response.text();
        return andy::lang::api::to_object(interpreter, text);
    });
    response_class->instance_functions["text?"] = std::make_shared<andy::lang::function>("text?", [](andy::lang::interpreter* interpreter) {
        auto& response = interpreter->current_context->self->as<andy::net::http::response>();
        return andy::lang::api::to_object(interpreter, response.is_text());
    });
    response_class->instance_functions["json"] = std::make_shared<andy::lang::function>("json", [](andy::lang::interpreter* interpreter) {
        auto& response = interpreter->current_context->self->as<andy::net::http::response>();
        andy::lang::dictionary json;
        std::string_view content_type = response.headers["Content-Type"];
        if (!content_type.starts_with("application/json")) {
            throw std::runtime_error("Trying to access response body as JSON, but Content-Type is not application/json");
        }

        std::string_view json_text(reinterpret_cast<const char*>(response.raw_body.data()), response.raw_body.size());

        andy::lang::lexer l("", std::string(json_text));
        l.tokenize();

        andy::lang::parser p;
        auto node = p.parse_identifier_or_literal(l);

        return interpreter->node_to_object(node);
    });
    response_class->instance_functions["json?"] = std::make_shared<andy::lang::function>("json?", [](andy::lang::interpreter* interpreter) {
        auto& response = interpreter->current_context->self->as<andy::net::http::response>();
        return andy::lang::api::to_object(interpreter, response.headers["Content-Type"].starts_with("application/json"));
    });
    auto http_class = std::make_shared<andy::lang::structure>("HTTP");
    http_class->functions["get"] = std::make_shared<andy::lang::function>("get", std::initializer_list<std::string>{ "url" }, [response_class](andy::lang::interpreter* interpreter) {
        const auto& url = interpreter->current_context->positional_params[0]->as<std::string>();

        auto response = andy::net::http::get(url);
        int status_code = response.status_code;
        std::shared_ptr<andy::lang::object> response_object = andy::lang::object::create(interpreter, response_class, std::move(response));

        response_object->variables["status_code"] = andy::lang::api::to_object(interpreter, status_code);
        return response_object;
    });
    andy::lang::api::contained_class(interpreter, http_class, response_class);
    return http_class;
}