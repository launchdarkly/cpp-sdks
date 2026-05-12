#include "instance_id.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace launchdarkly::server_side {

std::string MakeInstanceId() {
    // boost::uuids::random_generator emits a version 4 (random) UUID, which is
    // what the SCMP spec requires. The generator carries state (an internal
    // RNG), so constructing it on every call is fine for our use case where we
    // only call MakeInstanceId once per SDK instance.
    static thread_local boost::uuids::random_generator generator;
    return boost::uuids::to_string(generator());
}

}  // namespace launchdarkly::server_side
