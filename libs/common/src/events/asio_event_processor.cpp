#include "events/detail/asio_event_processor.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/use_future.hpp>
#include <chrono>

namespace launchdarkly::events {

AsioEventProcessor::AsioEventProcessor(
    boost::asio::any_io_executor const& executor,
    config::detail::Events config,
    Logger& logger)
    : inbox_(),
      io_(boost::asio::make_strand(executor)),
      flush_timer_(io_),
      logger_(logger),
      shutdown_(false) {
    do_flush();
}

void AsioEventProcessor::async_send(InputEvent event) {
    boost::asio::post(io_, [this]() { inbox_.emplace(SendCommand()); });
}

void AsioEventProcessor::async_flush() {
    LD_LOG(logger_, LogLevel::kDebug) << "dispatcher: flush requested";
    boost::asio::post(io_, [this]() {
        flush_timer_.expires_from_now(std::chrono::seconds(0));
    });
}

void AsioEventProcessor::sync_close() {
    auto future = boost::asio::post(io_, std::packaged_task<void()>([this]() {
                                        LD_LOG(logger_, LogLevel::kDebug)
                                            << "dispatcher: shutdown signal";
                                        shutdown_ = true;
                                        flush_timer_.cancel();
                                        flush_timer_.wait();
                                        LD_LOG(logger_, LogLevel::kDebug)
                                            << "dispatcher: shutdown complete";
                                    }));
    future.wait();
}

void AsioEventProcessor::do_flush() {
    flush_timer_.expires_from_now(std::chrono::seconds(10));
    flush_timer_.async_wait(
        [this](boost::system::error_code ec) { this->flush(ec); });
}

void AsioEventProcessor::flush(boost::system::error_code ec) {
    if (shutdown_) {
        LD_LOG(logger_, LogLevel::kDebug) << "dispatcher: shutting down";
        return;
    }
    if (!ec || ec == boost::asio::error::operation_aborted) {
        LD_LOG(logger_, LogLevel::kInfo) << "dispatcher: flushing";
        do_flush();
        return;
    }
    LD_LOG(logger_, LogLevel::kError) << "dispatcher: " << ec;
}
}  // namespace launchdarkly::events
