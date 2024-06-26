#pragma once
#include <chrono>
#include <optional>
#include <ostream>
#include <variant>

#include <boost/beast/http/status.hpp>

namespace launchdarkly::sse {
namespace errors {

struct NoContent {};
std::ostream& operator<<(std::ostream& out, NoContent const&);

struct InvalidRedirectLocation {
    std::string location;
};
std::ostream& operator<<(std::ostream& out, InvalidRedirectLocation const&);

struct NotRedirectable {};
std::ostream& operator<<(std::ostream& out, NotRedirectable const&);

struct ReadTimeout {
    std::optional<std::chrono::milliseconds> timeout;
};
std::ostream& operator<<(std::ostream& out, ReadTimeout const&);

struct UnrecoverableClientError {
    boost::beast::http::status status;
};
std::ostream& operator<<(std::ostream& out, UnrecoverableClientError const&);

}  // namespace errors

using Error = std::variant<errors::NoContent,
                           errors::InvalidRedirectLocation,
                           errors::NotRedirectable,
                           errors::ReadTimeout,
                           errors::UnrecoverableClientError>;

std::ostream& operator<<(std::ostream& out, Error const& error);

std::string ErrorToString(Error const& error);
}  // namespace launchdarkly::sse
