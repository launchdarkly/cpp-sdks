# Server-Sent Events Client Tests

This directory contains comprehensive unit tests for the SSE client implementation.

## Building with Code Coverage

To build the tests with code coverage instrumentation:

```bash
# Configure with coverage enabled (note: sanitizers must be disabled)
cmake -B build -DLD_BUILD_UNIT_TESTS=ON -DLD_BUILD_COVERAGE=ON -DLD_TESTING_SANITIZERS=OFF

# Build the tests
cmake --build build --target gtest_launchdarkly-sse-client

# Run the tests
cd build && ctest --output-on-failure

# Generate coverage report
../scripts/generate-coverage.sh

# View the report
xdg-open build/coverage/html/index.html
```

## Test Structure

- `backoff_test.cpp` - Tests for backoff/retry logic
- `curl_client_test.cpp` - Comprehensive tests for CurlClient SSE implementation
- `mock_sse_server.hpp` - Mock SSE server for testing

## Test Scenarios

The tests cover:

1. **Connection Management** - HTTP/HTTPS connections, timeouts, lifecycle
2. **SSE Parsing** - All SSE event formats, line endings, chunked data
3. **TLS/SSL** - Certificate verification, custom CA files
4. **HTTP Semantics** - Methods, headers, redirects, status codes
5. **Error Handling** - Network errors, malformed data, edge cases
6. **Resource Management** - No memory/thread/socket leaks
7. **Concurrency** - Thread safety, proper synchronization

## Coverage Goals

Target: >90% line and branch coverage for CurlClient implementation.
