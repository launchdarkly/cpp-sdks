#include "context_builder.hpp"

namespace launchdarkly {

AttributesBuilder<ContextBuilder, Context> ContextBuilder::kind(
    std::string kind,
    std::string key) {
    return {*this, std::move(kind), std::move(key)};
}

Context ContextBuilder::build() {
    return {kinds_};
}

void ContextBuilder::internal_add_kind(std::string kind, Attributes attrs) {
    kinds_.insert_or_assign(std::move(kind), std::move(attrs));
}

}  // namespace launchdarkly
