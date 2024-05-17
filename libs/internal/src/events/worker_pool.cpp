#include <memory>

#include <launchdarkly/events/detail/request_worker.hpp>
#include <launchdarkly/events/detail/worker_pool.hpp>

namespace launchdarkly::events::detail {

using namespace launchdarkly::config::shared::built;

std::optional<std::locale> GetLocale(std::string const& locale,
                                     std::string const& tag,
                                     Logger& logger) {
    try {
        return std::locale(locale);
    } catch (std::runtime_error) {
        LD_LOG(logger, LogLevel::kWarn)
            << tag << " couldn't load " << locale
            << " locale. If debug events are enabled, they may be emitted for "
               "longer than expected";
        return std::nullopt;
    }
}

WorkerPool::WorkerPool(boost::asio::any_io_executor io,
                       std::size_t pool_size,
                       std::chrono::milliseconds delivery_retry_delay,
                       TlsOptions tls_options,
                       Logger& logger)
    : io_(io), workers_() {
    // The en_US.utf-8 locale is used whenever a date is parsed from the HTTP
    // headers returned by the event-delivery endpoints. If the locale is
    // unavailable, then the workers will skip the parsing step.
    //
    // This may result in debug events being emitted for longer than expected
    // if the host's time is way out of sync.
    std::optional<std::locale> date_header_locale =
        GetLocale("en_US.utf-8", "event-processor", logger);

    for (std::size_t i = 0; i < pool_size; i++) {
        workers_.emplace_back(std::make_unique<RequestWorker>(
            io_, delivery_retry_delay, i, date_header_locale,
            tls_options.PeerVerifyMode(), logger));
    }
}

}  // namespace launchdarkly::events::detail
