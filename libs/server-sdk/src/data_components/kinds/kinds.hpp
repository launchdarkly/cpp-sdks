#pragma once

#include <launchdarkly/server_side/integrations/serialized_descriptors.hpp>

namespace launchdarkly::server_side::data_components {

class SegmentKind final : public integrations::IPersistentKind {
   public:
    std::string const& Namespace() const override;
    std::uint64_t Version(std::string const& data) const override;

    ~SegmentKind() override = default;

   private:
    static inline std::string const namespace_ = "segments";
};

class FlagKind final : public integrations::IPersistentKind {
   public:
    std::string const& Namespace() const override;
    std::uint64_t Version(std::string const& data) const override;

    ~FlagKind() override = default;

   private:
    static inline std::string const namespace_ = "features";
};
}  // namespace launchdarkly::server_side::data_components