# LaunchDarkly C++ Server SDK - OpenTelemetry Integration

This package provides OpenTelemetry tracing integration for the LaunchDarkly C++ Server SDK, enabling automatic enrichment of distributed traces with feature flag evaluation data.

## Overview

The OpenTelemetry integration implements the [OpenTelemetry Integration Specification](https://github.com/launchdarkly/open-sdk-specs/blob/main/specs/OTEL-opentelemetry-integration/README.md) using the LaunchDarkly Hooks framework. It automatically adds feature flag evaluation information to your OpenTelemetry traces.

> [!WARNING]
> Currently the C++ SDK doesn't support automatic collection of the environment ID.
> So the `feature_flag.set.id` will only be set if the environment ID is explicitly set.
>
> In a future version automatic collection will be supported.

> [!WARNING]
> This hook can only be used when using the SDK with C++.
> The OpenTelemetry C++ SDK is only designed for use with C++.

## Requirements

- C++17 or later
- LaunchDarkly C++ Server SDK 3.5.0 or later
- OpenTelemetry C++ API 1.23.0 or later

## Installation

### Prerequisites

This package has a **peer dependency** on OpenTelemetry C++.

**For end users:** You must provide OpenTelemetry yourself (via `find_package` or FetchContent in your project).

**For local development and CI:** Use the `LD_BUILD_OTEL_FETCH_DEPS=ON` CMake option to automatically fetch OpenTelemetry with sensible defaults.

**Important:** This package is not available as a pre-built binary. Users must build it themselves after providing OpenTelemetry.

### Method 1: Using CMake with FetchContent

```cmake
cmake_minimum_required(VERSION 3.19)
project(YourApp)

include(FetchContent)

# Step 1: Configure and fetch OpenTelemetry FIRST
# Set options before fetching
set(WITH_OTLP_HTTP ON CACHE BOOL "Build with OTLP HTTP exporter" FORCE)
set(WITH_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)

FetchContent_Declare(
    opentelemetry-cpp
    GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
    GIT_TAG ea1f0d61ce5baa5584b097266bf133d1f31e3607  # v1.23.0
)
FetchContent_MakeAvailable(opentelemetry-cpp)

# Step 2: Fetch LaunchDarkly SDK with OTel support enabled
FetchContent_Declare(
    launchdarkly-cpp
    GIT_REPOSITORY https://github.com/launchdarkly/cpp-sdks.git
    GIT_TAG main
)
set(LD_BUILD_OTEL_SUPPORT ON CACHE BOOL "Enable OTel integration" FORCE)
FetchContent_MakeAvailable(launchdarkly-cpp)

# Step 3: Link your application
add_executable(your_app main.cpp)
target_link_libraries(your_app
    PRIVATE
    launchdarkly::server
    launchdarkly::server_otel
    opentelemetry_trace
    opentelemetry_exporter_otlp_http  # Or your preferred exporter
)
```

### Method 2: Using CMake with find_package

If OpenTelemetry is already installed on your system:

```cmake
find_package(launchdarkly-cpp REQUIRED)
find_package(opentelemetry-cpp REQUIRED)

target_link_libraries(your_app
    PRIVATE
    launchdarkly::server
    launchdarkly::server_otel
    opentelemetry-cpp::trace # For OpenTelemetry SDK functionality
    opentelemetry-cpp::otlp_http_exporter # For exporting traces, or your preferred exporter.
)
```

### Method 3: Local Development and CI Builds

For local development or CI environments where you want automatic dependency management:

```bash
# From the repository root
mkdir build && cd build
cmake .. -DLD_BUILD_OTEL_SUPPORT=ON \
         -DLD_BUILD_OTEL_FETCH_DEPS=ON \
         -DLD_BUILD_EXAMPLES=ON \
         -DBUILD_TESTING=OFF
cmake --build . --target launchdarkly-cpp-server-otel
cmake --build . --target hello-cpp-server-otel  # Build the example
```

The `LD_BUILD_OTEL_FETCH_DEPS=ON` flag automatically:
- Fetches OpenTelemetry v1.23.0 via FetchContent
- Enables OTLP HTTP exporter
- Configures with sensible defaults for development

## Quick Start

Please refer to the [OpenTelemetry C++ installation guide](https://github.com/open-telemetry/opentelemetry-cpp/blob/main/INSTALL.md) for configuration of the OpenTelemetry library.

### Basic Usage (Span Events Only)

```cpp
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>
#include <launchdarkly/server_side/integrations/otel/tracing_hook.hpp>

// Create the hook
auto hook = std::make_shared<launchdarkly::server_side::integrations::otel::TracingHook>();

// Register it with the SDK
auto config = launchdarkly::server_side::ConfigBuilder("your-sdk-key")
                  .Hooks(hook)
                  .Build()
                  .value();

launchdarkly::server_side::Client client(std::move(config));

// Later inside instrumented code.
// Feature flag evaluations will now emit span events automatically
// Span events attach to the currently active span, so if there is no active span, then there is nothing to enrich
// with span events.
// This will use the active span based on your open telemetry context managent. For asynchronous frameworks handling 
// multiple requests per-thread, either custom context management is required, or the parent span can be explicitly
// provided. Refer to `Passing Parent Span Explicitly`.
/
bool result = client.BoolVariation(context, "my-flag", false);
```

### Advanced Usage with Options

```cpp
#include <launchdarkly/server_side/integrations/otel/tracing_hook.hpp>

// Configure the hook
auto options = launchdarkly::server_side::integrations::otel::TracingHookOptionsBuilder()
                   .IncludeValue(true)        // Include flag values in traces
                   .CreateSpans(true)          // Create dedicated spans
                   .EnvironmentId("ld-environment-id")      // Override environment ID
                   .Build();

auto hook = std::make_shared<launchdarkly::server_side::integrations::otel::TracingHook>(options);

auto config = launchdarkly::server_side::ConfigBuilder("your-sdk-key")
                  .Hooks(hook)
                  .Build()
                  .value();
```

### Passing Parent Span Explicitly

```cpp
#include <launchdarkly/server_side/integrations/otel/tracing_hook.hpp>
#include <opentelemetry/trace/provider.h>

// Get your tracer
auto tracer = opentelemetry::trace::Provider::GetTracerProvider()
                  ->GetTracer("my-service");

// Start a span
auto span = tracer->StartSpan("handle_request");

// Create hook context with the span
auto hook_ctx = launchdarkly::server_side::integrations::otel::MakeHookContextWithSpan(span);

// Evaluate with the hook context
bool result = client.BoolVariation(context, "my-flag", false, hook_ctx);

span->End();
```

## Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `IncludeValue` | `bool` | `false` | Include flag evaluation results in span events. **Privacy consideration**: flag values may contain sensitive data. |
| `CreateSpans` | `bool` | `false` | Create a dedicated span for each flag evaluation. **Performance consideration**: spans have higher overhead than events. |
| `EnvironmentId` | `std::optional<std::string>` | `nullopt` | Environment ID to include in telemetry. If not set, uses the environment ID from the SDK after initialization. |

## Span Event Attributes

The hook adds `feature_flag` events to spans with the following attributes:

### Required Attributes
- `feature_flag.key`: The flag key being evaluated
- `feature_flag.provider.name`: Always "LaunchDarkly"
- `feature_flag.context.id`: The canonical key of the evaluation context

### Optional Attributes
- `feature_flag.set.id`: Environment ID (if configured or available)
- `feature_flag.result.value`: Evaluated flag value as JSON string (if `IncludeValue` is enabled)
- `feature_flag.result.variationIndex`: Variation index (if available)
- `feature_flag.result.reason.inExperiment`: Whether the evaluation is part of an experiment (only if true)

## Dedicated Spans (When Enabled)

When `CreateSpans` is enabled, the hook creates spans with:
- **Name**: `LDClient.{method}` (e.g., `LDClient.BoolVariation`)
- **Kind**: Internal
- **Attributes**:
  - `feature_flag.key`: The flag key
  - `feature_flag.context.key`: The context's canonical key

## Examples

An example is included in `examples/hello-cpp-server-otel`.

Learn more
-----------

Read our [documentation](https://docs.launchdarkly.com) for in-depth instructions on configuring and using LaunchDarkly.
You can also head straight to
the [complete reference guide for this SDK][reference-guide].

Testing
-------

We run integration tests for all our SDKs using a centralized test harness. This approach gives us the ability to test
for consistency across SDKs, as well as test networking behavior in a long-running application. These tests cover each
method in the SDK, and verify that event sending, flag evaluation, stream reconnection, and other aspects of the SDK all
behave correctly.

Contributing
------------

We encourage pull requests and other contributions from the community. Read
our [contributing guidelines](../../CONTRIBUTING.md) for instructions on how to contribute to this SDK.

Verifying SDK build provenance with the SLSA framework
------------

LaunchDarkly uses the [SLSA framework](https://slsa.dev/spec/v1.0/about) (Supply-chain Levels for Software Artifacts) to help developers make their supply chain more secure by ensuring the authenticity and build integrity of our published SDK packages. To learn more, see the [provenance guide](../../PROVENANCE.md).

About LaunchDarkly
-----------

* LaunchDarkly is a continuous delivery platform that provides feature flags as a service and allows developers to
  iterate quickly and safely. We allow you to easily flag your features and manage them from the LaunchDarkly dashboard.
  With LaunchDarkly, you can:
  * Roll out a new feature to a subset of your users (like a group of users who opt-in to a beta tester group),
    gathering feedback and bug reports from real-world use cases.
  * Gradually roll out a feature to an increasing percentage of users, and track the effect that the feature has on
    key metrics (for instance, how likely is a user to complete a purchase if they have feature A versus feature B?).
  * Turn off a feature that you realize is causing performance problems in production, without needing to re-deploy,
    or even restart the application with a changed configuration file.
  * Grant access to certain features based on user attributes, like payment plan (eg: users on the ‘gold’ plan get
    access to more features than users in the ‘silver’ plan). Disable parts of your application to facilitate
    maintenance, without taking everything offline.
* LaunchDarkly provides feature flag SDKs for a wide variety of languages and technologies.
  Read [our documentation](https://docs.launchdarkly.com/docs) for a complete list.
* Explore LaunchDarkly
  * [launchdarkly.com](https://www.launchdarkly.com/ "LaunchDarkly Main Website") for more information
  * [docs.launchdarkly.com](https://docs.launchdarkly.com/  "LaunchDarkly Documentation") for our documentation and
    SDK reference guides
  * [apidocs.launchdarkly.com](https://apidocs.launchdarkly.com/  "LaunchDarkly API Documentation") for our API
    documentation
  * [blog.launchdarkly.com](https://blog.launchdarkly.com/  "LaunchDarkly Blog Documentation") for the latest product
    updates
