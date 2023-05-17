#include <launchdarkly/config/shared/builders/persistence_builder.hpp>

namespace launchdarkly::config::shared::builders {

PersistenceBuilder<ClientSDK>::PersistenceBuilder()
    : type_(PersistenceBuilder<ClientSDK>::None()),
      max_contexts_(Defaults<ClientSDK>::MaxCachedContexts()) {}

PersistenceBuilder<ClientSDK>::PersistenceBuilder(
    launchdarkly::config::shared::builders::PersistenceBuilder<
        ClientSDK>::Custom custom)
    : type_(custom), max_contexts_(Defaults<ClientSDK>::MaxCachedContexts()) {}

PersistenceBuilder<ClientSDK>::PersistenceBuilder(
    launchdarkly::config::shared::builders::PersistenceBuilder<ClientSDK>::None
        none)
    : type_(none), max_contexts_(Defaults<ClientSDK>::MaxCachedContexts()) {}

PersistenceBuilder<ClientSDK>& PersistenceBuilder<ClientSDK>::Persistence(
    std::variant<launchdarkly::config::shared::builders::PersistenceBuilder<
                     ClientSDK>::None,
                 launchdarkly::config::shared::builders::PersistenceBuilder<
                     ClientSDK>::Custom> persistence) {
    type_ = persistence;
    return *this;
}

built::Persistence<ClientSDK> PersistenceBuilder<ClientSDK>::Build() const {
    return std::visit(
        [this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T,
                                         PersistenceBuilder<ClientSDK>::None>) {
                return built::Persistence<ClientSDK>{true, nullptr,
                                                     max_contexts_};
            } else if constexpr (std::is_same_v<T, PersistenceBuilder<
                                                       ClientSDK>::Custom>) {
                if (arg.implementation_) {
                    return built::Persistence<ClientSDK>{
                        true, arg.implementation_, max_contexts_};
                }
                // No implementation set. Return a default config.
                return built::Persistence<ClientSDK>{true, nullptr,
                                                     max_contexts_};
            }
        },
        type_);
}

}  // namespace launchdarkly::config::shared::builders
