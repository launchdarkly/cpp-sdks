#include "data_store_updater.hpp"

namespace launchdarkly::server_side::data_store {

std::unique_ptr<IConnection>
DataStoreUpdater::OnFlagChange(
    launchdarkly::server_side::IChangeNotifier::ChangeHandler handler) {
    return std::unique_ptr<IConnection>();
}

void DataStoreUpdater::Init(
    launchdarkly::data_model::SDKDataSet dataSet) {

    if(HasListeners()) {
        // TODO: Calculate changes.
    }

    sink_->Init(dataSet);
}

void DataStoreUpdater::Upsert(
    std::string key,
    launchdarkly::server_side::data_source::IDataSourceUpdateSink::
        FlagDescriptor flag) {
    auto existing = store_->GetFlag(key);
    if(existing && (existing->version > flag.version)) {
        // Out of order update, ignore it.
        return;
    }

    if(HasListeners()) {

    }

    sink_->Upsert(key, flag);
}

void DataStoreUpdater::Upsert(
    std::string key,
    launchdarkly::server_side::data_source::IDataSourceUpdateSink::
        SegmentDescriptor segment) {
    auto existing = store_->GetSegment(key);
    if(existing && (existing->version > segment.version)) {
        // Out of order update, ignore it.
        return;
    }

    if(HasListeners()) {

    }

    sink_->Upsert(key, segment);
}

bool DataStoreUpdater::HasListeners() const {
    std::lock_guard lock{signal_mutex_};
    return !signals_.empty();
}


}  // namespace launchdarkly::server_side::data_store
