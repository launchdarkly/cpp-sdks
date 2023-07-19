#pragma once

#include <optional>
#include <ostream>
#include <optional>

namespace launchdarkly::server_side::evaluation {

class Error {
   public:
    static Error CyclicSegmentReference(std::string segment_key);
    static Error CyclicPrerequisiteReference(std::string prereq_key);
    static Error InvalidAttributeReference(std::string ref);
    static Error RolloutMissingVariations();
    static Error NonexistentVariationIndex(std::int64_t index);
    static Error MissingSalt(std::string item_key);

    friend std::ostream& operator<<(std::ostream& out, Error const& arr);
    friend bool operator==(Error const& lhs, Error const& rhs);

   private:
    Error(char const* format, std::string arg);
    Error(char const* format, std::int64_t arg);
    Error(char const* msg);

    char const* format_;
    std::optional<std::string> arg_;
};

}  // namespace launchdarkly::server_side::evaluation
