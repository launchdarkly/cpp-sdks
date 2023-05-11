#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include <launchdarkly/context.hpp>
#include <launchdarkly/value.hpp>

#include <launchdarkly/client_side/data_sources/data_source_status.hpp>
#include <launchdarkly/client_side/flag_manager/flag_notifier.hpp>
#include <launchdarkly/config/client.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>

namespace launchdarkly::client_side {

class IClient {
   public:
    [[nodiscard]] virtual bool Initialized() const = 0;

    using FlagKey = std::string;
    [[nodiscard]] virtual std::unordered_map<FlagKey, Value> AllFlags()
        const = 0;

    virtual void Track(std::string event_name,
                       Value data,
                       double metric_value) = 0;

    virtual void Track(std::string event_name, Value data) = 0;

    virtual void Track(std::string event_name) = 0;

    virtual void AsyncFlush() = 0;

    virtual void AsyncIdentify(Context context) = 0;

    virtual bool BoolVariation(FlagKey const& key, bool default_value) = 0;

    virtual EvaluationDetail<bool> BoolVariationDetail(FlagKey const& key,
                                                       bool default_value) = 0;

    virtual std::string StringVariation(FlagKey const& key,
                                        std::string default_value) = 0;

    virtual EvaluationDetail<std::string> StringVariationDetail(
        FlagKey const& key,
        std::string default_value) = 0;

    virtual double DoubleVariation(FlagKey const& key,
                                   double default_value) = 0;

    virtual EvaluationDetail<double> DoubleVariationDetail(
        FlagKey const& key,
        double default_value) = 0;

    virtual int IntVariation(FlagKey const& key, int default_value) = 0;

    virtual EvaluationDetail<int> IntVariationDetail(FlagKey const& key,
                                                     int default_value) = 0;

    virtual Value JsonVariation(FlagKey const& key, Value default_value) = 0;

    virtual EvaluationDetail<Value> JsonVariationDetail(
        FlagKey const& key,
        Value default_value) = 0;

    virtual data_sources::IDataSourceStatusProvider& DataSourceStatus() = 0;

    virtual flag_manager::detail::IFlagNotifier& FlagNotifier() = 0;

    virtual void WaitForReadySync(std::chrono::seconds timeout) = 0;

    virtual ~IClient() = default;
    IClient(IClient const& item) = delete;
    IClient(IClient&& item) = delete;
    IClient& operator=(IClient const&) = delete;
    IClient& operator=(IClient&&) = delete;

   protected:
    IClient() = default;
};

class Client : public IClient {
   public:
    inline static char const* const kVersion =
        "0.0.0";  // {x-release-please-version}

    Client(Config config, Context context);

    Client(Client&&) = delete;
    Client(Client const&) = delete;
    Client& operator=(Client) = delete;
    Client& operator=(Client&& other) = delete;

    [[nodiscard]] bool Initialized() const override;

    using FlagKey = std::string;
    [[nodiscard]] std::unordered_map<FlagKey, Value> AllFlags() const override;

    void Track(std::string event_name,
               Value data,
               double metric_value) override;

    void Track(std::string event_name, Value data) override;

    void Track(std::string event_name) override;

    void AsyncFlush() override;

    void AsyncIdentify(Context context) override;

    bool BoolVariation(FlagKey const& key, bool default_value) override;

    EvaluationDetail<bool> BoolVariationDetail(FlagKey const& key,
                                               bool default_value) override;

    std::string StringVariation(FlagKey const& key,
                                std::string default_value) override;

    EvaluationDetail<std::string> StringVariationDetail(
        FlagKey const& key,
        std::string default_value) override;

    double DoubleVariation(FlagKey const& key, double default_value) override;

    EvaluationDetail<double> DoubleVariationDetail(
        FlagKey const& key,
        double default_value) override;

    int IntVariation(FlagKey const& key, int default_value) override;

    EvaluationDetail<int> IntVariationDetail(FlagKey const& key,
                                             int default_value) override;

    Value JsonVariation(FlagKey const& key, Value default_value) override;

    EvaluationDetail<Value> JsonVariationDetail(FlagKey const& key,
                                                Value default_value) override;

    data_sources::IDataSourceStatusProvider& DataSourceStatus() override;

    flag_manager::detail::IFlagNotifier& FlagNotifier() override;

    void WaitForReadySync(std::chrono::seconds timeout) override;

   private:
    std::unique_ptr<IClient> client;
};

}  // namespace launchdarkly::client_side