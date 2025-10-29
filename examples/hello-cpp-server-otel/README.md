# LaunchDarkly C++ Server SDK - OpenTelemetry Integration Example

This example demonstrates how to integrate the LaunchDarkly C++ Server SDK with OpenTelemetry tracing to automatically enrich your distributed traces with feature flag evaluation data.

## What This Example Shows

- Setting up OpenTelemetry with OTLP HTTP exporter
- Configuring the LaunchDarkly OpenTelemetry tracing hook
- Creating HTTP spans with Boost.Beast
- Automatic feature flag span events in traces
- Passing explicit parent span context to evaluations

## Prerequisites

- C++17 or later
- CMake 3.19 or later
- Boost 1.81 or later
- LaunchDarkly SDK key
- OpenTelemetry collector (or compatible backend) running on `localhost:4318`

## Building

From the repository root:

```bash
mkdir build && cd build
cmake .. -DLD_BUILD_EXAMPLES=ON -DLD_BUILD_OTEL_SUPPORT=ON
cmake --build . --target hello-cpp-server-otel
```

## Running

### 1. Start an OpenTelemetry Collector

The easiest way is using Docker:

```bash
docker run -p 4318:4318 otel/opentelemetry-collector:latest
```

Or use Jaeger (which has a built-in OTLP receiver):

```bash
docker run -d -p 16686:16686 -p 4318:4318 jaegertracing/all-in-one:latest
```

### 2. Set Your LaunchDarkly SDK Key

Either edit `main.cpp` and set the `SDK_KEY` constant, or use an environment variable:

```bash
export LD_SDK_KEY=your-sdk-key-here
```

### 3. Create a Feature Flag

In your LaunchDarkly dashboard, create a boolean flag named `show-detailed-weather`.

### 4. Run the Example

```bash
./build/examples/hello-cpp-server-otel/hello-cpp-server-otel
```

You should see:

```
*** SDK successfully initialized!

*** Weather server running on http://0.0.0.0:8080
*** Try: curl http://localhost:8080/weather
*** OpenTelemetry tracing enabled (OTLP HTTP to localhost:4318)
*** LaunchDarkly integration enabled with OpenTelemetry tracing hook
```

### 5. Make Requests

```bash
curl http://localhost:8080/weather
```

### 6. View Traces

If using Jaeger, open http://localhost:16686 in your browser. You should see traces with:

- HTTP request spans
- Feature flag evaluation events with attributes:
  - `feature_flag.key`: "show-detailed-weather"
  - `feature_flag.provider.name`: "LaunchDarkly"
  - `feature_flag.context.id`: Context canonical key
  - `feature_flag.result.value`: The flag value (since `IncludeValue` is enabled)

## How It Works

### OpenTelemetry Setup

```cpp
void InitTracer() {
    opentelemetry::exporter::otlp::OtlpHttpExporterOptions opts;
    opts.url = "http://localhost:4318/v1/traces";

    auto exporter = opentelemetry::exporter::otlp::OtlpHttpExporterFactory::Create(opts);
    auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
    std::shared_ptr<trace_api::TracerProvider> provider =
            trace_sdk::TracerProviderFactory::Create(std::move(processor));
    trace_api::Provider::SetTracerProvider(provider);
}
```

### LaunchDarkly Hook Setup

```cpp
auto hook_options = launchdarkly::server_side::integrations::otel::TracingHookOptionsBuilder()
                        .IncludeValue(true)   // Include flag values in traces
                        .CreateSpans(false)   // Only create span events, not full spans
                        .Build();
auto tracing_hook = std::make_shared<launchdarkly::server_side::integrations::otel::TracingHook>(hook_options);

auto config = launchdarkly::server_side::ConfigBuilder(sdk_key)
        .Hooks(tracing_hook)
        .Build();
```

### Passing Parent Span Context

When using async frameworks like Boost.Beast, you need to manually pass the parent span:

```cpp
auto span = tracer->StartSpan("HTTP GET /weather");
auto scope = trace_api::Scope(span);

// Create hook context with the span
auto hook_ctx = launchdarkly::server_side::integrations::otel::MakeHookContextWithSpan(span);

// Pass it to the evaluation
auto flag_value = ld_client->BoolVariation(context, "my-flag", false, hook_ctx);
```

This ensures feature flag events appear as children of the correct span.

## What You'll See

### In Your Application Logs

```
*** SDK successfully initialized!

*** Weather server running on http://0.0.0.0:8080
```

### In Your Traces

Each HTTP request will have:
1. **Root Span**: "HTTP GET /weather" with HTTP attributes
2. **Span Event**: "feature_flag" with LaunchDarkly evaluation details

Example trace structure:
```
HTTP GET /weather (span)
  └─ feature_flag (event)
     ├─ feature_flag.key: "show-detailed-weather"
     ├─ feature_flag.provider.name: "LaunchDarkly"
     ├─ feature_flag.context.id: "user:weather-api-user"
     └─ feature_flag.result.value: "true"
```

## Customization

### Include/Exclude Flag Values

For privacy, you can exclude flag values from traces:

```cpp
.IncludeValue(false)  // Don't include flag values
```

### Create Dedicated Spans

For detailed performance tracking:

```cpp
.CreateSpans(true)  // Create a span for each evaluation
```

This creates spans like `LDClient.BoolVariation` in addition to the feature_flag event.

### Set Environment ID

To include environment information in traces:

```cpp
.EnvironmentId("production")
```

## Troubleshooting

### No traces appear

1. Verify OpenTelemetry collector is running: `curl http://localhost:4318/v1/traces`
2. Check the SDK initialized successfully
3. Ensure you're making requests to the server

### Feature flag events missing

1. Verify the hook is registered before creating the client
2. Check that you're passing the HookContext when evaluating flags in async contexts
3. Ensure there's an active span when the evaluation happens

## Architecture

This example uses:
- **Boost.Beast**: Async HTTP server
- **OpenTelemetry C++**: Distributed tracing
- **LaunchDarkly C++ Server SDK**: Feature flags
- **LaunchDarkly OTel Integration**: Automatic trace enrichment

The integration is non-invasive - the hook automatically captures all flag evaluations without changing your evaluation code.
