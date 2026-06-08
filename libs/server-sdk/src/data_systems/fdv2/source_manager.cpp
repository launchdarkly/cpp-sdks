#include "source_manager.hpp"

#include <utility>

namespace launchdarkly::server_side::data_systems {

using data_interfaces::IFDv2Synchronizer;
using data_interfaces::IFDv2SynchronizerFactory;

SourceManager::SourceManager(
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories) {
    synchronizers_.reserve(factories.size());
    for (auto& factory : factories) {
        synchronizers_.push_back(
            SynchronizerFactoryWithState{std::move(factory), State::kAvailable,
                                         /*is_fdv1_fallback=*/false});
    }
}

std::unique_ptr<IFDv2Synchronizer> SourceManager::NextSynchronizer() {
    if (synchronizers_.empty()) {
        current_factory_index_ = -1;
        return nullptr;
    }
    for (std::size_t visited = 0; visited < synchronizers_.size(); ++visited) {
        synchronizer_index_ =
            (synchronizer_index_ + 1) % static_cast<int>(synchronizers_.size());
        if (synchronizers_[synchronizer_index_].state == State::kAvailable) {
            current_factory_index_ = synchronizer_index_;
            return synchronizers_[synchronizer_index_].factory->Build();
        }
    }
    current_factory_index_ = -1;
    return nullptr;
}

void SourceManager::BlockCurrentSynchronizer() {
    if (current_factory_index_ >= 0) {
        synchronizers_[current_factory_index_].state = State::kBlocked;
    }
}

void SourceManager::ResetSourceIndex() {
    synchronizer_index_ = -1;
}

bool SourceManager::IsPrimeSynchronizer() const {
    for (std::size_t i = 0; i < synchronizers_.size(); ++i) {
        if (synchronizers_[i].state == State::kAvailable) {
            return synchronizer_index_ == static_cast<int>(i);
        }
    }
    return false;
}

std::size_t SourceManager::AvailableSynchronizerCount() const {
    std::size_t count = 0;
    for (auto const& s : synchronizers_) {
        if (s.state == State::kAvailable) {
            ++count;
        }
    }
    return count;
}

std::size_t SourceManager::SynchronizerCount() const {
    return synchronizers_.size();
}

bool SourceManager::IsCurrentSynchronizerFDv1Fallback() const {
    return current_factory_index_ >= 0 &&
           synchronizers_[current_factory_index_].is_fdv1_fallback;
}

}  // namespace launchdarkly::server_side::data_systems
