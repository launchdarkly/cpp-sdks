#pragma once

#include "stream_entity.hpp"
#include "definitions.hpp"

#include <unordered_map>
#include <mutex>
#include <memory>

class entity_manager {
    std::unordered_map<std::string, std::shared_ptr<stream_entity>> entities_;
    std::size_t counter_;
    std::mutex lock_;
    boost::asio::any_io_executor executor_;
public:
    explicit entity_manager(boost::asio::any_io_executor executor):
            entities_{},
            counter_{0},
            lock_{},
            executor_{std::move(executor)}{
    }

    std::optional<std::string> create(config_params params) {
        std::lock_guard<std::mutex> guard{lock_};
        auto id = std::to_string(counter_++);

        auto builder = launchdarkly::sse::builder{executor_, params.streamUrl};
        if (params.headers) {
            for (auto h: *params.headers) {
                builder.header(h.first, h.second);
            }
        }
        auto client = builder.build();
        if (!client) {
            return std::nullopt;
        }
        entities_.emplace(id, std::make_shared<stream_entity>(executor_, std::move(params)));
        return id;
    }

    bool destroy(std::string const& id) {
        std::lock_guard<std::mutex> guard{lock_};
        auto it = entities_.find(id);
        if (it == entities_.end()) {
            return false;
        }
        entities_.erase(it);
        return true;
    }
};
