#pragma once

#include "definitions.hpp"

#include <string>
#include <launchdarkly/sse/sse.hpp>


class stream_entity {
    std::string callback_url_;
    size_t callback_counter_;
    std::shared_ptr<launchdarkly::sse::client> client_;
public:
    stream_entity(boost::asio::any_io_executor executor, config_params params):
        callback_url_{params.callbackUrl},
        callback_counter_{0},
        client_{}
    {
        auto builder = launchdarkly::sse::builder{executor, params.streamUrl};
        if (params.headers) {
            for (auto h: *params.headers) {
                builder.header(h.first, h.second);
            }
        }
        client_ = builder.build();
    }
};
