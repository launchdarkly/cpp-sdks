#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <boost/asio/use_future.hpp>
#include <future>
#include <thread>
#include "config/detail/builders/http_properties_builder.hpp"
#include "config/detail/sdks.hpp"
#include "network/detail/asio_requester.hpp"

using launchdarkly::network::detail::AsioRequester;
using launchdarkly::network::detail::HttpMethod;
using launchdarkly::network::detail::HttpRequest;

using launchdarkly::config::detail::ClientSDK;
using launchdarkly::config::detail::builders::HttpPropertiesBuilder;

// template <typename CompletionToken>
// auto my_fancy_async_operation(boost::asio::io_service& ios,
//                               std::string const& message,
//                               CompletionToken&& token) {
//     namespace asio = boost::asio;
//     namespace system = boost::system;
//
//     using Sig = void(system::error_code, std::string);
//     using Result = asio::async_result<std::decay_t<CompletionToken>, Sig>;
//     using Handler = typename Result::completion_handler_type;
//
//     Handler handler(std::forward<decltype(token)>(token));
//     Result result(handler);
//
//     ios.post([handler, message]() mutable {
//         handler(boost::system::error_code(), message);
//     });
//
//     return result.get();
// }
//
// TEST(Futures, Please) {
//     boost::asio::io_service ios;
//     auto msg =
//         my_fancy_async_operation(ios, "future",
//         boost::asio::use_future).get();
// }

TEST(AsioRequesterTests, CanMakeRequest) {
    //    net::io_context ioc{1};
    boost::asio::io_service ios;
    boost::asio::io_service::work work(ios);

    std::thread io_thread([&ios]() { ios.run(); });
    AsioRequester requester(ios.get_executor());

    //    (std::string host,
    //     HttpMethod method,
    //     HeadersType headers,
    //     config::detail::HttpProperties properties,
    //     BodyType body)

    //    std::thread([&]() { ioc.run(); });

        requester.Request(
            HttpRequest("http://localhost:8080/", HttpMethod::kGet,
                        HttpPropertiesBuilder<ClientSDK>().Build(),
                        std::nullopt),
            [](auto response) {
                std::cout << "Response1: " << response << std::endl;
            });

    auto res =
        requester
            .Request(HttpRequest("http://localhost:8080/", HttpMethod::kGet,
                                 HttpPropertiesBuilder<ClientSDK>().Build(),
                                 std::nullopt),
                     boost::asio::use_future)
            .get();

    std::cout << "Response2:" << res << std::endl;

    ios.stop();
    io_thread.join();
    //    ioc.run();

    //    auto res =
    //        requester
    //            .Request(HttpRequest("http://localhost:8080/",
    //            HttpMethod::kGet,
    //                                 HttpPropertiesBuilder<ClientSDK>().Build(),
    //                                 std::nullopt),
    //                     boost::asio::use_future)
    //            .get();

    //    std::cout << "Response:" << res << std::endl;

    //    std::cout << "Making request" << std::endl;
    //    requester
    //        .Request(HttpRequest("http://localhost:8080/", HttpMethod::kGet,
    //                             HttpPropertiesBuilder<ClientSDK>().Build(),
    //                             std::nullopt),
    //                 [](auto response) {
    //                     std::cout << "Response: " << response << std::endl;
    //                 })
    //        ->run();
}