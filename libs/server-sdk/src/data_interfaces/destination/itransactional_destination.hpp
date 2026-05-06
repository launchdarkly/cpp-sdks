#pragma once

#include "../item_change.hpp"
#include "idestination.hpp"

#include <launchdarkly/data_model/change_set.hpp>

namespace launchdarkly::server_side::data_interfaces {

/**
 * ITransactionalDestination extends IDestination with the ability to apply
 * an FDv2 changeset atomically.
 *
 * A changeset is a batch of flag and segment upserts and deletions that must
 * be applied as a unit; readers must never observe a partially applied
 * changeset.
 */
class ITransactionalDestination : public IDestination {
   public:
    /**
     * Applies an FDv2 changeset to the destination atomically.
     */
    virtual void Apply(data_model::ChangeSet<ChangeSetData> change_set) = 0;

    ITransactionalDestination(ITransactionalDestination const&) = delete;
    ITransactionalDestination(ITransactionalDestination&&) = delete;
    ITransactionalDestination& operator=(ITransactionalDestination const&) =
        delete;
    ITransactionalDestination& operator=(ITransactionalDestination&&) = delete;
    ~ITransactionalDestination() override = default;

   protected:
    ITransactionalDestination() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
