#include <launchdarkly/client_side/client.hpp>

#include "client_impl.hpp"

namespace launchdarkly::client_side {

Client::Client(Config config, Context context)
    : client(std::make_unique<ClientImpl>(std::move(config),
                                          std::move(context),
                                          kVersion)) {}
bool Client::Initialized() const {
    return client->Initialized();
}

std::future<bool> Client::StartAsync() {
    return client->StartAsync();
}

using FlagKey = std::string;
[[nodiscard]] std::unordered_map<FlagKey, Value> Client::AllFlags() const {
    return client->AllFlags();
}

void Client::Track(std::string event_name, Value data, double metric_value) {
    client->Track(std::move(event_name), std::move(data), metric_value);
}

void Client::Track(std::string event_name, Value data) {
    client->Track(std::move(event_name), std::move(data));
}

void Client::Track(std::string event_name) {
    client->Track(std::move(event_name));
}

void Client::FlushAsync() {
    client->FlushAsync();
}

std::future<bool> Client::IdentifyAsync(Context context) {
    return client->IdentifyAsync(std::move(context));
}

bool Client::BoolVariation(FlagKey const& key, bool default_value) {
    return client->BoolVariation(key, default_value);
}

EvaluationDetail<bool> Client::BoolVariationDetail(FlagKey const& key,
                                                   bool default_value) {
    return client->BoolVariationDetail(key, default_value);
}

std::string Client::StringVariation(FlagKey const& key,
                                    std::string default_value) {
    return client->StringVariation(key, std::move(default_value));
}

EvaluationDetail<std::string> Client::StringVariationDetail(
    FlagKey const& key,
    std::string default_value) {
    return client->StringVariationDetail(key, std::move(default_value));
}

double Client::DoubleVariation(FlagKey const& key, double default_value) {
    return client->DoubleVariation(key, default_value);
}

EvaluationDetail<double> Client::DoubleVariationDetail(FlagKey const& key,
                                                       double default_value) {
    return client->DoubleVariationDetail(key, default_value);
}

int Client::IntVariation(FlagKey const& key, int default_value) {
    return client->IntVariation(key, default_value);
}

EvaluationDetail<int> Client::IntVariationDetail(FlagKey const& key,
                                                 int default_value) {
    return client->IntVariationDetail(key, default_value);
}

Value Client::JsonVariation(FlagKey const& key, Value default_value) {
    return client->JsonVariation(key, std::move(default_value));
}

EvaluationDetail<Value> Client::JsonVariationDetail(FlagKey const& key,
                                                    Value default_value) {
    return client->JsonVariationDetail(key, std::move(default_value));
}

data_sources::IDataSourceStatusProvider& Client::DataSourceStatus() {
    return client->DataSourceStatus();
}

flag_manager::IFlagNotifier& Client::FlagNotifier() {
    return client->FlagNotifier();
}

}  // namespace launchdarkly::client_side
