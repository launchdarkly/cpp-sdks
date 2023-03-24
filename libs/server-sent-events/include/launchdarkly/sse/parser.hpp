#pragma once

#include <boost/beast/http/basic_parser.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/type_traits.hpp>

#include <iostream>

namespace launchdarkly::sse {

using namespace boost::beast;

class parser : public http::basic_parser<false> {
   private:
    void on_request_impl(http::verb method,
                         string_view method_str,
                         string_view target,
                         int version,
                         error_code& ec) override {
//                try {
//                    m_.target(target);
//                    if (method != http::verb::unknown)
//                        m_.method(method);
//                    else
//                        m_.method_string(method_str);
//                    ec.assign(0, ec.category());
//                } catch (std::bad_alloc const&) {
//                    ec = http::error::bad_alloc;
//                }
//                m_.version(version);
std::cout << "on_request\n";
    }
    void on_response_impl(int code,
                          string_view reason,
                          int version,
                          error_code& ec) override {
        //        m_.result(code);
        //        m_.version(version);
        //        try {
        //            m_.reason(reason);
        //            ec.assign(0, ec.category());
        //        } catch (std::bad_alloc const&) {
        //            ec = http::error::bad_alloc;
        //        }
std::cout << "on_response\n";

    }
    void on_field_impl(http::field name,
                       string_view name_string,
                       string_view value,
                       error_code& ec) override {
        //        try {
        //            m_.insert(name, name_string, value);
        //            ec.assign(0, ec.category());
        //        } catch (std::bad_alloc const&) {
        //            ec = http::error::bad_alloc;
        //        }
std::cout << "on_field\n";

    }
    void on_header_impl(error_code& ec) override {
        // ec.assign(0, ec.category());
std::cout << "on_header\n";

    }

    void on_body_init_impl(boost::optional<std::uint64_t> const& content_length,
                           error_code& ec) override {
        //  rd_.emplace(m_, content_length, ec);
std::cout << "on_body_init\n";

    }
    std::size_t on_body_impl(string_view body, error_code& ec) override {
        // return rd_->put(boost::asio::buffer(body.data(), body.size()), ec);
std::cout << "on_body_impl" << body << '\n';

        return body.length();
    }

    void on_chunk_header_impl(std::uint64_t size,
                              string_view extensions,
                              error_code& ec) override {
        // ec.assign(0, ec.category());
        std::cout << "on_chunk_header_impl\n";

    }
    std::size_t on_chunk_body_impl(std::uint64_t remain,
                                   string_view body,
                                   error_code& ec) override {
        //  ec.assign(0, ec.category());
        std::cout << "on_chunk_body_impl " << body << '\n';

        return body.length();
    }

    void on_finish_impl(error_code& ec) override {
        //        if (rd_)
        //            rd_->finish(ec);
        //        else
        //            ec.assign(0, ec.category());
        std::cout << "on_finish_impl\n";
    }

   public:
    parser() = default;
};
}  // namespace launchdarkly::sse
