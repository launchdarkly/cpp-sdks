#pragma once

#include <launchdarkly/client_side/client.hpp>
#include <memory>
#include "definitions.hpp"

class ClientEntity {
   public:
    explicit ClientEntity(
        std::unique_ptr<launchdarkly::client_side::Client> client);

    tl::expected<nlohmann::json, std::string> Command(CommandParams params);

   private:
    tl::expected<nlohmann::json, std::string> Evaluate(
        EvaluateFlagParams const&);

    tl::expected<nlohmann::json, std::string> EvaluateDetail(
        EvaluateFlagParams const&);

    tl::expected<nlohmann::json, std::string> EvaluateAll(
        EvaluateAllFlagParams const&);

    tl::expected<nlohmann::json, std::string> Identify(
        IdentifyEventParams const&);

    tl::expected<nlohmann::json, std::string> Custom(CustomEventParams const&);

    std::unique_ptr<launchdarkly::client_side::Client> client_;
};

static tl::expected<nlohmann::json, std::string> ContextConvert(
    ContextConvertParams const&);

static tl::expected<nlohmann::json, std::string> ContextBuild(
    ContextBuildParams const&);
