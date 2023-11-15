#include <launchdarkly/config/shared/built/data_source_config.hpp>

namespace launchdarkly::config::shared::built {

RedisPullConfig::ConnectionOpts::ConnectionOpts()
    : host_(), port_(0), password_(), db_(0) {}

RedisPullConfig::ConnectionOpts::ConnectionOpts(std::string host,
                                                std::uint16_t port,
                                                std::string password,
                                                std::uint64_t db)
    : host_(std::move(host)),
      port_(port),
      password_(std::move(password)),
      db_(db) {}

std::string const& RedisPullConfig::ConnectionOpts::Host() const noexcept {
    return host_;
}

std::uint16_t RedisPullConfig::ConnectionOpts::Port() const noexcept {
    return port_;
}

std::string const& RedisPullConfig::ConnectionOpts::Password() const noexcept {
    return password_;
}

std::uint64_t RedisPullConfig::ConnectionOpts::DB() const noexcept {
    return db_;
}

}  // namespace launchdarkly::config::shared::built
