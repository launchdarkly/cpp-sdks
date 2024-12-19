#pragma once

#include <launchdarkly/events/event_processor_interface.hpp>

#include <string>
#include <vector>

namespace launchdarkly::server_side {

/**
 * This class is meant only to record direct prerequisites of a flag. That is,
 * although it will be passed events for all prerequisites seen during an
 * evaluation via SendAsync, it will only store those that are a direct
 * prerequisite of the parent flag passed in the constructor.
 *
 * As a future improvement, it would be possible to unify the EventScope
 * mechanism currently used by the Evaluator to send events with a class
 * similar to this one, or to refactor the Evaluator to include prerequisite
 * information in the returned EvaluationDetail (or a new Result class, which
 * would be a composite of the EvaluationDetail and a vector of prerequisites.)
 */
class PrereqEventRecorder final : public events::IEventProcessor {
   public:
    explicit PrereqEventRecorder(std::string flag_key);

    void SendAsync(events::InputEvent event) override;

    /* No-op */
    void FlushAsync() override;

    /* No-op */
    void ShutdownAsync() override;

    std::vector<std::string> const& Prerequisites() const;

    std::vector<std::string>&& TakePrerequisites() &&;

   private:
    std::string const flag_key_;
    std::vector<std::string> prereqs_;
};

}  // namespace launchdarkly::server_side
