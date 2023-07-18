#pragma once

#include "data_store/data_store.hpp"

#include <memory>

namespace launchdarkly::server_side::test_store {

/**
 * @return A data store preloaded with flags/segments for unit tests.
 */
std::unique_ptr<data_store::IDataStore> TestData();

/**
 * @return An initialized, but empty, data store.
 */
std::unique_ptr<data_store::IDataStore> Empty();

}  // namespace launchdarkly::server_side::test_store
