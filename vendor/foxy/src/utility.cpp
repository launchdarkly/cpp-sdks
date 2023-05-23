//
// Copyright (c) 2018-2019 Christian Mazakas (christian dot mazakas at gmail dot
// com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/LeonineKing1199/foxy
//

#include <foxy/utility.hpp>

#include <boost/certify/https_verification.hpp>

namespace asio = boost::asio;

void launchdarkly::foxy::certify::enable_https_verification(
    boost::asio::ssl::context& ssl_ctx) {
    // This method is called here, instead of in the header, so that boost/certify
    // is only used in implementation. If it was used in the header
    // it would be a public dependency.
    boost::certify::enable_native_https_server_verification(ssl_ctx);
}

auto launchdarkly::foxy::certify::set_server_hostname(
    ::SSL* ssl,
    boost::string_view hostname,
    unsigned int flags,
    boost::system::error_code& ec) -> void {
    auto* const verify_param = ::SSL_get0_param(ssl);
    ::X509_VERIFY_PARAM_set_hostflags(
        verify_param, flags | X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);

    if (!X509_VERIFY_PARAM_set1_host(verify_param, hostname.data(),
                                     hostname.size())) {
        ec = {static_cast<int>(::ERR_get_error()),
              boost::asio::error::get_ssl_category()};
        return;
    }

    ec = {};
}

auto launchdarkly::foxy::certify::set_server_hostname(
    ::SSL* ssl,
    boost::string_view hostname,
    unsigned int flags) -> void {
    auto ec = boost::system::error_code();

    auto* const verify_param = ::SSL_get0_param(ssl);
    ::X509_VERIFY_PARAM_set_hostflags(
        verify_param, flags | X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);

    if (!X509_VERIFY_PARAM_set1_host(verify_param, hostname.data(),
                                     hostname.size())) {
        ec = {static_cast<int>(::ERR_get_error()),
              boost::asio::error::get_ssl_category()};
    } else {
        ec = {};
    }

    if (ec) {
        boost::throw_exception(boost::system::system_error(ec));
    }
}
