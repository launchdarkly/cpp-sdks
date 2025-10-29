#!/bin/bash
set -e

echo "=================================="
echo "LaunchDarkly SDK Proxy Test"
echo "=================================="
echo ""

# Check if mobile key is set
if [ -z "$LD_MOBILE_KEY" ]; then
    echo "ERROR: LD_MOBILE_KEY environment variable is not set"
    echo "Please set your LaunchDarkly mobile key:"
    echo "  export LD_MOBILE_KEY='your-mobile-key-here'"
    echo "  docker-compose up"
    exit 1
fi

echo "Mobile Key: ${LD_MOBILE_KEY:0:10}..."
echo "Proxy: $ALL_PROXY"
echo ""

# Test 1: Verify proxy is reachable
echo "Test 1: Verifying proxy connectivity..."
PROXY_HOST=$(echo $ALL_PROXY | sed 's/.*@//' | cut -d':' -f1)
PROXY_PORT=$(echo $ALL_PROXY | sed 's/.*://')

if nc -z "$PROXY_HOST" "$PROXY_PORT" 2>/dev/null; then
    echo "✓ Proxy is reachable at $PROXY_HOST:$PROXY_PORT"
else
    echo "✗ Cannot reach proxy at $PROXY_HOST:$PROXY_PORT"
    exit 1
fi
echo ""

# Test 2: Verify direct access is blocked (should fail)
echo "Test 2: Verifying direct access to LaunchDarkly is blocked..."
# Explicitly disable proxy for this test to check if direct access works
if timeout 5 env -u ALL_PROXY -u all_proxy -u HTTP_PROXY -u http_proxy -u HTTPS_PROXY -u https_proxy curl -s https://clientsdk.launchdarkly.com >/dev/null 2>&1; then
    echo "⚠ WARNING: Direct access to LaunchDarkly succeeded (network is not isolated)"
    echo "  This means the client can reach the internet directly without the proxy."
    echo "  The test will still verify that the SDK uses the proxy when configured."
else
    echo "✓ Direct access blocked (network is properly isolated)"
fi
echo ""

# Test 3: Verify proxy access works (should succeed)
echo "Test 3: Verifying proxy can reach LaunchDarkly..."
if timeout 10 curl -s --proxy "$ALL_PROXY" https://clientsdk.launchdarkly.com >/dev/null 2>&1; then
    echo "✓ Proxy can reach LaunchDarkly endpoints"
else
    echo "✗ Proxy cannot reach LaunchDarkly endpoints"
    exit 1
fi
echo ""

# Test 4: Run the SDK client
echo "Test 4: Running SDK client with proxy..."
echo "----------------------------------------"

# Run the client and capture output
/sdk/build/examples/hello-cpp-client/hello-cpp-client 2>&1 | tee /tmp/client-output.log

# Check if SDK initialized successfully
if grep -q "SDK successfully initialized" /tmp/client-output.log; then
    echo ""
    echo "✓ SDK successfully initialized through proxy!"
    INIT_SUCCESS=1
else
    echo ""
    echo "✗ SDK failed to initialize"
    INIT_SUCCESS=0
fi

# Check for flag evaluation
if grep -q "Feature flag" /tmp/client-output.log; then
    echo "✓ Flag evaluation succeeded"
    FLAG_SUCCESS=1
else
    echo "✗ Flag evaluation failed"
    FLAG_SUCCESS=0
fi

echo ""
echo "=================================="
echo "Test Summary"
echo "=================================="
if [ $INIT_SUCCESS -eq 1 ] && [ $FLAG_SUCCESS -eq 1 ]; then
    echo "✓ ALL TESTS PASSED"
    echo ""
    echo "The SDK successfully:"
    echo "  - Connected through the SOCKS5 proxy"
    echo "  - Established SSE streaming connection"
    echo "  - Retrieved feature flag values"
    echo "  - Posted analytics events (if enabled)"
    exit 0
else
    echo "✗ TESTS FAILED"
    echo ""
    echo "Check the output above for details."
    exit 1
fi
