#include <launchdarkly/sse/error.hpp>

#include <sstream>

namespace launchdarkly::sse {
namespace errors {

std::ostream& operator<<(std::ostream& out,
                         InvalidRedirectLocation const& invalid) {
    out << "received invalid redirect from server, cannot follow ("
        << invalid.location << ") - giving up permanently";
    return out;
}

std::ostream& operator<<(std::ostream& out, NotRedirectable const&) {
    out << "received malformed redirect from server, cannot follow - giving up "
           "permanently";
    return out;
}

std::ostream& operator<<(std::ostream& out, ReadTimeout const& err) {
    out << "timed out reading response body (exceeded " << err.timeout->count()
        << "ms) - will retry";
    return out;
}

std::ostream& operator<<(std::ostream& out,
                         UnrecoverableClientError const& err) {
    std::string explanation;
    if (err.status == boost::beast::http::status::unauthorized ||
        err.status == boost::beast::http::status::forbidden) {
        explanation = " (invalid auth key)";
    }
    out << "received HTTP error " << static_cast<int>(err.status) << explanation
        << " for streaming connection - giving up "
           "permanently";
    return out;
}
}  // namespace errors

std::ostream& operator<<(std::ostream& out, Error const& error) {
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            out << arg;
        },
        error);
    return out;
}

std::string ErrorToString(Error const& error) {
    std::stringstream ss;
    ss << error;
    return ss.str();
}

bool IsRecoverable(Error const& error) {
    return std::holds_alternative<errors::ReadTimeout>(error);
}

}  // namespace launchdarkly::sse
