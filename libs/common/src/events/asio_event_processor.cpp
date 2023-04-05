#include "events/detail/asio_event_processor.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/use_future.hpp>
#include <chrono>

namespace launchdarkly::events {

AsioEventProcessor::AsioEventProcessor(boost::asio::any_io_executor executor,
                                       config::detail::Events config,
                                       Logger& logger)
    : inbox_(), io_(std::move(executor)), flush_timer_(io_), logger_(logger) {
    do_flush();
}

void AsioEventProcessor::async_send(InputEvent event) {}
void AsioEventProcessor::async_flush() {
    flush_timer_.expires_from_now(std::chrono::seconds(10));
}

void AsioEventProcessor::sync_close() {
    auto future = boost::asio::post(io_, std::packaged_task<void()>([this]() {
                                        flush_timer_.cancel();
                                        flush_timer_.wait();
                                    }));
    future.wait();
}

void AsioEventProcessor::do_flush() {
    flush_timer_.expires_from_now(std::chrono::seconds(10));
    flush_timer_.async_wait(
        [this](boost::system::error_code ec) { this->flush(ec); });
}

void AsioEventProcessor::flush(boost::system::error_code ec) {
    if (ec) {
        LD_LOG(logger_, LogLevel::kError) << "dispatcher: " << ec;
        return;
    }
    LD_LOG(logger_, LogLevel::kInfo) << "dispatcher: flushing";
    do_flush();
}
}  // namespace launchdarkly::events
