## Data Source Implementations

This directory contains data sources for use in the Background Sync system.

These sources implement `IDataSynchronizer`, which provides an interface that allows for passing in
an `IDestination` to which data updates should be sent.

There are two primary sources supported by the server-side SDK.

One is the [streaming](./streaming) source: it receives updates from a web-service (via Server-Sent Events).

The other is [polling](./polling): it periodically hits an endpoint to retrieve a full payload of flag data.

By default, the SDK uses the streaming source. Users can optionally configure the polling source if streaming
isn't feasible or desired. 
