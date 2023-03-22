#pragma once
#include <boost/beast/websocket/stream.hpp>
#include <boost/optional/optional.hpp>
#include <boost/tokenizer.hpp>
#include <iostream>
#include <vector>

namespace launchdarkly::sse::detail {

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace http = beast::http;    // from <boost/beast/http.hpp>
namespace net = boost::asio;     // from <boost/asio.hpp>

// A layered stream which implements the SSE protocol.
template <class NextLayer>
class sse_stream {
    NextLayer next_layer_;
    bool begin_CR_;
    boost::optional<std::string> buffered_line_;
    std::deque<std::string> complete_lines_;

    // This is the "initiation" object passed to async_initiate to start the
    // operation
    struct run_read_op {
        template <class ReadHandler, class MutableBufferSequence>
        void operator()(ReadHandler&& handler,
                        sse_stream* stream,
                        MutableBufferSequence const& buffers) {
            using handler_type = typename std::decay<ReadHandler>::type;

            // async_base handles all of the composed operation boilerplate for
            // us
            using base = beast::async_base<handler_type,
                                           beast::executor_type<NextLayer>>;

            // Our composed operation is implemented as a completion handler
            // object
            struct op : base {
                sse_stream& stream_;

                op(sse_stream& stream,
                   handler_type&& handler,
                   MutableBufferSequence const& buffers)
                    : base(std::move(handler), stream.get_executor()),
                      stream_(stream) {
                    // Start the asynchronous operation
                    std::cout << "op sse_stream: async_read_some\n";
                    stream_.next_layer().async_read_some(buffers,
                                                         std::move(*this));
                }

                void operator()(beast::error_code ec,
                                std::size_t bytes_transferred) {
                    std::cout << "op sse_stream: completion handler\n";
                    this->complete_now(ec, bytes_transferred);
                }
            };

            op(*stream, std::forward<ReadHandler>(handler), buffers);
        }
    };

    // This is the "initiation" object passed to async_initiate to start the
    // operation
    struct run_write_op {
        template <class WriteHandler, class ConstBufferSequence>
        void operator()(WriteHandler&& handler,
                        sse_stream* stream,
                        ConstBufferSequence const& buffers) {
            using handler_type = typename std::decay<WriteHandler>::type;

            // async_base handles all of the composed operation boilerplate for
            // us
            using base = beast::async_base<handler_type,
                                           beast::executor_type<NextLayer>>;

            // Our composed operation is implemented as a completion handler
            // object
            struct op : base {
                sse_stream& stream_;

                op(sse_stream& stream,
                   handler_type&& handler,
                   ConstBufferSequence const& buffers)
                    : base(std::move(handler), stream.get_executor()),
                      stream_(stream) {
                    // Start the asynchronous operation
                    std::cout << "sse_stream: async_write_some\n";
                    stream_.next_layer().async_write_some(buffers,
                                                          std::move(*this));
                }

                void operator()(beast::error_code ec,
                                std::size_t bytes_transferred) {

                    std::cout << "sse_stream: completion handler\n";
                    this->complete_now(ec, bytes_transferred);
                }
            };

            op(*stream, std::forward<WriteHandler>(handler), buffers);
        }
    };

    void complete_line() {
        if (buffered_line_.has_value()) {
            complete_lines_.push_back(buffered_line_.value());
            std::cout << "Line: <" << buffered_line_.value() << ">"
                      << std::endl;
            buffered_line_.reset();
        }
    }

    size_t append_up_to(std::string_view body, std::string const& search) {
        std::size_t index = body.find_first_of(search);
        if (index != std::string::npos) {
            body.remove_suffix(body.size() - index);
        }
        if (buffered_line_.has_value()) {
            buffered_line_->append(body);
        } else {
            buffered_line_ = std::string{body};
        }
        return index == std::string::npos ? body.size() : index;
    }

    template <class MutableBufferSequence>
    void decode_and_buffer_lines(MutableBufferSequence const& body) {
        size_t i = 0;
        while (i < body.length()) {
            i += this->append_up_to(body.substr(i, body.length() - i), "\r\n");
            if (body[i] == '\r') {
                if (this->begin_CR_) {
                    // todo: illegal token
                } else {
                    this->begin_CR_ = true;
                }
            } else if (body[i] == '\n') {
                this->begin_CR_ = false;
                this->complete_line();
                i++;
            }
        }
        return body.length();
    }

   public:
    using executor_type = beast::executor_type<NextLayer>;

    template <class... Args>
    explicit sse_stream(Args&&... args)
        : next_layer_{std::forward<Args>(args)...},
          begin_CR_{false},
          buffered_line_{},
          complete_lines_{} {}

    /// Returns an instance of the executor used to submit completion handlers
    executor_type get_executor() noexcept { return next_layer_.get_executor(); }

    /// Returns a reference to the next layer
    NextLayer& next_layer() noexcept { return next_layer_; }

    /// Returns a reference to the next layer
    NextLayer const& next_layer() const noexcept { return next_layer_; }

    /// Read some data from the stream
    template <class MutableBufferSequence>
    std::size_t read_some(MutableBufferSequence const& buffers) {
        std::cout << "sse_stream: read_some\n";
        auto const bytes_transferred = next_layer_.read_some(buffers);
        this->decode_and_buffer_lines(buffers);
        return bytes_transferred;
    }

    /// Read some data from the stream
    template <class MutableBufferSequence>
    std::size_t read_some(MutableBufferSequence const& buffers,
                          beast::error_code& ec) {
        std::cout << "sse_stream: read_some\n";
        auto const bytes_transferred = next_layer_.read_some(buffers, ec);
        this->decode_and_buffer_lines(buffers);
        return bytes_transferred;
    }

    template <class MutableBufferSequence,
              class ReadHandler = net::default_completion_token<executor_type>>
    BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
    async_read_some(MutableBufferSequence const& buffers,
                    ReadHandler&& handler =
                        net::default_completion_token<executor_type>{}) {
        std::cout << "sse_stream: async_read_some\n";
        return net::async_initiate<ReadHandler,
                                   void(beast::error_code, std::size_t)>(
            run_read_op{}, handler, this, buffers);
    }

    /// Write some data to the stream asynchronously
    template <class ConstBufferSequence,
              class WriteHandler = net::default_completion_token<executor_type>>
    BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
    async_write_some(ConstBufferSequence const& buffers,
                     WriteHandler&& handler =
                         net::default_completion_token_t<executor_type>{}) {
        std::cout << "sse_stream: async_write_some\n";
        return net::async_initiate<WriteHandler,
                                   void(beast::error_code, std::size_t)>(
            run_write_op{}, handler, this, buffers);
    }
};

}  // namespace launchdarkly::sse::detail
