#pragma once

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <string>

namespace launchdarkly::server_side::data_interfaces {

/**
 * \brief IDestination represents a sink for data received by the
 * SDK. A destination may be a database, local file, etc.
 */
class IDestination {
   public:
    /**
     * \brief Initialize the destination with a base set of data.
     * \param data_set The initial data received by the SDK.
     */
    virtual void Init(data_model::SDKDataSet data_set) = 0;

    /**
     * \brief Upsert a flag named by key.
     * \param key Flag key.
     * \param flag Flag descriptor.
     */
    virtual void Upsert(std::string const& key,
                        data_model::FlagDescriptor flag) = 0;

    /**
     * \brief Upsert a segment named by key.
     * \param key Segment key.
     * \param segment Segment descriptor.
     */
    virtual void Upsert(std::string const& key,
                        data_model::SegmentDescriptor segment) = 0;

    /**
     * \return Identity of the destination. Used in logs.
     */
    [[nodiscard]] virtual std::string const& Identity() const = 0;

    IDestination(IDestination const& item) = delete;
    IDestination(IDestination&& item) = delete;
    IDestination& operator=(IDestination const&) = delete;
    IDestination& operator=(IDestination&&) = delete;
    virtual ~IDestination() = default;

   protected:
    IDestination() = default;
};
}  // namespace launchdarkly::server_side::data_interfaces
