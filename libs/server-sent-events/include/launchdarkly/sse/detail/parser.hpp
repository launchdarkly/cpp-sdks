#pragma once

#include <launchdarkly/sse/event.hpp>

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http/basic_parser.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/type_traits.hpp>

#include <deque>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace launchdarkly::sse::detail {

using namespace boost::beast;

struct Event {
    std::string type;
    std::string data;
    std::optional<std::string> id;

    Event() = default;

    void append_data(std::string const&);
    void trim_trailing_newline();
};

template <class EventReceiver>
struct EventBody {
    using event_type = EventReceiver;
    class reader;
    class value_type;
};

template <class EventReceiver>
class EventBody<EventReceiver>::value_type {
    friend class reader;
    friend struct EventBody;

    EventReceiver events_;

   public:
    void on_event(EventReceiver&& receiver) { events_ = std::move(receiver); }
};

template <class EventReceiver>
struct EventBody<EventReceiver>::reader {
    value_type& body_;

    std::optional<std::string> buffered_line_;
    std::deque<std::string> complete_lines_;
    bool begin_CR_;
    std::optional<std::string> last_event_id_;
    std::optional<Event> event_;

   public:
    template <bool isRequest, class Fields>
    reader(http::header<isRequest, Fields>& h, value_type& body)
        : body_(body),
          buffered_line_(),
          complete_lines_(),
          begin_CR_(false),
          last_event_id_(),
          event_() {
        boost::ignore_unused(h);
    }

    /** Initialize the reader.

    This is called after construction and before the first
    call to `put`. The message is valid and complete upon
    entry.@param ec Set to the error, if any occurred.
    */
    void init(boost::optional<std::uint64_t> const& content_length,
              error_code& ec) {
        boost::ignore_unused(content_length);

        // The specification requires this to indicate "no error"
        ec = {};
    }

    /** Store buffers.
    This is called zero or more times with parsed body octets.

    @param buffers The constant buffer sequence to store.

    @param ec Set to the error, if any occurred.

    @return The number of bytes transferred from the input buffers.
    */
    template <class ConstBufferSequence>
    std::size_t put(ConstBufferSequence const& buffers, error_code& ec) {
        // The specification requires this to indicate "no error"
        ec = {};
        parse_stream(buffers_to_string(buffers));
        parse_events();
        return buffer_bytes(buffers);
    }

    /** Called when the body is complete.

        @param ec Set to the error, if any occurred.
    */
    void finish(error_code& ec) {
        // The specification requires this to indicate "no error"
        ec = {};
    }

   private:
    void complete_line() {
        if (buffered_line_.has_value()) {
            complete_lines_.push_back(buffered_line_.value());
            buffered_line_.reset();
        }
    }

    size_t append_up_to(boost::string_view body, std::string const& search) {
        std::size_t index = body.find_first_of(search);
        if (index != std::string::npos) {
            body.remove_suffix(body.size() - index);
        }
        if (buffered_line_.has_value()) {
            buffered_line_->append(body.to_string());
        } else {
            buffered_line_ = std::string{body};
        }
        return index == std::string::npos ? body.size() : index;
    }

    void parse_stream(boost::string_view body) {
        size_t i = 0;
        while (i < body.length()) {
            i += this->append_up_to(body.substr(i, body.length() - i), "\r\n");
            if (i == body.size()) {
                continue;
            } else if (body.at(i) == '\r') {
                complete_line();
                begin_CR_ = true;
                i++;
            } else if (body.at(i) == '\n') {
                if (begin_CR_) {
                    begin_CR_ = false;
                    i++;
                } else {
                    complete_line();
                    i++;
                }
            } else {
                begin_CR_ = false;
            }
        }
    }

    static std::pair<std::string, std::string> parse_field(std::string field) {
        if (field.empty()) {
            assert(0 && "should never parse an empty line");
        }

        size_t colon_index = field.find(':');
        switch (colon_index) {
            case 0:
                field.erase(0, 1);
                return std::make_pair(std::string{"comment"}, std::move(field));
            case std::string::npos:
                return std::make_pair(std::move(field), std::string{});
            default:
                auto key = field.substr(0, colon_index);
                field.erase(0, colon_index + 1);
                if (field.find(' ') == 0) {
                    field.erase(0, 1);
                }
                return std::make_pair(std::move(key), std::move(field));
        }
    }

    void parse_events() {
        while (true) {
            bool seen_empty_line = false;

            while (!complete_lines_.empty()) {
                std::string line = std::move(complete_lines_.front());
                complete_lines_.pop_front();

                if (line.empty()) {
                    if (event_.has_value()) {
                        seen_empty_line = true;
                        break;
                    }
                    continue;
                }

                auto field = parse_field(std::move(line));
                if (field.first == "comment") {
                    body_.events_(
                        launchdarkly::sse::Event("comment", field.second));
                    continue;
                }

                if (!event_.has_value()) {
                    event_.emplace(Event{});
                    event_->id = last_event_id_;
                }

                if (field.first == "event") {
                    event_->type = field.second;
                } else if (field.first == "data") {
                    event_->append_data(field.second);
                } else if (field.first == "id") {
                    if (field.second.find('\0') != std::string::npos) {
                        // IDs with null-terminators are acceptable, but
                        // ignored.
                        continue;
                    }
                    last_event_id_ = field.second;
                    event_->id = last_event_id_;
                } else if (field.first == "retry") {
                }
            }

            if (seen_empty_line) {
                std::optional<Event> data = event_;
                event_ = std::nullopt;

                if (data.has_value()) {
                    data->trim_trailing_newline();
                    body_.events_(launchdarkly::sse::Event(
                        data->type, data->data, data->id));
                    data.reset();
                }

                continue;
            }

            break;
        }
    }
};

template <bool isRequest, class EventReceiver, class Fields>
std::ostream& operator<<(
    std::ostream&,
    http::message<isRequest, EventBody<EventReceiver>, Fields> const&) = delete;

}  // namespace launchdarkly::sse::detail
