#pragma once
#include <future>
namespace launchdarkly::data_source {

class IDataSource {
   public:
    virtual void Start() = 0;
    virtual void ShutdownAsync(std::function<void()>) = 0;
    virtual ~IDataSource() = default;
    IDataSource(IDataSource const& item) = delete;
    IDataSource(IDataSource&& item) = delete;
    IDataSource& operator=(IDataSource const&) = delete;
    IDataSource& operator=(IDataSource&&) = delete;

   protected:
    IDataSource() = default;
};

}  // namespace launchdarkly::client_side
