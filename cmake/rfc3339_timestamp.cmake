FetchContent_Declare(timestamp
        GIT_REPOSITORY https://github.com/chansen/c-timestamp
        GIT_TAG "b205c407ae6680d23d74359ac00444b80989792f"
)

FetchContent_GetProperties(timestamp)
if (NOT timestamp_POPULATED)
    FetchContent_Populate(timestamp)
endif ()

add_library(timestamp OBJECT
        ${timestamp_SOURCE_DIR}/timestamp_tm.c
        ${timestamp_SOURCE_DIR}/timestamp_valid.c
        ${timestamp_SOURCE_DIR}/timestamp_parse.c
)

if (BUILD_SHARED_LIBS)
    set_target_properties(timestamp PROPERTIES
            POSITION_INDEPENDENT_CODE 1
            C_VISIBILITY_PRESET hidden
    )
endif ()

target_include_directories(timestamp PUBLIC
        $<BUILD_INTERFACE:${timestamp_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/timestamp>
)

install(
        TARGETS timestamp
        EXPORT launchdarklyTargets
)
