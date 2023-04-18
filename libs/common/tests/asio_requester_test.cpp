#include <gtest/gtest.h>

#include "config/detail/http_properties_builder.hpp"
#include "config/detail/sdks.hpp"
#include "network/detail/asio_requester.hpp"

using launchdarkly::network::detail::AsioRequester;
using launchdarkly::network::detail::HttpMethod;
using launchdarkly::network::detail::HttpRequest;

using launchdarkly::config::detail::ClientSDK;
using launchdarkly::config::detail::HttpPropertiesBuilder;

TEST(AsioRequesterTests, CanMakeRequest) {
    net::io_context ioc{1};
    AsioRequester requester(ioc.get_executor());

    //    (std::string host,
    //     HttpMethod method,
    //     HeadersType headers,
    //     config::detail::HttpProperties properties,
    //     BodyType body)

    std::cout << "Making request" << std::endl;
    requester
        .Request(HttpRequest("http://localhost:8080/", HttpMethod::kGet,
                             HttpPropertiesBuilder<ClientSDK>().build(),
                             std::nullopt),
                 [](auto response) {
                     std::cout << "Response: " << response << std::endl;
                 })
        ->run();

    ioc.run();
}