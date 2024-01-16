## Verifying SDK build provenance with the SLSA framework

LaunchDarkly uses the [SLSA framework](https://slsa.dev/spec/v1.0/about) (Supply-chain Levels for Software Artifacts) to help developers make their supply chain more secure by ensuring the authenticity and build integrity of our published SDK packages.

As part of [SLSA requirements for level 3 compliance](https://slsa.dev/spec/v1.0/requirements), LaunchDarkly publishes provenance about our SDK package builds using [GitHub's generic SLSA3 provenance generator](https://github.com/slsa-framework/slsa-github-generator/blob/main/internal/builders/generic/README.md#generation-of-slsa3-provenance-for-arbitrary-projects) for distribution alongside our packages. These attestations are available for download from the GitHub release page for the release version under Assets > `OSNAME-multiple-provenance.intoto.jsonl`.

To verify SLSA provenance attestations, we recommend using [slsa-verifier](https://github.com/slsa-framework/slsa-verifier). Example usage for verifying SDK packages for Linux is included below:

```
# Ensure provenance file is downloaded along with packages for your OS
$ ls /tmp/launchdarkly-cpp-client-3.4.0
linux-gcc-x64-dynamic.zip              linux-gcc-x64-static.zip               linux-multiple-provenance.intoto.jsonl

# Run slsa-verifier to verify provenance against package artifacts 
$ slsa-verifier verify-artifact \
--provenance-path linux-multiple-provenance.intoto.jsonl \
--source-uri github.com/launchdarkly/cpp-sdks \
linux-gcc-x64-static.zip linux-gcc-x64-dynamic.zip
Verified signature against tlog entry index 59501683 at URL: https://rekor.sigstore.dev/api/v1/log/entries/24296fb24b8ad77ad75383b2cf5388a2587a27acf06c948205b60999c208ae5fcbe89fae6a6aae70
Verified build using builder "https://github.com/slsa-framework/slsa-github-generator/.github/workflows/generator_generic_slsa3.yml@refs/tags/v1.7.0" at commit 533d512ccf050e6bf50078d64ec97338dc03aaef
Verifying artifact linux-gcc-x64-static.zip: PASSED

Verified signature against tlog entry index 59501683 at URL: https://rekor.sigstore.dev/api/v1/log/entries/24296fb24b8ad77ad75383b2cf5388a2587a27acf06c948205b60999c208ae5fcbe89fae6a6aae70
Verified build using builder "https://github.com/slsa-framework/slsa-github-generator/.github/workflows/generator_generic_slsa3.yml@refs/tags/v1.7.0" at commit 533d512ccf050e6bf50078d64ec97338dc03aaef
Verifying artifact linux-gcc-x64-dynamic.zip: PASSED

PASSED: Verified SLSA provenance
```

Alternatively, to verify the provenance manually, the SLSA framework specifies [recommendations for verifying build artifacts](https://slsa.dev/spec/v1.0/verifying-artifacts) in their documentation.

**Note:** These instructions do not apply when building our SDKs from source. 
