#include <launchdarkly/api.hpp>
#include <launchdarkly/sse/sse.hpp>
#include <iostream>

int main() {
    if (auto num = launchdarkly::foo()) {
        std::cout << "Got: " << *num << '\n';
    } else {
        std::cout << "Got nothing\n";
    }

    // curl "https://stream-stg.launchdarkly.com/all?filter=even-flags-2" -H "Authorization: sdk-66a5dbe0-8b26-445a-9313-761e7e3d381b" -v
    auto client = launchdarkly::sse::Builder("https://stream-stg.launchdarkly.com/all").build();
    if (!client) {
        std::cout << "Failed to build client" << std::endl;
        return 1;
    }

}
