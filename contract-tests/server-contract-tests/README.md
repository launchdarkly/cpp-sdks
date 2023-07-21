## SDK contract tests

Contract tests have a "test service" on one side, and the "test harness" on
the other.

This project implements the test service for the C++ Server-side SDK.

**session (session.hpp)**

This provides a simple REST API for creating/destroying
test entities. Examples:

`GET /` - returns the capabilities of this service.

`DELETE /` - shutdown the service.

`POST /` - create a new test entity, and return its ID.

`DELETE /entity/1` - delete the an entity identified by `1`.

**entity manager (entity_manager.hpp)**

This manages "entities", which are unique instances of the SDK client.

**definitions (definitions.hpp)**

Contains JSON definitions that are used to communicate with the test harness.

**server (server.hpp)**

Glues everything together, mainly providing the TCP acceptor that spawns new sessions.

**session (session.hpp)**

Prepares HTTP responses based on the results of commands sent to entities. 
