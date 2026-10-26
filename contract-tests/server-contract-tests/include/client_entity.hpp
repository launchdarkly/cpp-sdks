#pragma once

#include <data_model/data_model.hpp>
#include <launchdarkly/connection.hpp>
#include <launchdarkly/server_side/client.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <memory>
#include <string>
#include <unordered_map>

class ClientEntity {
   public:
    ClientEntity(std::unique_ptr<launchdarkly::server_side::Client> client,
                 boost::asio::any_io_executor executor);

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

    tl::expected<nlohmann::json, std::string> GetBigSegmentStoreStatus();

    tl::expected<nlohmann::json, std::string> RegisterFlagChangeListener(
        RegisterFlagChangeListenerParams const&);

    tl::expected<nlohmann::json, std::string> UnregisterListener(
        UnregisterListenerParams const&);

    boost::asio::any_io_executor executor_;
    std::unique_ptr<launchdarkly::server_side::Client> client_;
    // Declared after client_ so listeners disconnect before the client is
    // destroyed.
    std::unordered_map<std::string, std::unique_ptr<launchdarkly::IConnection>>
        listeners_;
};

static tl::expected<nlohmann::json, std::string> ContextConvert(
    ContextConvertParams const&);

static tl::expected<nlohmann::json, std::string> ContextBuild(
    ContextBuildParams const&);
