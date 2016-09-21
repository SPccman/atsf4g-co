
# =========== 3rd_party redis ==================
set (3RD_PARTY_REDIS_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set (3RD_PARTY_REDIS_PKG_DIR "${3RD_PARTY_REDIS_BASE_DIR}/pkg")
set (3RD_PARTY_REDIS_HAPP_DIR "${3RD_PARTY_REDIS_BASE_DIR}/hiredis-happ")

if (NOT EXISTS ${3RD_PARTY_REDIS_HAPP_DIR})
    find_package(Git)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init -f --remote "3rd_party/redis/hiredis-happ"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endif()

set (3RD_PARTY_REDIS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/prebuilt/${PLATFORM_BUILD_PLATFORM_NAME}")
set (LIBHIREDIS_ROOT ${3RD_PARTY_REDIS_ROOT_DIR})

if (NOT WIN32 AND NOT CYGWIN)
    find_package(Libhiredis)

    if (NOT LIBHIREDIS_FOUND)
        if(NOT EXISTS ${3RD_PARTY_REDIS_PKG_DIR})
            file(MAKE_DIRECTORY ${3RD_PARTY_REDIS_PKG_DIR})
        endif()

        message(STATUS "hiredis not found try to build it.")
        if (NOT EXISTS "${3RD_PARTY_REDIS_PKG_DIR}/hiredis")
            find_package(Git)
            if(GIT_FOUND)
                message(STATUS "git found: ${GIT_EXECUTABLE}")
                execute_process(COMMAND ${GIT_EXECUTABLE} clone "https://github.com/owent-contrib/hiredis" hiredis
                    WORKING_DIRECTORY ${3RD_PARTY_REDIS_PKG_DIR}
                )
            endif()
        else()
            message(STATUS "use cache hiredis sources")
        endif()

        execute_process(COMMAND ${CMAKE_MAKE_PROGRAM} "PREFIX=${3RD_PARTY_REDIS_ROOT_DIR}" install
            WORKING_DIRECTORY "${3RD_PARTY_REDIS_PKG_DIR}/hiredis"
        )

        find_package(Libhiredis)
    endif()
else()
    # windows 下仅用来看代码
    set(Libhiredis_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/prebuilt/linux_x86_64/include/hiredis")
    set(Libhiredis_LIBRARIES hiredis)
    set(LIBHIREDIS_FOUND TRUE)
endif()

if(LIBHIREDIS_FOUND)
    EchoWithColor(COLOR GREEN "-- Dependency: redis lib found.(${Libhiredis_LIBRARIES})")
else()
    EchoWithColor(COLOR RED "-- Dependency: redis lib is required")
    message(FATAL_ERROR "redis libs not found")
endif()

set (3RD_PARTY_REDIS_INC_DIR "${Libhiredis_INCLUDE_DIRS}")
set (3RD_PARTY_REDIS_LINK_NAME hiredis-happ ${Libhiredis_LIBRARIES})

include_directories(${3RD_PARTY_REDIS_INC_DIR} "${3RD_PARTY_REDIS_HAPP_DIR}/include")
