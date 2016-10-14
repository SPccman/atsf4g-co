include_directories(${PROJECT_SERVER_FRAME_INC_DIR})

# build libserver_frame.so
file(GLOB_RECURSE SRC_LIST
    RELATIVE ${PROJECT_SERVER_FRAME_SRC_DIR}
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.h
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.hpp
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.c
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.cpp
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.cc
)

# add generated files
include("${PROJECT_SERVER_FRAME_BAS_DIR}/protocol/generate_proto_source.cmake")
list(REMOVE_DUPLICATES SRC_LIST)

source_group_by_dir(SRC_LIST)
if ( NOT MSVC )
    add_definitions(-shared)
endif()

# these service will use atgateway protocol
include_directories(${ATFRAMEWORK_SERVICE_GATEWAY_PROTOCOL_DIR})

add_library(${PROJECT_SERVER_FRAME_LIB_LINK} SHARED
    ${SRC_LIST}
)

target_link_libraries(${PROJECT_SERVER_FRAME_LIB_LINK}
    ${PROJECT_LIB_LINK}
    ${3RD_PARTY_LIBCOPP_LINK_NAME}
    libatgw_inner_v1_c
    ${ATFRAMEWORK_SERVICE_LINK_NAME}
    ${ATFRAMEWORK_ATAPP_LINK_NAME}
    ${ATFRAMEWORK_ATBUS_LINK_NAME}
    ${ATFRAMEWORK_ATFRAME_UTILS_LINK_NAME}
    ${3RD_PARTY_PROTOBUF_LINK_NAME}
    ${3RD_PARTY_REDIS_LINK_NAME}
    ${3RD_PARTY_LIBUV_LINK_NAME}
    uuid
    ${COMPILER_OPTION_EXTERN_CXX_LIBS}
)
