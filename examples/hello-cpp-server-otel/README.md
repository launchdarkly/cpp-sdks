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

## Building

From the repository root:

```bash
mkdir build && cd build
cmake .. -DLD_BUILD_EXAMPLES=ON -DLD_BUILD_OTEL_SUPPORT=ON
cmake --build . --target hello-cpp-server-otel
```

## Running

### 1. Set Your LaunchDarkly SDK Key

Either edit `main.cpp` and set the `SDK_KEY` constant, or use an environment variable:

```bash
export LD_SDK_KEY=your-sdk-key-here
```

### 2. Create a Feature Flag

In your LaunchDarkly dashboard, create a boolean flag named `show-detailed-weather`.

### 3. Run the Example

```bash
./build/examples/hello-cpp-server-otel/hello-cpp-server-otel
```

You should see:

```
*** SDK successfully initialized!

*** Weather server running on http://0.0.0.0:8080
*** Try: curl http://localhost:8080/weather
*** OpenTelemetry tracing enabled, sending traces to LaunchDarkly
*** LaunchDarkly integration enabled with OpenTelemetry tracing hook
```

### 4. Make Requests

```bash
curl http://localhost:8080/weather
```

### 5. View Traces in LaunchDarkly

Navigate to your LaunchDarkly project's Observability section to view traces. Each HTTP request will have:
1. **Root Span**: "HTTP GET /weather" with HTTP attributes
2. **Feature Flag Event**: Attached to the span with evaluation details:
   - `feature_flag.key`: "show-detailed-weather"
   - `feature_flag.provider.name`: "LaunchDarkly"
   - `feature_flag.context.id`: "user:weather-api-user"
   - `feature_flag.result.value`: The evaluated flag value


### Custom OTLP Endpoint

To send traces to a different OpenTelemetry collector, set the `LD_OTEL_ENDPOINT` environment variable:

```bash
export LD_OTEL_ENDPOINT=http://localhost:4318/v1/traces
./build/examples/hello-cpp-server-otel/hello-cpp-server-otel
```
