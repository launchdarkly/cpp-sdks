#include <launchdarkly/server_side/client.hpp>

#include "client_impl.hpp"

namespace launchdarkly::server_side {

void operator|=(AllFlagsState::Options& lhs, AllFlagsState::Options rhs) {
    lhs = lhs | rhs;
}

AllFlagsState::Options operator|(AllFlagsState::Options lhs,
                                 AllFlagsState::Options rhs) {
    return static_cast<AllFlagsState::Options>(
        static_cast<std::underlying_type_t<AllFlagsState::Options>>(lhs) |
        static_cast<std::underlying_type_t<AllFlagsState::Options>>(rhs));
}

AllFlagsState::Options operator&(AllFlagsState::Options lhs,
                                 AllFlagsState::Options rhs) {
    return static_cast<AllFlagsState::Options>(
        static_cast<std::underlying_type_t<AllFlagsState::Options>>(lhs) &
        static_cast<std::underlying_type_t<AllFlagsState::Options>>(rhs));
}

Client::Client(Config config)
    : client(std::make_unique<ClientImpl>(std::move(config), kVersion)) {}

bool Client::Initialized() const {
    return client->Initialized();
}

std::future<bool> Client::StartAsync() {
    return client->StartAsync();
}

using FlagKey = std::string;
[[nodiscard]] AllFlagsState Client::AllFlagsState(
    Context const& context,
    enum AllFlagsState::Options options) {
    return client->AllFlagsState(context, options);
}

void Client::Track(Context const& ctx,
                   std::string event_name,
                   Value data,
                   double metric_value) {
    client->Track(ctx, std::move(event_name), std::move(data), metric_value);
}

void Client::Track(Context const& ctx, std::string event_name, Value data) {
    client->Track(ctx, std::move(event_name), std::move(data));
}

void Client::Track(Context const& ctx, std::string event_name) {
    client->Track(ctx, std::move(event_name));
}

void Client::FlushAsync() {
    client->FlushAsync();
}

void Client::Identify(Context context) {
    return client->Identify(std::move(context));
}

bool Client::BoolVariation(Context const& ctx,
                           FlagKey const& key,
                           bool default_value) {
    return client->BoolVariation(ctx, key, default_value);
}

EvaluationDetail<bool> Client::BoolVariationDetail(Context const& ctx,
                                                   FlagKey const& key,
                                                   bool default_value) {
    return client->BoolVariationDetail(ctx, key, default_value);
}

std::string Client::StringVariation(Context const& ctx,
                                    FlagKey const& key,
                                    std::string default_value) {
    return client->StringVariation(ctx, key, std::move(default_value));
}

EvaluationDetail<std::string> Client::StringVariationDetail(
    Context const& ctx,
    FlagKey const& key,
    std::string default_value) {
    return client->StringVariationDetail(ctx, key, std::move(default_value));
}

double Client::DoubleVariation(Context const& ctx,
                               FlagKey const& key,
                               double default_value) {
    return client->DoubleVariation(ctx, key, default_value);
}

EvaluationDetail<double> Client::DoubleVariationDetail(Context const& ctx,
                                                       FlagKey const& key,
                                                       double default_value) {
    return client->DoubleVariationDetail(ctx, key, default_value);
}

int Client::IntVariation(Context const& ctx,
                         FlagKey const& key,
                         int default_value) {
    return client->IntVariation(ctx, key, default_value);
}

EvaluationDetail<int> Client::IntVariationDetail(Context const& ctx,
                                                 FlagKey const& key,
                                                 int default_value) {
    return client->IntVariationDetail(ctx, key, default_value);
}

Value Client::JsonVariation(Context const& ctx,
                            FlagKey const& key,
                            Value default_value) {
    return client->JsonVariation(ctx, key, std::move(default_value));
}

EvaluationDetail<Value> Client::JsonVariationDetail(Context const& ctx,
                                                    FlagKey const& key,
                                                    Value default_value) {
    return client->JsonVariationDetail(ctx, key, std::move(default_value));
}

IDataSourceStatusProvider& Client::DataSourceStatus() {
    return client->DataSourceStatus();
}

char const* Client::Version() {
    return kVersion;
}

}  // namespace launchdarkly::server_side
