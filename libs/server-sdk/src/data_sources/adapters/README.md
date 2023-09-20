## Adapters

Contains utilities for converting from in-memory evaluation models to serialized models, and back again.

This is a useful building block for implementing various Bootstrapper/Synchronizers/Destinations.

For example, to build a new Destination that ferries data to Redis, you might pull the `JSONDestination`
off the shelf.

It accepts memory models, serializes them, and forwards to any `ISerializedDataDestination`.

```
IDataDestination -> (serialization step) -> ISerializedDataDestination
```

On the other hand, to build a new Bootstrapper that pulls JSON from a web service, you might pull
the `JSONSource` off the shelf. It accepts JSON models, deserializes them, and forwards to any `IDataDestination`.

```
ISerializedDataDestination -> (deserialization step) -> IDataDestination
```
