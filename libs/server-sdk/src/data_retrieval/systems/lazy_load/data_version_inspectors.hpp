#pragma once

#include <launchdarkly/persistence/persistent_store_core.hpp>

#include <string>

namespace launchdarkly::server_side::data_retrieval {
class SegmentKind : public persistence::IPersistentKind {
   public:
    std::string const& Namespace() const override;
    uint64_t Version(std::string const& data) const override;

    ~SegmentKind() override = default;

   private:
    static inline std::string const namespace_ = "segments";
};

class FlagKind : public persistence::IPersistentKind {
   public:
    std::string const& Namespace() const override;
    uint64_t Version(std::string const& data) const override;

    ~FlagKind() override = default;

   private:
    static inline std::string const namespace_ = "features";
};

struct Kinds {
    static FlagKind const Flag;
    static SegmentKind const Segment;
};
}  // namespace launchdarkly::server_side::data_retrieval
