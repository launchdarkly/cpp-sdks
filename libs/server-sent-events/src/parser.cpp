#include <launchdarkly/sse/parser.hpp>

namespace launchdarkly::sse {
event_data::event_data() : m_type{}, m_data{}, m_id{} {}

void event_data::set_type(std::string type) {
    m_type = std::move(type);
}
void event_data::append_data(std::string const& data) {
    m_data.append(data);
    m_data.append("\n");
}

void event_data::set_id(std::optional<std::string> id) {
    m_id = std::move(id);
}

std::string const& event_data::get_type() {
    return m_type;
}
std::string const& event_data::get_data() {
    return m_data;
}

void event_data::trim_trailing_newline() {
    if (m_data[m_data.size() - 1] == '\n') {
        m_data.resize(m_data.size() - 1);
    }
}

std::optional<std::string> const& event_data::get_id() {
    return m_id;
}
}  // namespace launchdarkly::sse
