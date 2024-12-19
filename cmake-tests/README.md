## CMake project tests overview

This directory contains tests for various integration techniques that users of the
LaunchDarkly C++ SDKs may employ.

Each test takes the form of a minimal CMake project with a `CMakeLists.txt` and `main.cpp`.
An additional `CMakeLists.txt` sets up the test properties.

Structure:

```
some_test_directory
    project                 # Contains the CMake project under test.
        - main.cpp          # Minimal code that invokes LaunchDarkly SDK.
        - CMakeLists.txt    # CMake configuration that builds the project executable.
    - CMakeLists.txt        # CMake configuration that sets up the CTest machinery for this test.
```

*Important note about `main.cpp`*:

The optimizer employed by whatever toolchain is building the project might omit function definitions in the SDK
during static linking, if those functions are proven to be unused.

The code in the main file should not have any branches that allow this to happen
(such as a check for an SDK key, like in the hello demo projects.)

This could obscure linker errors that would have otherwise been caught.

## CMake test setup

The toplevel `CMakeLists.txt` in each subdirectory is responsible for setting up
the actual CTest tests that configure and build the projects.

Note, the logic described below is encapsulated in two macros defined in `declareProjectTest.cmake`, so that
that new tests don't need to copy boilerplate.

Test creation is generally done in two phases:

1) Make a test that configures the project (simulating `cmake .. [options]`)
2) Make a test that builds the project (simulating `cmake --build .`)

The tests are ordered via `set_tests_properties` to ensure the configure test
runs before the build test, as would be expected.

The test creation logic harbors additional complexity because these tests are executed
in CI on multiple types of executors (Windows/Mac/Linux) in various configurations.

In particular, some environment variables must be forwarded to each test project CMake configuration.
These include `C` and `CXX` variables, which are explicitly set/overridden in the `clang11` CI build.
Without setting these, the test would fail to build with the same compilers as the SDK.

Additionally, certain variables must be forwarded to each test project CMake configuration.

| Variable           | Explanation      |
|--------------------|------------------|
| `BOOST_ROOT`       | Path to Boost.   |
| `OPENSSL_ROOT_DIR` | Path to OpenSSL. |

## Tests

### cmake_projects/test_add_subdirectory

Checks that a project can include the SDK as a sub-project, via `add_subdirectory`.
This would be a likely use-case when the repo is a submodule of another project.

### cmake_projects/test_find_package

Checks that a project can include the SDK via `find_package(ldserverapi)`.
This would be a likely use-case if the SDK was installed on the system by the user.

**NOTE:** Requires SDK to be installed.
