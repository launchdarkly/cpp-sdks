#pragma once

#include <launchdarkly/client_side/api.hpp>
#include <memory>
#include "definitions.hpp"

class ClientEntity {
   public:
    explicit ClientEntity(
        std::unique_ptr<launchdarkly::client_side::Client> client);

    tl::expected<nlohmann::json, std::string> Command(CommandParams params);

   private:
    tl::expected<nlohmann::json, std::string> Evaluate(
        EvaluateFlagParams params);

    tl::expected<nlohmann::json, std::string> EvaluateDetail(
        EvaluateFlagParams);

    tl::expected<nlohmann::json, std::string> EvaluateAll(
        EvaluateAllFlagParams params);

    tl::expected<nlohmann::json, std::string> Identify(
        IdentifyEventParams params);

    tl::expected<nlohmann::json, std::string> Custom(CustomEventParams params);
    std::unique_ptr<launchdarkly::client_side::Client> client_;
};
