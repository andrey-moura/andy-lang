#include <andy/lang/api.hpp>
#include <andy/net/http.hpp>

#include <iostream>

std::shared_ptr<andy::lang::structure> create_http_class(andy::lang::interpreter* interpreter)
{
    auto response_class = std::make_shared<andy::lang::structure>("Response");
    auto http_class = std::make_shared<andy::lang::structure>("HTTP");
    http_class->class_methods["get"] = andy::lang::method("get", andy::lang::method_storage_type::class_method, { "url" }, [=](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        const auto& url = params[0]->as<std::string>();

        auto response = andy::net::http::get(url);

        auto response_object = andy::lang::object::create(interpreter, response_class, nullptr);
        response_object->instance_variables["status_code"] = andy::lang::api::to_object(interpreter, response.status_code);
        return response_object;
    });
    andy::lang::api::contained_class(interpreter, http_class, response_class);
    return http_class;
}