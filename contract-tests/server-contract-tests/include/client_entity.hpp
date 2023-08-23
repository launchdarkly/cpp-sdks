#pragma once

#include <data_model/data_model.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <memory>

class ClientEntity {
   public:
    explicit ClientEntity(
        std::unique_ptr<launchdarkly::server_side::Client> client);

    tl::expected<nlohmann::json, std::string> Command(CommandParams params);

   private:
    tl::expected<nlohmann::json, std::string> Evaluate(
        EvaluateFlagParams const&);

    tl::expected<nlohmann::json, std::string> EvaluateDetail(
        EvaluateFlagParams const&,
        launchdarkly::Context const&);

    tl::expected<nlohmann::json, std::string> EvaluateAll(
        EvaluateAllFlagParams const&);

    tl::expected<nlohmann::json, std::string> Identify(
        IdentifyEventParams const&);

    tl::expected<nlohmann::json, std::string> Custom(CustomEventParams const&);

    std::unique_ptr<launchdarkly::server_side::Client> client_;
};

static tl::expected<nlohmann::json, std::string> ContextConvert(
    ContextConvertParams const&);

static tl::expected<nlohmann::json, std::string> ContextBuild(
    ContextBuildParams const&);
