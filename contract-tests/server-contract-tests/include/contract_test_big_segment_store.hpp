#pragma once

#include <launchdarkly/server_side/integrations/big_segments/ibig_segment_store.hpp>

#include <nlohmann/json.hpp>

#include <tl/expected.hpp>

#include <string>

/**
 * A Big Segment store that delegates membership and metadata lookups to the
 * contract-test harness over HTTP, mirroring how ContractTestHook posts to the
 * harness. Each lookup is a synchronous request/response (unlike the hook's
 * fire-and-forget) because IBigSegmentStore must return the result.
 *
 * Thread-safe: holds only the immutable callback URI and performs each request
 * on its own local io_context.
 */
class ContractTestBigSegmentStore
    : public launchdarkly::server_side::integrations::IBigSegmentStore {
   public:
    explicit ContractTestBigSegmentStore(std::string callback_uri);

    [[nodiscard]] GetMembershipResult GetMembership(
        std::string const& context_hash) const noexcept override;

    [[nodiscard]] GetMetadataResult GetMetadata() const noexcept override;

   private:
    // Synchronous POST of `body` to `<callback_uri><path>`. Returns the parsed
    // JSON response on HTTP 200, or an error string (non-200 body, transport
    // failure, or parse failure).
    [[nodiscard]] tl::expected<nlohmann::json, std::string> Post(
        std::string const& path,
        nlohmann::json const& body) const noexcept;

    std::string const callback_uri_;
};
