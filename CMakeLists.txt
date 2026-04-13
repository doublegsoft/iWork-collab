##########################################
##    ▄▄▄▄  ▄▄▄  ▄▄▄▄                   ##
## ▀▀ ▀███  ███  ███▀            ▄▄     ##
## ██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀ ##
## ██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████   ##
## ██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄ ##
##########################################
cmake_minimum_required(VERSION 3.16)

project(iw-collab LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (WIN32)
elseif (APPLE)
  set(PLATFORM                              "darwin")
elseif (UNIX)
  add_definitions(-D_POSIX_C_SOURCE=200809L)
  set(PLATFORM                              "linux")
endif()

set(BUILD                                   "build/${PLATFORM}")

################################################################################
##                                                                            ##
##                                libwebsockets                               ##
##                                                                            ##
################################################################################
set(LIBWEBSOCKETS_DIR                       "3rd/libwebsockets-4.5.4")
set(LIBWEBSOCKETS_INCLUDE_DIR               "${LIBWEBSOCKETS_DIR}/include"
                                            "${LIBWEBSOCKETS_DIR}/${BUILD}/include")
set(LIBWEBSOCKETS_LIBRARY_DIR               "${LIBWEBSOCKETS_DIR}/${BUILD}/lib")
set(LIBWEBSOCKETS_LIBRARIES                 websockets websockets-evlib_uv)

################################################################################
##                                                                            ##
##                                    libuv                                   ##
##                                                                            ##
################################################################################
set(LIBUV_DIR                               "3rd/libuv-1.52.1")
set(LIBUV_INCLUDE_DIR                       "${LIBUV_DIR}/include")
set(LIBUV_LIBRARY_DIR                       "${LIBUV_DIR}/${BUILD}")
set(LIBUV_LIBRARIES                         uv)

include_directories(
  "include"
  "gen/include"
  ${LIBWEBSOCKETS_INCLUDE_DIR}
  ${LIBUV_INCLUDE_DIR}
)

link_directories(
  ${LIBWEBSOCKETS_LIBRARY_DIR}
  ${LIBUV_LIBRARY_DIR}
)

set(IWORK_RUNTIME_RPATH
  "${CMAKE_CURRENT_SOURCE_DIR}/${LIBWEBSOCKETS_LIBRARY_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/${LIBUV_LIBRARY_DIR}"
)

set(IWORK_COLLAB_SOURCES
  "gen/src/iw-pkt.c"
  "gen/src/iw-pkt-codec.c"
  "src/iw-ws.c"
)

add_library(iw_collab STATIC ${IWORK_COLLAB_SOURCES})
if (WIN32)
elseif (APPLE)
  add_library(iw_collab_shared SHARED ${IWORK_COLLAB_SOURCES})
  set_target_properties(iw_collab_shared PROPERTIES OUTPUT_NAME iw_collab)
  set(LIBNAME                  "iw_collab_shared")
elseif (UNIX)
  set(LIBNAME                  "iw_collab")
endif()

add_executable(iw-collab-test)

target_sources(iw-collab-test PRIVATE
  src/main.c
)

# 3. 链接库（最重要的一步）
target_link_libraries(iw-collab-test PRIVATE
  iw_collab
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
)

set_target_properties(iw-collab-test PROPERTIES
  BUILD_RPATH "${IWORK_RUNTIME_RPATH}"
)

add_executable(iw-server)

target_sources(iw-server PRIVATE
  src/iw-server.c
)

target_link_libraries(iw-server PRIVATE
  iw_collab
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
)

set_target_properties(iw-server PROPERTIES
  BUILD_RPATH "${IWORK_RUNTIME_RPATH}"
)

add_executable(iw-client)

target_sources(iw-client PRIVATE
  src/iw-client.c
)

target_link_libraries(iw-client PRIVATE
  iw_collab
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
)

set_target_properties(iw-client PROPERTIES
  BUILD_RPATH "${IWORK_RUNTIME_RPATH}"
)

add_executable(perf)

target_sources(perf PRIVATE
  test/perf.c
)

target_link_libraries(perf PRIVATE
  iw_collab
  ${LIBWEBSOCKETS_LIBRARIES}
)

add_executable(hello)

target_sources(hello PRIVATE
  test/hello.c
)

target_link_libraries(hello PRIVATE
  iw_collab
  ${LIBWEBSOCKETS_LIBRARIES}
)

################################################################################
##                                                                            ##
##                           iw-pkt-codec-test (gen)                       ##
##                                                                            ##
################################################################################

add_executable(iw-pkt-codec-test)

target_compile_options(iw-pkt-codec-test PRIVATE -g)

target_sources(iw-pkt-codec-test PRIVATE
  "gen/src/iw-pkt-codec.c"
  "gen/src/iw-pkt.c"
  "gen/test/iw-pkt-codec-test.c"
)

target_link_libraries(iw-pkt-codec-test PRIVATE
  
)

add_executable(iw-server-ping-test)

target_sources(iw-server-ping-test PRIVATE
  test/iw-server-ping-test.c
)

target_link_libraries(iw-server-ping-test PRIVATE
  iw_collab
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
)

set_target_properties(iw-server-ping-test PROPERTIES
  BUILD_RPATH "${IWORK_RUNTIME_RPATH}"
)

add_executable(iw-server-prompt-test)

target_sources(iw-server-prompt-test PRIVATE
  test/iw-server-prompt-test.c
)

target_link_libraries(iw-server-prompt-test PRIVATE
  iw_collab
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
)

set_target_properties(iw-server-prompt-test PROPERTIES
  BUILD_RPATH "${IWORK_RUNTIME_RPATH}"
)
