#pragma once

namespace launchdarkly::client_side {

class IDataSource {
   public:
    virtual void start() = 0;
    virtual void close() = 0;
};

}  // namespace launchdarkly::client_side
