## Data Source Implementations

This directory contains implementations of `IDataSource`.

There are two primary schemes supported by the server-side SDK.

One is to receive push-updates from a web-service (via Server-Sent Events). The other is to pull
data from a database as needed.

### Push Data Source

The Push data source is able to operate in "streaming" or "polling" mode. In either mode, flag
updates are delivered in the background from the perspective of the application.

### Pull Data Source

The Pull data source fetches flag data on-demand from a database. Because this is likely slow,
it operates a read-through cache wherein each stored item has a TTL indicating when it should be
re-fetched.

When an item expires, it does not evict it from memory - often stale data is better
than no data. Eviction is disabled at this time, but may be added in a later version.
