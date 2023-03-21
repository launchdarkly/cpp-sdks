#pragma once
#include <boost/beast.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/optional/optional.hpp>
#include <boost/tokenizer.hpp>
#include <iostream>
#include <vector>

namespace launchdarkly::sse::detail {

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

// A layered stream which implements the SSE protocol.
template <class NextLayer>
class sse_stream {
    NextLayer m_nextLayer;
    boost::optional<std::string> m_bufferedLine;
    bool m_lastCharWasCR;
    std::vector<std::string> m_completeLines;

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
                    stream_.next_layer().async_read_some(buffers,
                                                         std::move(*this));
                }

                void operator()(beast::error_code ec,
                                std::size_t bytes_transferred) {
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
                    stream_.next_layer().async_write_some(buffers,
                                                          std::move(*this));
                }

                void operator()(beast::error_code ec,
                                std::size_t bytes_transferred) {
                    this->complete_now(ec, bytes_transferred);
                }
            };

            op(*stream, std::forward<WriteHandler>(handler), buffers);
        }
    };

    void push_buffered() {
        if (m_bufferedLine) {
            m_completeLines.push_back(*m_bufferedLine);
            m_bufferedLine.reset();
        }
    }

    void append_buffered(std::string token) {
        if (m_bufferedLine) {
            m_bufferedLine->append(token);
        } else {
            m_bufferedLine.emplace(token);
        }
    }

    template <class MutableBufferSequence>
    void decode_and_buffer_lines(MutableBufferSequence const& chunk) {
        boost::char_separator<char> sep{"", "\r\n|\r|\n"};
        boost::tokenizer<boost::char_separator<char>> tokens_all{chunk, sep};

        std::vector<std::string> tokens{std::begin(tokens_all),
                                        std::end(tokens_all)};

        for (auto tok = tokens.begin(); tok != tokens.end(); tok = tok++) {
            if (*tok == "\r" || *tok == "\n" || *tok == "\r\n") {
                this->push_buffered();
            } else {
                this->append_buffered(*tok);
            }
        }
    }

   public:
    using executor_type = beast::executor_type<NextLayer>;

    template <class... Args>
    explicit sse_stream(Args&&... args)
        : m_nextLayer{std::forward<Args>(args)...},
          m_lastCharWasCR{false},
          m_bufferedLine{} {}

    /// Returns an instance of the executor used to submit completion handlers
    executor_type get_executor() noexcept { return m_nextLayer.get_executor(); }

    /// Returns a reference to the next layer
    NextLayer& next_layer() noexcept { return m_nextLayer; }

    /// Returns a reference to the next layer
    NextLayer const& next_layer() const noexcept { return m_nextLayer; }

    /// Read some data from the stream
    template <class MutableBufferSequence>
    std::size_t read_some(MutableBufferSequence const& buffers) {
        auto const bytes_transferred = m_nextLayer.read_some(buffers);
        this->decode_and_buffer_lines(buffers);
        return bytes_transferred;
    }

    /// Read some data from the stream
    template <class MutableBufferSequence>
    std::size_t read_some(MutableBufferSequence const& buffers,
                          beast::error_code& ec) {
        auto const bytes_transferred = m_nextLayer.read_some(buffers, ec);
        this->decode_and_buffer_lines(buffers);
        return bytes_transferred;
    }

    template <class MutableBufferSequence,
              class ReadHandler = net::default_completion_token<executor_type>>
    BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
    async_read_some(MutableBufferSequence const& buffers,
                    ReadHandler&& handler =
                        net::default_completion_token<executor_type>{}) {
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
        return net::async_initiate<WriteHandler,
                                   void(beast::error_code, std::size_t)>(
            run_write_op{}, handler, this, buffers);
    }
};

}  // namespace launchdarkly::sse::detail
