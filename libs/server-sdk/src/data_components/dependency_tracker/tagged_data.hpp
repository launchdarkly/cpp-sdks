#pragma once

#include "data_kind.hpp"

namespace launchdarkly::server_side::data_components {
/**
 * Class which can be used to tag a collection with the DataKind that collection
 * is for. This is primarily to decrease the complexity of iterating collections
 * allowing for a kvp style iteration, but with an array storage container.
 * @tparam Storage
 */
template <typename Storage>
class TaggedData {
   public:
    explicit TaggedData(DataKind const kind) : kind_(kind) {}
    [[nodiscard]] DataKind Kind() const { return kind_; }
    [[nodiscard]] Storage const& Data() const { return storage_; }

    [[nodiscard]] Storage& Data() { return storage_; }

   private:
    DataKind kind_;
    Storage storage_;
};

}  // namespace launchdarkly::server_side::data_components
