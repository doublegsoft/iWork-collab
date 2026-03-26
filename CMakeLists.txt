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

add_executable(perf)

target_sources(perf PRIVATE
  test/perf.c
)

# 3. 链接库（最重要的一步）
target_link_libraries(iWork-collab-test PRIVATE
  iWork-collab
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
)