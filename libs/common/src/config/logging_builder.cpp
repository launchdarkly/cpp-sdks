#include <launchdarkly/config/shared/builders/logging_bulder.hpp>

namespace launchdarkly::config::shared::builders {

built::Logging LoggingBuilder::Build() const {
    return built::Logging();
}

LoggingBuilder& LoggingBuilder::Logging(
    std::variant<LoggingBuilder::BasicLogging,
                 LoggingBuilder::CustomLogging,
                 LoggingBuilder::NoLogging> logging) {
    logging_ = logging;
    return *this;
}

LoggingBuilder::BasicLogging& LoggingBuilder::BasicLogging::Level(
    LogLevel level) {
    level_ = level;
    return *this;
}

LoggingBuilder::BasicLogging& LoggingBuilder::BasicLogging::Name(
    std::string name) {
    name_ = std::move(name);
    return *this;
}

LoggingBuilder::CustomLogging& LoggingBuilder::CustomLogging::Backend(
    std::shared_ptr<ILogBackend> backend) {
    backend_ = backend;
    return *this;
}

}  // namespace launchdarkly::config::shared::builders