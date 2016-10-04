file (GLOB_RECURSE PROTO_DESC_LIST_PBDESC
    "${CMAKE_CURRENT_LIST_DIR}/pbdesc/*.proto"
)

file (GLOB_RECURSE PROTO_DESC_LIST_CONFIG
    "${CMAKE_CURRENT_LIST_DIR}/config/*.proto"
)

# ============= configure protocols =============

foreach(PROTO_FILE_FULL_PATH ${PROTO_DESC_LIST_CONFIG})
    file(RELATIVE_PATH PROTO_FILE ${CMAKE_CURRENT_LIST_DIR} ${PROTO_FILE_FULL_PATH})
    STRING(REGEX REPLACE "[^/]proto" "" PROTO_NAME ${PROTO_FILE})
    set(PROTO_SRC "${PROTO_NAME}.pb.h" "${PROTO_NAME}.pb.cc")

    list(APPEND PROJECT_SERVER_FRAME_PROTO_FILES ${PROTO_FILE})
    list(APPEND PROJECT_SERVER_FRAME_PROTO_SRCS ${PROTO_SRC})
    list(APPEND SRC_LIST ${PROTO_SRC})
endforeach()
add_custom_command(OUTPUT ${PROJECT_SERVER_FRAME_PROTO_SRCS}
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    --proto_path "config"
    --cpp_out "config"
    #-o "${PROJECT_INSTALL_RES_PBD_DIR}/config.pb"
    ${PROJECT_SERVER_FRAME_PROTO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    DEPENDS ${PROTO_DESC_LIST_CONFIG}
    COMMENT "Generate ${PROJECT_SERVER_FRAME_PROTO_FILES} to ${PROJECT_SERVER_FRAME_PROTO_SRCS}"
)

# ============= network protocols =============
file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/pbdesc")
unset(PROJECT_SERVER_FRAME_PROTO_FILES)
unset(PROJECT_SERVER_FRAME_PROTO_SRCS)
foreach(PROTO_FILE_FULL_PATH ${PROTO_DESC_LIST_PBDESC})
    file(RELATIVE_PATH PROTO_FILE ${CMAKE_CURRENT_LIST_DIR} ${PROTO_FILE_FULL_PATH})
    STRING(REGEX REPLACE "[^/]proto" "" PROTO_NAME ${PROTO_FILE})
    set(PROTO_SRC "${PROTO_NAME}.pb.h" "${PROTO_NAME}.pb.cc")

    list(APPEND PROJECT_SERVER_FRAME_PROTO_FILES ${PROTO_FILE})
    list(APPEND PROJECT_SERVER_FRAME_PROTO_SRCS ${PROTO_SRC})
    list(APPEND SRC_LIST ${PROTO_SRC})
endforeach()

add_custom_command(OUTPUT ${PROJECT_SERVER_FRAME_PROTO_SRCS}
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    --proto_path "pbdesc"
    --cpp_out "pbdesc"
    #-o "${PROJECT_INSTALL_RES_PBD_DIR}/config.pb"
    ${PROJECT_SERVER_FRAME_PROTO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    DEPENDS ${PROTO_DESC_LIST_PBDESC}
    COMMENT "Generate ${PROJECT_SERVER_FRAME_PROTO_FILES} to ${PROJECT_SERVER_FRAME_PROTO_SRCS}"
)

unset(PROJECT_SERVER_FRAME_PROTO_FILES)
unset(PROJECT_SERVER_FRAME_PROTO_SRCS)