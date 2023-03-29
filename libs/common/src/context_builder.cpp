#include "context_builder.hpp"

namespace launchdarkly {

AttributesBuilder<ContextBuilder, Context> ContextBuilder::kind(
    std::string kind,
    std::string key) {
    return {*this, kind, key};
}

Context ContextBuilder::build() {
    return Context(kinds_);
}

void ContextBuilder::internal_add_kind(std::string kind, Attributes attrs) {
    kinds_.insert_or_assign(kind, attrs);
}

}  // namespace launchdarkly
