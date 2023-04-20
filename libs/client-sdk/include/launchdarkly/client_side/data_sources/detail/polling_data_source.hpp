#pragma once

#include <chrono>

#include <boost/asio/any_io_executor.hpp>

#include "config/detail/built/http_properties.hpp"
#include "config/detail/built/service_endpoints.hpp"
#include "context.hpp"
#include "data/evaluation_result.hpp"
#include "launchdarkly/client_side/data_source.hpp"
#include "launchdarkly/client_side/data_source_update_sink.hpp"
#include "logger.hpp"

namespace launchdarkly::client_side::data_sources::detail {

class PollingDataSource final : public IDataSource {
    void Start() override;
    void Close() override;
};

}  // namespace launchdarkly::client_side::data_sources::detail
