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

TEST(AsioRequesterTests, CanMakeRequest) {
    boost::asio::io_service ios;
    boost::asio::io_service::work work(ios);

    std::thread io_thread([&ios]() { ios.run(); });
    AsioRequester requester(ios.get_executor());

    //    requester.Request(
    //        HttpRequest("http://localhost:8080/", HttpMethod::kGet,
    //                    HttpPropertiesBuilder<ClientSDK>().Build(),
    //                    std::nullopt),
    //        [](auto response) {
    //            std::cout << "Response1: " << response << std::endl;
    //        });
    //
    //        requester.Request(
    //            HttpRequest("https://www.google.com", HttpMethod::kGet,
    //                        HttpPropertiesBuilder<ClientSDK>().Build(),
    //                        std::nullopt),
    //            [](auto response) {
    //                std::cout << "Response1: " << response << std::endl;
    //            });

//    auto res =
//        requester
//            .Request(HttpRequest("http://localhost:8080", HttpMethod::kGet,
//                                 HttpPropertiesBuilder<ClientSDK>().Build(),
//                                 R"({"bacon":true})"),
//                     boost::asio::use_future)
//            .get();

        auto res =
            requester
                .Request(HttpRequest("https://www.google.com",
                HttpMethod::kGet,
                                     HttpPropertiesBuilder<ClientSDK>().Build(),
                                     std::nullopt),
                         boost::asio::use_future)
                .get();

    std::cout << "Response2:" << res << std::endl;

    ios.stop();
    io_thread.join();
}