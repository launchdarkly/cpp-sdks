## SSE contract tests


Contract tests have a "test service" on one side, and the "test harness" on 
the other. 

This project implements the test service for the C++ EventSource client.




**session (session.hpp)**

This provides a simple REST API for creating/destroying 
test entities. Examples:

`GET /` - returns the capabilities of this service.

`DELETE /` - shutdown the service.

`POST /` - create a new test entity, and return its ID.

`DELETE /entity/1` - delete the an entity identified by `1`. 

**entity manager (entity_manager.hpp)** 

This manages "entities" - the combination of an SSE client, and an outbox that posts events _received_ from the stream
_back to_ the test harness. 

The point is to allow the test harness to assert that events were parsed and dispatched as expected.

**event outbox (event_outbox.hpp)**

The 2nd half of an "entity". It receives events from the SSE client, pushes them into a queue,
and then periodically flushes the queue out to the test harness.


**definitions (definitions.hpp)**

Contains JSON definitions that are used to communicate with the test harness.

**server (server.hpp)**

Glues everything together, mainly providing the TCP acceptor that spawns new sessions. 
