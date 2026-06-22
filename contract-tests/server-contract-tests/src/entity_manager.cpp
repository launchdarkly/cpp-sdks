#include "entity_manager.hpp"
#include "contract_test_big_segment_store.hpp"
#include "contract_test_hook.hpp"

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/serialization/json_context.hpp>
#include <launchdarkly/server_side/config/builders/big_segments_builder.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include <boost/json.hpp>

using launchdarkly::LogLevel;
using namespace launchdarkly::server_side;

namespace {

config::builders::DataSystemBuilder::FDv2 BuildFDv2(
    ConfigDataSystemParams const& cfg,
    config::builders::EndpointsBuilder* endpoints) {
    auto fdv2 = config::builders::DataSystemBuilder::FDv2::Custom();

    if (cfg.synchronizers) {
        for (auto const& sync : *cfg.synchronizers) {
            if (sync.streaming) {
                auto s = decltype(fdv2)::Streaming();
                if (sync.streaming->baseUri) {
                    s.BaseUrl(*sync.streaming->baseUri);
                }
                if (sync.streaming->initialRetryDelayMs) {
                    s.InitialReconnectDelay(std::chrono::milliseconds(
                        *sync.streaming->initialRetryDelayMs));
                }
                fdv2.Synchronizer(std::move(s));
            } else if (sync.polling) {
                auto p = decltype(fdv2)::Polling();
                if (sync.polling->baseUri) {
                    p.BaseUrl(*sync.polling->baseUri);
                }
                if (sync.polling->pollIntervalMs) {
                    p.PollInterval(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::milliseconds(
                                *sync.polling->pollIntervalMs)));
                }
                fdv2.Synchronizer(std::move(p));
            }
        }
    }

    if (cfg.initializers) {
        for (auto const& init : *cfg.initializers) {
            if (init.polling) {
                auto p = decltype(fdv2)::Polling();
                if (init.polling->baseUri) {
                    p.BaseUrl(*init.polling->baseUri);
                }
                if (init.polling->pollIntervalMs) {
                    p.PollInterval(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::milliseconds(
                                *init.polling->pollIntervalMs)));
                }
                fdv2.Initializer(std::move(p));
            }
        }
    }

    using FDv2Builder = config::builders::DataSystemBuilder::FDv2;
    if (cfg.fdv1Fallback) {
        if (cfg.fdv1Fallback->baseUri) {
            endpoints->PollingBaseUrl(*cfg.fdv1Fallback->baseUri);
        } else if (cfg.synchronizers && !cfg.synchronizers->empty()) {
            // No explicit baseUri: derive from the synchronizers list, matching
            // the no-fdv1Fallback branch below.
            ConfigDataSynchronizerParams const* selected = nullptr;
            for (auto const& sync : *cfg.synchronizers) {
                if (sync.polling) {
                    selected = &sync;
                    break;
                }
            }
            if (!selected) {
                selected = &cfg.synchronizers->front();
            }
            if (selected->polling && selected->polling->baseUri) {
                endpoints->PollingBaseUrl(*selected->polling->baseUri);
            } else if (selected->streaming && selected->streaming->baseUri) {
                endpoints->PollingBaseUrl(*selected->streaming->baseUri);
            }
        }
        FDv2Builder::FDv1Polling p;
        if (cfg.fdv1Fallback->pollIntervalMs) {
            p.PollInterval(std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::milliseconds(*cfg.fdv1Fallback->pollIntervalMs)));
        }
        fdv2.FDv1Fallback(std::move(p));
    } else if (cfg.synchronizers && !cfg.synchronizers->empty()) {
        // Derive an FDv1 fallback from the synchronizers list: prefer the
        // first polling sync, otherwise reuse the first synchronizer's
        // baseUri. The fallback is always polling. The fallback reads its
        // URL from the global ServiceEndpoints, so set the polling endpoint
        // to the selected baseUri.
        ConfigDataSynchronizerParams const* selected = nullptr;
        for (auto const& sync : *cfg.synchronizers) {
            if (sync.polling) {
                selected = &sync;
                break;
            }
        }
        if (!selected) {
            selected = &cfg.synchronizers->front();
        }
        FDv2Builder::FDv1Polling p;
        if (selected->polling) {
            if (selected->polling->baseUri) {
                endpoints->PollingBaseUrl(*selected->polling->baseUri);
            }
            if (selected->polling->pollIntervalMs) {
                p.PollInterval(std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::milliseconds(
                        *selected->polling->pollIntervalMs)));
            }
        } else if (selected->streaming && selected->streaming->baseUri) {
            endpoints->PollingBaseUrl(*selected->streaming->baseUri);
        }
        fdv2.FDv1Fallback(std::move(p));
    }

    return fdv2;
}

