# Analytic Event Processor

The Event Processor is responsible for consuming, batching, and delivering events generated
by the server and client-side LaunchDarkly SDKs.

```mermaid
classDiagram
    IEventProcessor <|-- NullEventProcessor
    IEventProcessor <|-- AsioEventProcessor

    AsioEventProcessor *-- LRUCache
    AsioEventProcessor *-- Outbox
    AsioEventProcessor *-- WorkerPool
    AsioEventProcessor *-- Summarizer

    RequestWorker *-- EventBatch
    WorkerPool *-- "5" RequestWorker

    TrackEvent -- TrackEventParams
    InputEvent *-- IdentifyEventParams
    InputEvent *-- FeatureEventParams
    InputEvent *-- TrackEventParams


    OutputEvent *-- IndexEvent
    OutputEvent *-- FeatureEvent
    OutputEvent *-- DebugEvent
    OutputEvent *-- IdentifyEvent
    OutputEvent *-- TrackEvent

    EventBatch --> Outbox: Pulls individual events from..
    EventBatch --> Summarizer: Pulls summary events from..

    IEventProcessor --> InputEvent
    Outbox --> OutputEvent

    Summarizer --> FeatureEventParams


    class IEventProcessor {
        <<interface>>
        +SendAsync(InputEvent event) void
        +FlushAsync() void
        +ShutdownAsync() void
    }

    class NullEventProcessor {

    }

    class AsioEventProcessor {

    }

    class EventBatch {
        +const Count() size_t
        +const Request() network:: HttpRequest
        +const Target() std:: string
    }

    class LRUCache {
        +Notice(std:: string value) bool
        +const Size() size_t
        +Clear() void
    }

    class Outbox {
        +PushDiscardingOverflow(std:: vector~OutputEvent~ events) bool
        +Consume() std:: vector~OutputEvent~
        +const Empty() bool
    }

    class RequestWorker {
        +const Available() bool
        +AsyncDeliver(EventBatch, delivery_callback)
    }

    class WorkerPool {
        +Get(worker_callback) void
    }

    class Summarizer {
        +Update(FeatureEventParams) void
        +Finish()
        +const StartTime() Time
        +const EndTime() Time
    }

%% note: the 'namespace' feature isn't supported on Github yet
%% namespace events { 
    class InputEvent {
        +std:: variant
    }


    class OutputEvent {
        +std:: variant
    }

    class FeatureEventParams {

    }

    class IdentifyEventParams {

    }

    class TrackEventParams {

    }

    class FeatureEvent {

    }

    class DebugEvent {

    }

    class IdentifyEvent {

    }

    class IndexEvent {

    }

    class TrackEvent {

    }

%% }
```

### Notes

SDKs may be configured to disable events, so `NullEventProcessor` is made available. This component accepts
events generated
by the SDK and discards them.

If events are enabled, SDKs use the `AsioEventProcessor` implementation, which is an asynchronous processor
utilizing `boost::asio`.

Most event definitions are shared between the server and client-side SDKs. Unique to the server-side SDK
is `IndexEvent`.
