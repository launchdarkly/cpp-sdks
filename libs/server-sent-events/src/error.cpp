#include <launchdarkly/sse/error.hpp>

#include <sstream>

namespace launchdarkly::sse {
namespace errors {

std::ostream& operator<<(std::ostream& out, NoContent const&) {
    out << "no content, will not attempt to reconnect (HTTP 204)";
    return out;
}

std::ostream& operator<<(std::ostream& out,
                         InvalidRedirectLocation const& invalid) {
    out << "server responded with an invalid redirect (" << invalid.location
        << ")";
    return out;
}

std::ostream& operator<<(std::ostream& out, NotRedirectable const&) {
    out << "cannot follow server redirect";
    return out;
}

std::ostream& operator<<(std::ostream& out, ReadTimeout const& err) {
    out << "timed out reading response body (exceeded " << err.timeout->count()
        << "ms)";
    return out;
}

std::ostream& operator<<(std::ostream& out,
                         UnrecoverableClientError const& err) {
    if (err.status == boost::beast::http::status::unauthorized ||
        err.status == boost::beast::http::status::forbidden) {
        out << "invalid auth key (HTTP " << static_cast<int>(err.status) << ")";

    } else {
        out << "unrecoverable client-side error (HTTP "
            << static_cast<int>(err.status) << ")";
    }
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

}  // namespace launchdarkly::sse
