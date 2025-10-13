#pragma once

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <gtest/gtest.h>
#include <sw/redis++/redis++.h>
#include <boost/json.hpp>

#include <string>

class PrefixedClient {
   public:
    PrefixedClient(sw::redis::Redis& client, std::string prefix)
        : client_(client), prefix_(std::move(prefix)) {}

    void Init() const {
        try {
            client_.set(Prefixed("$inited"), "true");
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void PutFlag(launchdarkly::data_model::Flag const& flag) const {
        try {
            client_.hset(Prefixed("features"), flag.key,
                         serialize(boost::json::value_from(flag)));
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void PutDeletedFlag(std::string const& key, std::string const& ts) const {
        try {
            client_.hset(Prefixed("features"), key, ts);
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void PutDeletedSegment(std::string const& key,
                           std::string const& ts) const {
        try {
            client_.hset(Prefixed("segments"), key, ts);
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

    void PutSegment(launchdarkly::data_model::Segment const& segment) const {
        try {
            client_.hset(Prefixed("segments"), segment.key,
                         serialize(boost::json::value_from(segment)));
        } catch (sw::redis::Error const& e) {
            FAIL() << e.what();
        }
    }

   private:
    std::string Prefixed(std::string const& name) const {
        return prefix_ + ":" + name;
    }

    sw::redis::Redis& client_;
    std::string const prefix_;
};
