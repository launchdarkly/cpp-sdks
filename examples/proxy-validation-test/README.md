# Proxy Validation Test

This test validates that the LaunchDarkly C++ Client SDK properly uses CURL for all network requests and correctly routes traffic through a proxy when configured.

## Prerequisites

- Docker and Docker Compose
- A LaunchDarkly mobile key (for testing with real endpoints)

## What This Test Validates

1. **SSE streaming** connections go through the configured proxy
2. **Event posting** goes through the configured proxy
3. **Without proxy access**, the SDK cannot reach LaunchDarkly servers
4. **With proxy access**, all SDK operations work correctly

Polling can be validated by changing the hello-cpp-client application to use polling.

## Test Architecturegit

The test uses Docker Compose with network isolation to validate proxy functionality:

- **proxy**: SOCKS5 proxy server (using `serjs/go-socks5-proxy`)
  - Connected to both the internal network and the internet
  - Provides authenticated SOCKS5 proxy on port 1080

- **client**: SDK test application
  - Only connected to the internal network (no direct internet access)
  - Must route all traffic through the proxy to reach LaunchDarkly

The client container is on an isolated Docker network (`internal: true`), which physically blocks direct internet access. This proves that the SDK must use the configured proxy to communicate with LaunchDarkly.

## Running the Test

```bash
# Set your LaunchDarkly mobile key
export LD_MOBILE_KEY="your-mobile-key-here"

# Run the test
docker compose up --build
```

**Note:** The Dockerfile uses Ubuntu 24.04 to ensure Boost 1.81+ is available.

## Supported Proxy Types

The SDK (via CURL) supports:
- HTTP proxies: `http://proxy:port`
- HTTPS proxies: `https://proxy:port`
- SOCKS4 proxies: `socks4://proxy:port`
- SOCKS5 proxies: `socks5://proxy:port`
- SOCKS5 with auth: `socks5://user:pass@proxy:port`
- SOCKS5 with hostname resolution: `socks5h://user:pass@proxy:port`

## Environment Variables

The test configures the following environment variables:

- `ALL_PROXY` - Proxy for all protocols (set to `socks5h://proxyuser:proxypass@proxy:1080`)
- `LD_MOBILE_KEY` - LaunchDarkly mobile key for testing
- `LD_LOG_LEVEL` - Set to `debug` for verbose logging

CURL also respects:
- `HTTP_PROXY` - Proxy for HTTP requests
- `HTTPS_PROXY` - Proxy for HTTPS requests
- `NO_PROXY` - Comma-separated list of hosts to bypass proxy

## Expected Output

### Success Case
```
Test 1: Verifying proxy connectivity...
✓ Proxy is reachable at proxy:1080

Test 2: Verifying direct access to LaunchDarkly is blocked...
✓ Direct access blocked (network is properly isolated)

Test 3: Verifying proxy can reach LaunchDarkly...
✓ Proxy can reach LaunchDarkly endpoints

Test 4: Running SDK client with proxy...
*** SDK successfully initialized!
*** Feature flag 'my-boolean-flag' is true for this user

✓ SDK successfully initialized through proxy!
✓ Flag evaluation succeeded

================================
✓ ALL TESTS PASSED

The SDK successfully:
  - Connected through the SOCKS5 proxy
  - Established SSE streaming connection
  - Retrieved feature flag values
  - Posted analytics events (if enabled)
```

### Failure Case (no proxy access)
```
Test 2: Verifying direct access to LaunchDarkly is blocked...
✓ Direct access blocked (network is properly isolated)

Test 3: Verifying proxy can reach LaunchDarkly...
✗ Proxy cannot reach LaunchDarkly endpoints
```

## Purpose of the test

The test validates proxy functionality through network isolation:

1. **Network Architecture**: The client container is on an isolated Docker network (`internal: true`) that blocks all external connections
2. **Direct Access Test**: With proxy variables unset, `curl` cannot reach LaunchDarkly (proves isolation)
3. **SDK Success**: The SDK successfully connects when `ALL_PROXY` is configured (proves proxy usage)

Since the client cannot reach the internet directly, the SDK MUST be using the proxy to succeed.

## Troubleshooting

### Enable CURL verbose logging

Set the `CURL_VERBOSE` environment variable in `docker-compose.yml`:
```yaml
environment:
  - CURL_VERBOSE=1
```

### Check proxy logs

```bash
docker compose logs proxy
```

You should see multiple SOCKS5 connections from the SDK client:
```
[INFO] socks: Connection from 172.18.0.3 to clientsdk.launchdarkly.com
[INFO] socks: Connection from 172.18.0.3 to events.launchdarkly.com
```

### Verify SDK is using proxy

The best proof is in the proxy logs. Each SDK network operation creates a connection:
- Initial SSE stream connection
- Event delivery (if enabled)

### Verify network isolation

```bash
# This should fail (timeout) because the client has no direct internet access
docker compose run client sh -c "unset ALL_PROXY && curl -v https://clientsdk.launchdarkly.com"
# Expected: Connection timeout or refused
```

### Test fails with DNS errors

If you see DNS resolution errors, ensure you're using `socks5h://` instead of `socks5://`. The `socks5h` protocol performs DNS resolution through the proxy, which is necessary when the client is on an isolated network.
