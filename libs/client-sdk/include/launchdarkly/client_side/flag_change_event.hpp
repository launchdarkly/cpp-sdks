#pragma once

#include <launchdarkly/value.hpp>

namespace launchdarkly::client_side::flag_manager::detail {
/**
 * A notification, for the consumer of the SDK, that a flag value has
 * changed.
 */
class FlagValueChangeEvent {
   public:
    /**
     * The name of the flag that changed.
     * @return The name of the flag.
     */
    [[nodiscard]] std::string const& FlagName() const;

    /**
     * Get the new value. If there was not an new value, because the flag was
     * deleted, then the Value will be of a null type. Check the Deleted method
     * to see if a flag was deleted.
     *
     * @return The new value.
     */
    [[nodiscard]] Value const& NewValue() const;

    /**
     * Get the old value. If there was not an old value, for instance a newly
     * created flag, then the Value will be of a null type.
     *
     * @return The new value.
     */
    [[nodiscard]] Value const& OldValue() const;

    /**
     * Will be true if the flag was deleted. In which case the NewValue will
     * be of a null type.
     *
     * @return True if the flag was deleted.
     */
    [[nodiscard]] bool Deleted() const;

    /**
     * Construct a flag changed event with a new and old value.
     *
     * This is a change event for a flag that has not been Deleted.
     *
     * @param new_value The new value.
     * @param old_value The old value.
     */
    FlagValueChangeEvent(std::string name, Value new_value, Value old_value);
    FlagValueChangeEvent(std::string name, Value old_value);

   private:
    Value new_value_;
    Value old_value_;
    bool deleted_;
    std::string flag_name_;
};

std::ostream& operator<<(std::ostream& out, FlagValueChangeEvent const& event);

}  // namespace launchdarkly::client_side::flag_manager::detail
