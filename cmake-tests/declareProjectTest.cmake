# This file defines macros which can be used to setup
# new cmake project tests without introducing excessive boilerplate.

# declare_add_subdirectory_test(<name of test>):
#   Use when the test depends on launchdarkly via add_subdirectory.

# declare_find_package_test(<test name>):
#   Use when the test depends on launchdarkly via find_package.

# add_build_step(<test name>):
#   By default, the declare_* macros result in a test where "cmake -DSOMEVARIABLE=WHATEVER .."
#   (the cmake configure step) is invoked. This may be sufficient for a particular test,
#   for example testing that the configure step fails.
#   If the test should also invoke "cmake --build .", use this macro.

# require_configure_failure(<test name>):
#   Asserts that the cmake configure step should fail. For example, this would
#   happen if a required version of a dependency couldn't be satisfied with find_package.

# require_build_failure(<test name>):
#   Asserts that the cmake build step should fail.

macro(declare_add_subdirectory_test name)
    set(test_prefix ${name})

    add_test(
            NAME ${test_prefix}_configure
            COMMAND
            ${CMAKE_COMMAND}
            # Since project/CMakeLists.txt is going to call add_subdirectory(), it needs to know where
            # the SDK's project is (which is actually a couple directories above this particular file; not normally the case.)
            # The variable name is arbitrary.
            -DLAUNCHDARKLY_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            # Do not setup all of the SDK's testing machinery, which would normally happen when calling add_subdirectory.
            -DBUILD_TESTING=OFF
            # Forward variables from the SDK project to the test project, if set.
            $<$<BOOL:${CMAKE_GENERATOR_PLATFORM}>:-DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}>
            $<$<BOOL:${BOOST_LIBRARY}>:-DBOOST_LIBRARY=${BOOST_LIBRARY}>
            $<$<BOOL:${BOOST_INCLUDE_DIR}>:-DBOOST_INCLUDE_DIR=${BOOST_INCLUDE_DIR}>
            $<$<BOOL:${OPENSSL_LIBRARY}>:-DOPENSSL_LIBRARY=${OPENSSL_LIBRARY}>
            $<$<BOOL:${OPENSSL_INCLUDE_DIR}>:-DOPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}>
            ${CMAKE_CURRENT_SOURCE_DIR}/project
    )

    set_tests_properties(${test_prefix}_configure
            PROPERTIES
            FIXTURES_SETUP ${test_prefix}
            # Forward along the CC and CXX environment variables, because clang11 CI build uses them.
            ENVIRONMENT "CC=${CMAKE_C_COMPILER};CXX=${CMAKE_CXX_COMPILER}"
    )
endmacro()

macro(require_configure_failure name)
    set_tests_properties(${name}_configure PROPERTIES WILL_FAIL TRUE)
endmacro()

macro(require_build_failure name)
    set_tests_properties(${name}_build PROPERTIES WILL_FAIL TRUE)
endmacro()

macro(add_build_step name)
    # Setup a 'test' to perform the cmake build step.
    add_test(
            NAME ${name}_build
            COMMAND ${CMAKE_COMMAND} --build .
    )

    set_tests_properties(${name}_build
            PROPERTIES
            FIXTURES_REQUIRED ${name}
    )
endmacro()

macro(declare_find_package_test name)
    # This test assumes that the SDK has been installed at CMAKE_INSTALL_PREFIX.
    set(test_prefix ${name})

    add_test(
            NAME ${test_prefix}_configure
            COMMAND
            ${CMAKE_COMMAND}
            # Since project/CMakeLists.txt uses find_package(), it needs to know where to find
            # ldserverapiConfig.cmake. That can be found where the SDK is installed, which is CMAKE_INSTALL_PREFIX.
            -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
            # Forward variables from the SDK project to the test project, if set.
            $<$<BOOL:${CMAKE_GENERATOR_PLATFORM}>:-DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}>
            $<$<BOOL:${BOOST_LIBRARY}>:-DBOOST_LIBRARY=${BOOST_LIBRARY}>
            $<$<BOOL:${BOOST_INCLUDE_DIR}>:-DBOOST_INCLUDE_DIR=${BOOST_INCLUDE_DIR}>
            $<$<BOOL:${OPENSSL_LIBRARY}>:-DOPENSSL_LIBRARY=${OPENSSL_LIBRARY}>
            $<$<BOOL:${OPENSSL_INCLUDE_DIR}>:-DOPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}>
            ${CMAKE_CURRENT_SOURCE_DIR}/project
    )

    set_tests_properties(${test_prefix}_configure
            PROPERTIES
            FIXTURES_SETUP ${test_prefix}
            # Forward along the CC and CXX environment variables, because clang11 CI build uses them.
            ENVIRONMENT "CC=${CMAKE_C_COMPILER};CXX=${CMAKE_CXX_COMPILER}"
    )
endmacro()
