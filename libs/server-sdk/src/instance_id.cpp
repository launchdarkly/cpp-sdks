#include "instance_id.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace launchdarkly::server_side {

std::string MakeInstanceId() {
    // boost::uuids::random_generator emits a version 4 (random) UUID, which is
    // what the SCMP spec requires. The generator carries state (an internal RNG
    // seeded from system entropy on construction), so we cache it per thread to
    // avoid redundant entropy draws on repeated calls.
    static thread_local boost::uuids::random_generator generator;
    return boost::uuids::to_string(generator());
}

}  // namespace launchdarkly::server_side