config::builders::DataSystemBuilder::BackgroundSync BuildBackgroundSync(
    ConfigParams const& in,
    config::builders::EndpointsBuilder* endpoints) {
    auto datasystem = config::builders::DataSystemBuilder::BackgroundSync();

    if (in.streaming) {
        if (in.streaming->baseUri) {
            endpoints->StreamingBaseUrl(*in.streaming->baseUri);
        }
        auto streaming = decltype(datasystem)::Streaming();
        if (in.streaming->initialRetryDelayMs) {
            streaming.InitialReconnectDelay(
                std::chrono::milliseconds(*in.streaming->initialRetryDelayMs));
        }
        if (in.streaming->filter) {
            streaming.Filter(*in.streaming->filter);
        }
        datasystem.Synchronizer(std::move(streaming));
    }

    if (in.polling) {
        if (in.polling->baseUri) {
            endpoints->PollingBaseUrl(*in.polling->baseUri);
        }
        if (!in.streaming) {
            auto method = decltype(datasystem)::Polling();
            if (in.polling->pollIntervalMs) {
                method.PollInterval(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::milliseconds(
                            *in.polling->pollIntervalMs)));
            }
            if (in.polling->filter) {
                method.Filter(*in.polling->filter);
            }
            datasystem.Synchronizer(std::move(method));
        }
    }

    return datasystem;
}

}  // namespace

EntityManager::EntityManager(boost::asio::any_io_executor executor,
                             launchdarkly::Logger& logger)
    : counter_{0}, executor_{std::move(executor)}, logger_{logger} {}

