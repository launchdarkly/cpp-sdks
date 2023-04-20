#pragma once

namespace launchdarkly::client_side {
// TODO: Can be moved to common.

/**
 * Represents the connection of a listener.
 * Disconnecting the connection will cause the listener to stop receiving
 * events.
 */
class IConnection {
   public:
    /**
     * Disconnect the listener and stop receiving events.
     */
    virtual void Disconnect() = 0;

    virtual ~IConnection() = default;
    IConnection(IConnection const& item) = delete;
    IConnection(IConnection&& item) = delete;
    IConnection& operator=(IConnection const&) = delete;
    IConnection& operator=(IConnection&&) = delete;

   protected:
    IConnection() = default;
};
}  // namespace launchdarkly::client_side
