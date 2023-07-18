#pragma once

#include <launchdarkly/logging/log_backend.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <vector>

#include <gtest/gtest-assertion-result.h>

namespace launchdarkly::logging {

class SpyLoggerBackend : public launchdarkly::ILogBackend {
   public:
    using Record = std::pair<LogLevel, std::string>;

    SpyLoggerBackend() : messages_() {}

    /**
     * Always returns true.
     */
    bool Enabled(LogLevel level) noexcept override { return true; }

    /**
     * Records the message internally.
     */
    void Write(LogLevel level, std::string message) noexcept override {
        messages_.push_back({level, std::move(message)});
    }

    /**
     * Asserts that 'count' messages were recorded.
     * @param count Number of expected messages.
     */
    testing::AssertionResult Count(std::size_t count) const {
        if (messages_.size() == count) {
            return testing::AssertionSuccess();
        }
        return testing::AssertionFailure()
               << "Expected " << count << " messages, got " << messages_.size();
    }

    testing::AssertionResult Equals(std::size_t index,
                                    LogLevel level,
                                    std::string const& expected) const {
        return GetIndex(index, level, [&](auto const& actual) {
            if (actual.second != expected) {
                return testing::AssertionFailure()
                       << "Expected message " << index << " to be " << expected
                       << ", got " << actual.second;
            }
            return testing::AssertionSuccess();
        });
    }

    testing::AssertionResult Contains(std::size_t index,
                                      LogLevel level,
                                      std::string const& expected) const {
        return GetIndex(
            index, level, [&](auto const& actual) -> testing::AssertionResult {
                if (actual.second.find(expected) != std::string::npos) {
                    return testing::AssertionSuccess();
                }
                return testing::AssertionFailure()
                       << "Expected message " << index << " to contain "
                       << expected << ", got " << actual.second;
            });
    }

   private:
    testing::AssertionResult GetIndex(
        std::size_t index,
        LogLevel level,
        std::function<testing::AssertionResult(Record const&)> const& f) const {
        if (index >= messages_.size()) {
            return testing::AssertionFailure()
                   << "Message index " << index << " out of range";
        }
        auto const& record = messages_[index];
        if (level != record.first) {
            return testing::AssertionFailure()
                   << "Expected message " << index << " to be " << level
                   << ", got " << record.first;
        }
        return f(record);
    }
    using Records = std::vector<Record>;
    Records messages_;
};

}  // namespace launchdarkly::logging
