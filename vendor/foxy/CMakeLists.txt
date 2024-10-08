#
# Copyright (c) 2018-2019 Christian Mazakas (christian dot mazakas at gmail dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/LeonineKing1199/foxy
#

cmake_minimum_required(VERSION 3.13)

set(foxy_minimum_boost_version 1.75)

project(
        foxy

        LANGUAGES
        CXX

        VERSION
        1.0.0

        DESCRIPTION
        "Session-based abstractions for Beast + URL parsing and pct-coding"
)

set(CMAKE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(${CMAKE_FILES}/certify.cmake)

find_package(
        Boost ${foxy_minimum_boost_version}
        REQUIRED
        system
        date_time
)

find_package(OpenSSL REQUIRED)

if (UNIX)
    set(THREADS_PREFER_PTHREAD_FLAG ON CACHE BOOL "")
endif ()

find_package(Threads REQUIRED)

add_library(
        foxy OBJECT
        include/foxy.hpp
        include/foxy/client_session.hpp
        include/foxy/code_point_iterator.hpp
        include/foxy/code_point_view.hpp
        include/foxy/error.hpp
        include/foxy/listener.hpp
        include/foxy/log.hpp
        include/foxy/multi_stream.hpp
        include/foxy/parse_uri.hpp
        include/foxy/pct_decode.hpp
        include/foxy/pct_encode.hpp
        include/foxy/proxy.hpp
        include/foxy/server_session.hpp
        include/foxy/session_opts.hpp
        include/foxy/session.hpp
        include/foxy/speak.hpp
        include/foxy/type_traits.hpp
        include/foxy/uri_parts.hpp
        include/foxy/uri.hpp
        include/foxy/utf8.hpp
        include/foxy/utility.hpp

        include/foxy/detail/close_stream.hpp
        include/foxy/detail/export_connect_fields.hpp
        include/foxy/detail/has_token.hpp
        include/foxy/detail/relay.hpp
        include/foxy/detail/timed_op_wrapper_v3.hpp
        include/foxy/detail/tunnel.hpp

        include/foxy/impl/session.impl.hpp

        include/foxy/impl/client_session/async_connect.impl.hpp
        include/foxy/impl/client_session/async_request.impl.hpp

        include/foxy/impl/server_session/async_detect_ssl.impl.hpp
        include/foxy/impl/server_session/async_handshake.impl.hpp

        include/foxy/impl/session/async_read.impl.hpp
        include/foxy/impl/session/async_read_header.impl.hpp
        include/foxy/impl/session/async_write.impl.hpp
        include/foxy/impl/session/async_write_header.impl.hpp

        src/log.cpp
        src/proxy.cpp
        src/parse_uri.cpp
        src/utility.cpp

        # TODO: someday make this work
        #
        # src/uri.cpp
)

target_compile_features(foxy PUBLIC cxx_generic_lambdas)

if (MSVC)
    # Win10
    #
    set(FOXY_WIN_ARCH "0x0A00" CACHE STRING "Asio-specific Win architecture value (default: Win10)")
    target_compile_definitions(foxy PUBLIC "_WIN32_WINNT=${FOXY_WIN_ARCH}")

    target_compile_options(foxy PUBLIC "/bigobj")

    # keep this around, just in case
    target_link_libraries(foxy PUBLIC Boost::disable_autolinking)
endif ()

set(FOXY_QUIET ON CACHE BOOL "Suppresses messages internal to Beast/Asio about deprecated features")
if (FOXY_QUIET)
    target_compile_definitions(
            foxy

            PUBLIC
            BOOST_COROUTINES_NO_DEPRECATION_WARNING=1
            BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE=1
            BOOST_BEAST_ALLOW_DEPRECATED=1
    )
endif ()

set(FOXY_FAST_BUILD ON CACHE BOOL "Builds Foxy with separately compiled Beast and Asio")
if (FOXY_FAST_BUILD)
    target_compile_definitions(
            foxy

            PUBLIC
            BOOST_ASIO_SEPARATE_COMPILATION=1
            BOOST_BEAST_SEPARATE_COMPILATION=1
    )

    target_sources(foxy PRIVATE src/asio.cpp src/beast.cpp)
endif ()

target_include_directories(
        foxy

        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)

target_link_libraries(
        foxy

        PUBLIC
        Boost::boost
        Boost::system
        Boost::date_time
        OpenSSL::SSL
        Threads::Threads
        PRIVATE
        certify::core
)

add_library(
        test_utils

        test/include/foxy/test/helpers/ssl_ctx.hpp
        test/include/foxy/test/helpers/ssl_ctx.cpp
)

target_include_directories(test_utils PUBLIC test/include)
target_link_libraries(test_utils PUBLIC foxy)

cmake_dependent_option(FOXY_BUILD_TESTING
        "Build the C++ unit tests."
        ON
        "BUILD_TESTING"
        OFF
)

if (FOXY_BUILD_TESTING)
    include(CTest)

    find_package(Catch2 CONFIG REQUIRED)
    find_package(Boost ${foxy_minimum_boost_version} REQUIRED coroutine thread container)

    add_executable(
            foxy_tests

            test/allocator_client_test.cpp
            test/client_session_test.cpp
            test/code_point_view_test.cpp
            test/export_connect_fields_test.cpp
            test/iterator_test.cpp
            test/listener_test.cpp
            test/main.cpp
            test/parse_uri_test.cpp
            test/pct_decode_test.cpp
            test/pct_encode_test.cpp
            test/proxy_test.cpp
            test/proxy_test2.cpp
            test/relay_test.cpp
            test/server_session_test.cpp
            test/session_test.cpp
            test/speak_test.cpp
            test/ssl_client_session_test.cpp
            test/timed_op_wrapper_v3.cpp
            test/unicode_uri_test.cpp
            test/uri_test.cpp
            test/utility_test.cpp
    )

    target_link_libraries(
            foxy_tests
            PRIVATE
            test_utils
            Catch2::Catch2
            Boost::coroutine
            Boost::thread
            Boost::container
    )

    file(COPY test/root-cas.pem DESTINATION ${CMAKE_BINARY_DIR})
    include(Catch)
    catch_discover_tests(foxy_tests)
endif ()

if (FOXY_BUILD_EXAMPLES)
    find_package(Boost ${foxy_minimum_boost_version} REQUIRED coroutine thread container)

    add_executable(hello-world examples/hello_world/main.cpp)
    target_link_libraries(hello-world PRIVATE foxy Boost::coroutine Boost::thread)

    add_executable(ip-parser examples/uri_combinators/ip_parser.cpp)
    target_link_libraries(ip-parser PRIVATE foxy Boost::coroutine Boost::thread)

    add_executable(client examples/allocators/client.cpp)
    target_link_libraries(client PRIVATE foxy Boost::container Boost::thread)

    add_executable(server examples/server/main.cpp)
    target_link_libraries(server PRIVATE foxy Boost::container Boost::thread Boost::coroutine)

    add_executable(https-server examples/https_server/main.cpp)
    target_link_libraries(
            https-server
            PRIVATE
            foxy
            test_utils
            Boost::container
            Boost::thread
            Boost::coroutine
    )

    add_executable(listener examples/listener/main.cpp)
    target_link_libraries(listener PRIVATE foxy test_utils Boost::thread Boost::coroutine)
endif ()

if (FOXY_FUZZ)
    add_executable(uri-parser fuzz/parse_uri.cpp)
    target_link_libraries(uri-parser PRIVATE foxy)
    target_compile_options(uri-parser PRIVATE "-fsanitize=fuzzer")
    target_link_options(uri-parser PRIVATE "-fsanitize=fuzzer")
endif ()

install(
        TARGETS foxy
        EXPORT ${LD_TARGETS_EXPORT_NAME}
)
