file (GLOB_RECURSE PROTO_DESC_LIST_PBDESC
    "${CMAKE_CURRENT_LIST_DIR}/pbdesc/*.proto"
)

file (GLOB_RECURSE PROTO_DESC_LIST_CONFIG
    "${CMAKE_CURRENT_LIST_DIR}/config/*.proto"
)

foreach(PROTO_FILE_FULL_PATH ${PROTO_DESC_LIST_CONFIG})
    file(RELATIVE_PATH PROTO_FILE ${CMAKE_CURRENT_LIST_DIR} ${PROTO_FILE_FULL_PATH})
    STRING(REGEX REPLACE "[^/]proto" "" PROTO_NAME ${PROTO_FILE})
    set(PROTO_SRC "${PROTO_NAME}.pb.h" "${PROTO_NAME}.pb.cc")
    add_custom_command(OUTPUT ${PROTO_SRC}
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} 
        --proto_path "${CMAKE_CURRENT_LIST_DIR}/config"
        --cpp_out "${CMAKE_CURRENT_LIST_DIR}/config"
        #-o "${PROJECT_INSTALL_RES_PBD_DIR}/config.pb"
        ${PROTO_DESC_LIST_CONFIG}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        DEPENDS ${PROTO_DESC_LIST_CONFIG}
        COMMENT "Generate ${PROTO_FILE} to ${PROTO_SRC}"
    )

    list(APPEND SRC_LIST ${PROTO_SRC})
endforeach()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/pbdesc")
foreach(PROTO_FILE_FULL_PATH ${PROTO_DESC_LIST_PBDESC})
    file(RELATIVE_PATH PROTO_FILE ${CMAKE_CURRENT_LIST_DIR} ${PROTO_FILE_FULL_PATH})
    STRING(REGEX REPLACE "[^/]proto" "" PROTO_NAME ${PROTO_FILE})
    set(PROTO_SRC "${PROTO_NAME}.pb.h" "${PROTO_NAME}.pb.cc")
    add_custom_command(OUTPUT ${PROTO_SRC}
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} 
        --proto_path "${CMAKE_CURRENT_LIST_DIR}/pbdesc"
        --cpp_out "${CMAKE_CURRENT_LIST_DIR}/pbdesc"
        #-o "${PROJECT_INSTALL_RES_PBD_DIR}/config.pb"
        ${PROTO_DESC_LIST_PBDESC}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        DEPENDS ${PROTO_DESC_LIST_PBDESC}
        COMMENT "Generate ${PROTO_FILE} to ${PROTO_SRC}"
    )

    list(APPEND SRC_LIST ${PROTO_SRC})
endforeach()