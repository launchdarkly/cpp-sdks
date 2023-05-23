//
// Copyright (c) 2018-2019 Christian Mazakas (christian dot mazakas at gmail dot
// com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/LeonineKing1199/foxy
//

#ifndef FOXY_UTILITY_HPP_
#define FOXY_UTILITY_HPP_

#include <string>

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>

namespace launchdarkly::foxy {

template <class... Args>
auto make_ssl_ctx(Args&&... args) -> boost::asio::ssl::context {
    auto ctx = boost::asio::ssl::context(std::forward<Args>(args)...);

    boost::certify::enable_native_https_server_verification(ctx);
    ctx.set_verify_mode(boost::asio::ssl::context::verify_peer |
                        boost::asio::ssl::context::verify_fail_if_no_peer_cert);

    return ctx;
}

}  // namespace launchdarkly::foxy

#endif  // FOXY_UTILITY_HPP_
