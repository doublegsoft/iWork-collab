##########################################
##    ▄▄▄▄  ▄▄▄  ▄▄▄▄                   ##
## ▀▀ ▀███  ███  ███▀            ▄▄     ##
## ██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀ ##
## ██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████   ##
## ██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄ ##
##########################################
cmake_minimum_required(VERSION 3.16)

project(iWork-collab LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (WIN32)
elseif (APPLE)
  set(PLATFORM                              "darwin")
elseif (UNIX)
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
  "gen/src/iWork-pkt.c"
  "gen/src/iWork-pkt-codec.c"
)

add_library(iWork-collab STATIC ${IWORK_COLLAB_SOURCES})
if (WIN32)
elseif (APPLE)
  add_library(iWork_collab_shared SHARED ${IWORK_COLLAB_SOURCES})
  set_target_properties(iWork_collab_shared PROPERTIES OUTPUT_NAME iWork_collab)
  set(LIBNAME                  "iWork_collab_shared")
elseif (UNIX)
  set(LIBNAME                  "iWork_collab")
endif()

add_library(iWork-ws STATIC
  "src/iWork-ws.c"
)

add_executable(iWork-collab-test)

target_sources(iWork-collab-test PRIVATE
  src/main.c
)

# 3. 链接库（最重要的一步）
target_link_libraries(iWork-collab-test PRIVATE
  iWork-collab
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
)

set_target_properties(iWork-collab-test PROPERTIES
  BUILD_RPATH "${IWORK_RUNTIME_RPATH}"
)

add_executable(iWork-server)

target_sources(iWork-server PRIVATE
  src/iWork-server.c
)

target_link_libraries(iWork-server PRIVATE
  iWork-collab
  iWork-ws
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
)

set_target_properties(iWork-server PROPERTIES
  BUILD_RPATH "${IWORK_RUNTIME_RPATH}"
)

add_executable(iWork-client)

target_sources(iWork-client PRIVATE
  src/iWork-client.c
)

target_link_libraries(iWork-client PRIVATE
  iWork-collab
  iWork-ws
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
)

set_target_properties(iWork-client PROPERTIES
  BUILD_RPATH "${IWORK_RUNTIME_RPATH}"
)

add_executable(perf)

target_sources(perf PRIVATE
  test/perf.c
)

target_link_libraries(perf PRIVATE
  iWork-collab
  ${LIBWEBSOCKETS_LIBRARIES}
)

add_executable(hello)

target_sources(hello PRIVATE
  test/hello.c
)

target_link_libraries(hello PRIVATE
  iWork-collab
  ${LIBWEBSOCKETS_LIBRARIES}
)

add_executable(iWork-pkt-codec-test)

target_compile_options(iWork-pkt-codec-test PRIVATE -g)

target_sources(iWork-pkt-codec-test PRIVATE
  gen/test/iWork-pkt-codec-test.c
)

target_link_libraries(iWork-pkt-codec-test PRIVATE
  iWork-collab
)
