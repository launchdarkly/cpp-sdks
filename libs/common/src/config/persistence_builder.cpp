#include <launchdarkly/config/shared/builders/persistence_builder.hpp>

namespace launchdarkly::config::shared::builders {

PersistenceBuilder<ClientSDK>::PersistenceBuilder()
    : type_(PersistenceBuilder<ClientSDK>::NoneBuilder()),
      max_contexts_(Defaults<ClientSDK>::MaxCachedContexts()) {}

PersistenceBuilder<ClientSDK>& PersistenceBuilder<ClientSDK>::Type(
    PersistenceBuilder<ClientSDK>::PersistenceType persistence) {
    type_ = persistence;
    return *this;
}

PersistenceBuilder<ClientSDK>& PersistenceBuilder<ClientSDK>::Custom(
    std::shared_ptr<IPersistence> implementation) {
    type_ = PersistenceBuilder<ClientSDK>::CustomBuilder().Implementation(
        implementation);
    return *this;
}

PersistenceBuilder<ClientSDK>& PersistenceBuilder<ClientSDK>::None() {
    type_ = PersistenceBuilder<ClientSDK>::NoneBuilder();
    return *this;
}

built::Persistence<ClientSDK> PersistenceBuilder<ClientSDK>::Build() const {
    return std::visit(
        [this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<
                              T, PersistenceBuilder<ClientSDK>::NoneBuilder>) {
                return built::Persistence<ClientSDK>{true, nullptr,
                                                     max_contexts_};
            } else if constexpr (std::is_same_v<
                                     T, PersistenceBuilder<
                                            ClientSDK>::CustomBuilder>) {
                if (arg.implementation_) {
                    return built::Persistence<ClientSDK>{
                        false, arg.implementation_, max_contexts_};
                }
                // No implementation set. Return a default config.
                return built::Persistence<ClientSDK>{true, nullptr,
                                                     max_contexts_};
            }
        },
        type_);
}

launchdarkly::config::shared::builders::PersistenceBuilder<
    ClientSDK>::CustomBuilder&
PersistenceBuilder<ClientSDK>::CustomBuilder::Implementation(
    std::shared_ptr<IPersistence> implementation) {
    implementation_ = implementation;
    return *this;
}
}  // namespace launchdarkly::config::shared::builders
