#include "launchdarkly/client_side/flag_manager/detail/flag_change_event.hpp"

namespace launchdarkly::client_side::flag_manager::detail {

std::string const& FlagValueChangeEvent::flag_name() const {
    return flag_name_;
}

Value const& FlagValueChangeEvent::new_value() const {
    return new_value_;
}

Value const& FlagValueChangeEvent::old_value() const {
    return old_value_;
}

bool FlagValueChangeEvent::deleted() const {
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

}  // namespace launchdarkly::client_side::flag_manager::detail
