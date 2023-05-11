#include <launchdarkly/client_side/flag_manager/detail/flag_change_event.hpp>

namespace launchdarkly::client_side::flag_manager::detail {

std::string const& FlagValueChangeEvent::FlagName() const {
    return flag_name_;
}

Value const& FlagValueChangeEvent::NewValue() const {
    return new_value_;
}

Value const& FlagValueChangeEvent::OldValue() const {
    return old_value_;
}

bool FlagValueChangeEvent::Deleted() const {
    return deleted_;
}

FlagValueChangeEvent::FlagValueChangeEvent(std::string name,
                                           Value new_value,
                                           Value old_value)
    : flag_name_(std::move(name)),
      new_value_(std::move(new_value)),
      old_value_(std::move(old_value)),
      deleted_(false) {}

FlagValueChangeEvent::FlagValueChangeEvent(std::string name, Value old_value)
    : flag_name_(std::move(name)),
      old_value_(std::move(old_value)),
      deleted_(true) {}

std::ostream& operator<<(std::ostream& out, FlagValueChangeEvent const& event) {
    if (event.Deleted()) {
        out << "Event(name: " << event.FlagName() << " deleted)";
    } else {
        out << "Event(name: " << event.FlagName()
            << ", old value: " << event.OldValue()
            << ", new value: " << event.NewValue() << ")";
    }
    return out;
}

}  // namespace launchdarkly::client_side::flag_manager::detail
