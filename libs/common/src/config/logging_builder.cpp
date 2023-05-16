#include <launchdarkly/config/shared/builders/logging_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>

namespace launchdarkly::config::shared::builders {

built::Logging LoggingBuilder::Build() const {
    return std::visit(
        [](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, LoggingBuilder::BasicLogging>) {
                return built::Logging{false, nullptr, arg.tag_, arg.level_};
            } else if constexpr (std::is_same_v<
                                     T, LoggingBuilder::CustomLogging>) {
                if (arg.backend_) {
                    return built::Logging{false, arg.backend_,
                                          Defaults<AnySDK>::LogTag(),
                                          Defaults<AnySDK>::LogLevel()};
                }
                // No back-end set. Return a default config.
                return built::Logging{false, nullptr,
                                      Defaults<AnySDK>::LogTag(),
                                      Defaults<AnySDK>::LogLevel()};
            } else if constexpr (std::is_same_v<T, LoggingBuilder::NoLogging>) {
                return built::Logging{true, nullptr, Defaults<AnySDK>::LogTag(),
                                      Defaults<AnySDK>::LogLevel()};
            }
        },
        logging_);
}

LoggingBuilder& LoggingBuilder::Logging(
    std::variant<LoggingBuilder::BasicLogging,
                 LoggingBuilder::CustomLogging,
                 LoggingBuilder::NoLogging> logging) {
    logging_ = logging;
    return *this;
}

LoggingBuilder::LoggingBuilder(LoggingBuilder::CustomLogging custom) {
    Logging(custom);
}

LoggingBuilder::LoggingBuilder(LoggingBuilder::BasicLogging basic) {
    Logging(basic);
}

LoggingBuilder::LoggingBuilder(LoggingBuilder::NoLogging no) {
    Logging(no);
}

LoggingBuilder::BasicLogging& LoggingBuilder::BasicLogging::Level(
    LogLevel level) {
    level_ = level;
    return *this;
}

LoggingBuilder::BasicLogging& LoggingBuilder::BasicLogging::Tag(
    std::string tag) {
    tag_ = std::move(tag);
    return *this;
}
LoggingBuilder::BasicLogging::BasicLogging()
    : level_(Defaults<AnySDK>::LogLevel()), tag_(Defaults<AnySDK>::LogTag()) {}

LoggingBuilder::CustomLogging& LoggingBuilder::CustomLogging::Backend(
    std::shared_ptr<ILogBackend> backend) {
    backend_ = backend;
    return *this;
}

}  // namespace launchdarkly::config::shared::builders
