#pragma once

#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <functional>
#include <optional>
#include <string>

namespace launchdarkly::server_side::data_interfaces {

/**
 * \brief IPushSource obtains data via a push synchronization mechanism,
 * updating a local cache whenever changes are made upstream.
 */
class IPushSource {
   public:
    /**
     * \brief Initialize the source, optionally with an initial data set. Init
     * will be called before Start.
     * \param initial_data Initial set of SDK data.
     */
    virtual void Init(std::optional<data_model::SDKDataSet> initial_data) = 0;

    /**
     * \brief Starts the synchronization mechanism. Start will be called only
     * once after Init; the source is responsible for maintaining a persistent
     * connection. Start should not block.
     */
    virtual void StartAsync() = 0;

    /**
     * \brief Stops the synchronization mechanism. Stop will be called only once
     * after StartAsync. Stop should not block, but should invoke the completion
     * function once shutdown.
     * \param complete A callback to be invoked on completion.
     */
    virtual void ShutdownAsync(std::function<void()> complete) = 0;

    /**
     * \return Identity of the source. Used in logs.
     */
    [[nodiscard]] virtual std::string const& Identity() const = 0;

    virtual ~IPushSource() = default;
    IPushSource(IPushSource const& item) = delete;
    IPushSource(IPushSource&& item) = delete;
    IPushSource& operator=(IPushSource const&) = delete;
    IPushSource& operator=(IPushSource&&) = delete;

   protected:
    IPushSource() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
