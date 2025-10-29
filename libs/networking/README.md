# LaunchDarkly C++ Networking Library

This library provides common networking components for the LaunchDarkly C++ SDKs.

Headers in this folder are intended to be used internally to LD SDKs and should not be used by application developers.

Interfaces for these header files, such as method signatures, can change without major versions.

## Components

### CurlMultiManager

Manages asynchronous HTTP operations using the CURL multi interface integrated with Boost.ASIO for non-blocking socket monitoring.

Features:
- Non-blocking HTTP requests using `curl_multi_socket_action()`
- Integration with Boost.ASIO event loop
- Continuous socket monitoring for long-lived connections (e.g., SSE)
- Automatic cleanup and resource management

## Usage

This library is used internally by:
- `launchdarkly-cpp-internal` - For general HTTP requests
- `launchdarkly-cpp-sse` - For server-sent events streaming

## Building

This library requires:
- CMake 3.19+
- C++17 compiler
- libcurl
- Boost (ASIO, System)

Build with `LD_CURL_NETWORKING` flag enabled:
```bash
cmake -DLD_CURL_NETWORKING=ON ..
cmake --build .
```
