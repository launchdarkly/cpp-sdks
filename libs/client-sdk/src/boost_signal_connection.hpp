#pragma once

#include <boost/signals2.hpp>

#include "launchdarkly/connection.hpp"

namespace launchdarkly::client_side {

class SignalConnection : public IConnection {
   public:
    friend class FlagUpdater;
    SignalConnection(boost::signals2::connection connection);
    void Disconnect() override;

   private:
    boost::signals2::connection connection_;
};

}  // namespace launchdarkly::client_side
