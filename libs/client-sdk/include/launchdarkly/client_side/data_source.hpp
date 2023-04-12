#pragma once

namespace launchdarkly::client_side {

class IDataSource {
   public:
    virtual void start() = 0;
    virtual void close() = 0;

    virtual ~IDataSource() = default;
    IDataSource(IDataSource const& item) = delete;
    IDataSource(IDataSource&& item) = delete;
    IDataSource& operator=(IDataSource const&) = delete;
    IDataSource& operator=(IDataSource&&) = delete;

   protected:
    IDataSource() = default;
};

}  // namespace launchdarkly::client_side