std::optional<std::string> EntityManager::create(ConfigParams const& in) {
    std::string id = std::to_string(counter_++);

    auto config_builder = ConfigBuilder(in.credential);

    // The contract test service sets endpoints in a way that is disallowed
    // for users. Specifically, it may set just 1 of the 3 endpoints, whereas
    // we require all 3 to be set.
    //
    // To avoid that error being detected, we must configure the Endpoints
    // builder with the 3 default URLs, which we can fetch by just calling Build
    // on a new builder. That way when the contract tests set just 1 URL,
    // the others have already been "set" so no error occurs.
    auto const default_endpoints =
        *config::builders::EndpointsBuilder().Build();

    auto& endpoints =
        config_builder.ServiceEndpoints()
            .EventsBaseUrl(default_endpoints.EventsBaseUrl())
            .PollingBaseUrl(default_endpoints.PollingBaseUrl())
            .StreamingBaseUrl(default_endpoints.StreamingBaseUrl());

    if (in.serviceEndpoints) {
        if (in.serviceEndpoints->streaming) {
            endpoints.StreamingBaseUrl(*in.serviceEndpoints->streaming);
        }
        if (in.serviceEndpoints->polling) {
            endpoints.PollingBaseUrl(*in.serviceEndpoints->polling);
        }
        if (in.serviceEndpoints->events) {
            endpoints.EventsBaseUrl(*in.serviceEndpoints->events);
        }
    }

    if (in.dataSystem) {
        config_builder.DataSystem().Method(
            BuildFDv2(*in.dataSystem, &endpoints));
    } else {
        config_builder.DataSystem().Method(BuildBackgroundSync(in, &endpoints));
    }

    auto& event_config = config_builder.Events();

    if (in.events) {
        ConfigEventParams const& events = *in.events;

        if (events.baseUri) {
            endpoints.EventsBaseUrl(*events.baseUri);
        }

        if (events.allAttributesPrivate) {
            event_config.AllAttributesPrivate(*events.allAttributesPrivate);
        }

        if (!events.globalPrivateAttributes.empty()) {
            launchdarkly::AttributeReference::SetType attrs(
                events.globalPrivateAttributes.begin(),
                events.globalPrivateAttributes.end());
            event_config.PrivateAttributes(std::move(attrs));
        }

        if (events.capacity) {
            event_config.Capacity(*events.capacity);
        }

        if (events.flushIntervalMs) {
            event_config.FlushInterval(
                std::chrono::milliseconds(*events.flushIntervalMs));
        }

    } else {
        event_config.Disable();
    }

    if (in.tags) {
        if (in.tags->applicationId) {
            config_builder.AppInfo().Identifier(*in.tags->applicationId);
        }
        if (in.tags->applicationVersion) {
            config_builder.AppInfo().Version(*in.tags->applicationVersion);
        }
    }

    if (in.tls) {
        auto builder = config::builders::TlsBuilder();
        if (in.tls->skipVerifyPeer) {
            builder.SkipVerifyPeer(*in.tls->skipVerifyPeer);
        }
        if (in.tls->customCAFile) {
            builder.CustomCAFile(*in.tls->customCAFile);
        }
        config_builder.HttpProperties().Tls(std::move(builder));
    }

    if (in.hooks) {
        for (auto const& hook_config : in.hooks->hooks) {
            auto hook =
                std::make_shared<ContractTestHook>(executor_, hook_config);
            config_builder.Hooks(hook);
        }
    }

    if (in.wrapper) {
        if (!in.wrapper->name.empty()) {
            config_builder.HttpProperties().WrapperName(in.wrapper->name);
        }
        if (!in.wrapper->version.empty()) {
            config_builder.HttpProperties().WrapperVersion(in.wrapper->version);
        }
    }

    if (in.bigSegments) {
        auto store = std::make_shared<ContractTestBigSegmentStore>(
            in.bigSegments->callbackUri);
        auto big_segments = config::builders::BigSegmentsBuilder(store);
        if (in.bigSegments->userCacheSize) {
            big_segments.ContextCacheSize(*in.bigSegments->userCacheSize);
        }
        if (in.bigSegments->userCacheTimeMs) {
            big_segments.ContextCacheTime(
                std::chrono::milliseconds(*in.bigSegments->userCacheTimeMs));
        }
        if (in.bigSegments->statusPollIntervalMs) {
            big_segments.StatusPollInterval(std::chrono::milliseconds(
                *in.bigSegments->statusPollIntervalMs));
        }
        if (in.bigSegments->staleAfterMs) {
            big_segments.StaleAfter(
                std::chrono::milliseconds(*in.bigSegments->staleAfterMs));
        }
        config_builder.BigSegments(std::move(big_segments));
    }

    auto config = config_builder.Build();
    if (!config) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "entity_manager: couldn't build config: " << config.error();
        return std::nullopt;
    }

    auto client = std::make_unique<Client>(std::move(*config));

    std::chrono::milliseconds waitForClient = std::chrono::seconds(5);
    if (in.startWaitTimeMs) {
        waitForClient = std::chrono::milliseconds(*in.startWaitTimeMs);
    }

    auto init = client->StartAsync();
    init.wait_for(waitForClient);

    entities_.try_emplace(id, std::move(client));

    return id;
}

bool EntityManager::destroy(std::string const& id) {
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return false;
    }

    entities_.erase(it);
    return true;
}

tl::expected<nlohmann::json, std::string> EntityManager::command(
    std::string const& id,
    CommandParams const& params) {
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return tl::make_unexpected("entity not found");
    }
    return it->second.Command(params);
}
