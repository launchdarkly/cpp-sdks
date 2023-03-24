#include <launchdarkly/sse/detail/parser.hpp>

namespace launchdarkly::sse::detail {

Event::Event() : type("message"), data(), id() {}

void Event::append_data(std::string const& input) {
    data.append(input);
    data.append("\n");
}

void Event::trim_trailing_newline() {
    if (data[data.size() - 1] == '\n') {
        data.resize(data.size() - 1);
    }
}

}  // namespace launchdarkly::sse::detail
