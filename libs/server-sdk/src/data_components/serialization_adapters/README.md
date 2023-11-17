## Adapters

Contains utilities for converting from in-memory evaluation models to serialized models, and back again.

This is a useful building block for implementing various Bootstrapper/Synchronizers/Destinations.

For example, to build a new Destination that ferries data to Redis, you might pull the `JSONDestination`
off the shelf.

It accepts memory models, serializes them, and forwards to any `ISerializedDestination`.

```
IDestination -> (serialization step) -> ISerializedDestination
```

To handle pulling data out of Redis, use the  `JSONSource`. It pulls from any `ISerializedDataSource`, deserializes it,
and passes it back up by implementing `IDataSource`.

```
IDataSource <- (deserialization step) <- ISerializedDataSource
```

On the other hand, to build a new Bootstrapper that pulls JSON from a web service, you might pull
the `JSONSource` off the shelf. It accepts JSON models, deserializes them, and forwards to any `IDestination`.

```
ISerializedDestination -> (deserialization step) -> IDestination
```
