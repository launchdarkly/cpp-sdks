#pragma once

#include <launchdarkly/events/event_processor_interface.hpp>

#include <gtest/gtest-assertion-result.h>

namespace launchdarkly {
class SpyEventProcessor : public events::IEventProcessor {
   public:
    struct Flush {};
    struct Shutdown {};

    using Record = events::InputEvent;

    SpyEventProcessor() : events_() {}

    void SendAsync(events::InputEvent event) override {
        events_.push_back(std::move(event));
    }

    void FlushAsync() override {}

    void ShutdownAsync() override {}

    /**
     * Asserts that 'count' events were recorded.
     * @param count Number of expected events.
     */
    [[nodiscard]] testing::AssertionResult Count(std::size_t count) const {
        if (events_.size() == count) {
            return testing::AssertionSuccess();
        }
        return testing::AssertionFailure()
               << "Expected " << count << " events, got " << events_.size();
    }

    template <typename T>
    [[nodiscard]] testing::AssertionResult Kind(std::size_t index) const {
        return GetIndex(index, [&](auto const& actual) {
            if (std::holds_alternative<T>(actual)) {
                return testing::AssertionSuccess();
            } else {
                return testing::AssertionFailure()
                       << "Expected message " << index << " to be of kind "
                       << typeid(T).name() << ", got variant index "
                       << actual.index();
            }
        });
    }

   private:
    [[nodiscard]] testing::AssertionResult GetIndex(
        std::size_t index,
        std::function<testing::AssertionResult(Record const&)> const& f) const {
        if (index >= events_.size()) {
            return testing::AssertionFailure()
                   << "Event index " << index << " out of range";
        }
        auto const& record = events_[index];
        return f(record);
    }
    using Records = std::vector<Record>;
    Records events_;
};
}  // namespace launchdarkly
