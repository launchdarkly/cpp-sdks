# Required for Apple Silicon support.
cmake_minimum_required(VERSION 3.19)

project(
        LaunchDarklyCPPSDKTestHarnessDataModel
        VERSION 0.1
        DESCRIPTION "LaunchDarkly CPP SDK Test Harness Data Model definitions"
        LANGUAGES CXX
)

include(${CMAKE_FILES}/json.cmake)


add_library(contract-test-data-model src/data_model.cpp)
target_link_libraries(contract-test-data-model PUBLIC nlohmann_json::nlohmann_json
        )
target_include_directories(contract-test-data-model PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )
