#pragma once

#include <boost/beast/http/basic_parser.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/type_traits.hpp>

#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace launchdarkly::sse {

using namespace boost::beast;

class parser : public http::basic_parser<false> {
   private:
    std::optional<std::string> buffered_line_;
    bool begin_CR_;
    std::vector<std::string> complete_lines_;
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

        return parse_stream(0, body, ec);
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
        return parse_stream(remain, body, ec);
    }

    void on_finish_impl(error_code& ec) override {
        //        if (rd_)
        //            rd_->finish(ec);
        //        else
        //            ec.assign(0, ec.category());
        std::cout << "on_finish_impl\n";
    }

    void complete_line() {
        if (buffered_line_.has_value()) {
            complete_lines_.push_back(buffered_line_.value());
            buffered_line_.reset();
        }
    }

    size_t append_up_to(boost::string_view body, std::string const& search) {
        std::size_t index = body.find_first_of(search);
        if (index != std::string::npos) {
            body.remove_suffix(body.size() - index);
        }
        if (buffered_line_.has_value()) {
            buffered_line_->append(body.to_string());
        } else {
            buffered_line_ = std::string{body};
        }
        return index == std::string::npos ? body.size() : index;
    }

    size_t parse_stream(std::uint64_t remain,
                        boost::string_view body,
                        boost::beast::error_code& ec) {
        size_t i = 0;
        while (i < body.length()) {
            i += this->append_up_to(body.substr(i, body.length() - i), "\r\n");
            if (i == body.size()) {
                continue;
            } else if (body.at(i) == '\r') {
                complete_line();
                begin_CR_ = true;
                i++;
            } else if (body.at(i) == '\n') {
                if (begin_CR_) {
                    begin_CR_ = false;
                    i++;
                } else {
                    complete_line();
                    i++;
                }
            } else {
                begin_CR_ = false;
            }
        }
        return body.length();
    }

   public:
    parser() : buffered_line_(), complete_lines_(), begin_CR_(false) {}
};
}  // namespace launchdarkly::sse
