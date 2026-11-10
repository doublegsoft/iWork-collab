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
set(LIBWEBSOCKETS_DIR                       "${CMAKE_CURRENT_SOURCE_DIR}/../iWork-3rd/libwebsockets-4.5.4")
set(LIBWEBSOCKETS_INCLUDE_DIR               "${LIBWEBSOCKETS_DIR}/include"
                                            "${LIBWEBSOCKETS_DIR}/${BUILD}/include")
set(LIBWEBSOCKETS_LIBRARY_DIR               "${LIBWEBSOCKETS_DIR}/${BUILD}/lib")
set(LIBWEBSOCKETS_LIBRARIES                 websockets websockets-evlib_uv)

################################################################################
##                                                                            ##
##                                    libuv                                   ##
##                                                                            ##
################################################################################
set(LIBUV_DIR                               "${CMAKE_CURRENT_SOURCE_DIR}/../iWork-3rd/libuv-1.52.1")
set(LIBUV_INCLUDE_DIR                       "${LIBUV_DIR}/include")
set(LIBUV_LIBRARY_DIR                       "${LIBUV_DIR}/${BUILD}")
set(LIBUV_LIBRARIES                         uv)

################################################################################
##                                                                            ##
##                                    libpq                                   ##
##                                                                            ##
################################################################################
set(LIBPQ_DIR                               "${CMAKE_CURRENT_SOURCE_DIR}/../iWork-3rd/pgsql-windows")
set(LIBPQ_INCLUDE_DIR                       "${LIBPQ_DIR}/include")
if(APPLE)
set(LIBPQ_LIBRARY_DIR                       "/opt/homebrew/Cellar/libpq/18.4/lib")
elseif(MINGW)
set(LIBPQ_LIBRARY_DIR                       "${LIBPQ_DIR}/lib")
else(LINUX)
set(LIBPQ_LIBRARY_DIR                       "/usr/lib/x86_64-linux-gnu")
set(LIBPQ_INCLUDE_DIR                       "/usr/include/postgresql")
endif()
set(LIBPQ_LIBRARIES                         pq)

################################################################################
##                                                                            ##
##                                     cui                                    ##
##                                                                            ##
################################################################################
set(CUI_DIR                               "${CMAKE_CURRENT_SOURCE_DIR}/../iWork-3rd/CUI-1.1.0")
set(CUI_INCLUDE_DIR                       "${CUI_DIR}/include")
set(CUI_LIBRARY_DIR                       "${CUI_DIR}/${BUILD}")
set(CUI_LIBRARIES                         cui)

################################################################################
##                                                                            ##
##                                  together                                  ##
##                                                                            ##
################################################################################
set(TOGETHER_DIR                            "${CMAKE_CURRENT_SOURCE_DIR}/3rd/together")
set(TOGETHER_INCLUDE_DIR                    "${TOGETHER_DIR}/src" 
                                            "${TOGETHER_DIR}/gen")
set(TOGETHER_LIBRARY_DIR                    "${TOGETHER_DIR}/${BUILD}")
set(TOGETHER_LIBRARIES                      together)

################################################################################
##                                                                            ##
##                                   goolib                                   ##
##                                                                            ##
################################################################################
set(GOOLIB_DIR                              "${CMAKE_CURRENT_SOURCE_DIR}/3rd/goolib")
set(GOOLIB_INCLUDE_DIR                      "${GOOLIB_DIR}/src")
set(GOOLIB_LIBRARY_DIR                      "${GOOLIB_DIR}/${BUILD}")
set(GOOLIB_LIBRARIES                        goolib)

################################################################################
##                                                                            ##
##                                   openssl                                   ##
##                                                                            ##
################################################################################
set(OPENSSL_DIR                             "${GOOLIB_DIR}/3rd/openssl-3.6.0")
set(OPENSSL_INCLUDE_DIR                     "${OPENSSL_DIR}/include"
                                            "${OPENSSL_DIR}/${BUILD}/include")
set(OPENSSL_LIBRARY_DIR                     "${OPENSSL_DIR}/${BUILD}")
set(OPENSSL_LIBRARIES                       crypto ssl)

################################################################################
##                                                                            ##
##                                    curl                                    ##
##                                                                            ##
################################################################################
set(CURL_DIR                             "${GOOLIB_DIR}/3rd/curl-8.16.0")
set(CURL_INCLUDE_DIR                     "${CURL_DIR}/include")
set(CURL_LIBRARY_DIR                     "${CURL_DIR}/${BUILD}")
set(CURL_LIBRARIES                       curl)

include_directories(
  "include"
  "gen/include"
  ${LIBWEBSOCKETS_INCLUDE_DIR}
  ${LIBUV_INCLUDE_DIR}
  ${LIBPQ_INCLUDE_DIR}
  ${TOGETHER_INCLUDE_DIR}
  ${GOOLIB_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
  ${CUI_INCLUDE_DIR}
)

link_directories(
  ${LIBWEBSOCKETS_LIBRARY_DIR}
  ${LIBUV_LIBRARY_DIR}
  ${LIBPQ_LIBRARY_DIR}
  ${TOGETHER_LIBRARY_DIR}
  ${OPENSSL_LIBRARY_DIR}
  ${CURL_LIBRARY_DIR}
  ${CUI_LIBRARY_DIR}
)

################################################################################
##                                                                            ##
##                                    tool                                    ##
##                                                                            ##
################################################################################

## gaussdb import tool
add_executable(iw-gaussimp)

target_sources(iw-gaussimp PRIVATE
  "src/tool/iw-gaussimp.c"
)

target_link_libraries(iw-gaussimp PRIVATE
  ${LIBPQ_LIBRARIES}
)

## mail monitor tool
add_executable(iw-mailmon)

target_sources(iw-mailmon PRIVATE
  "src/tool/iw-mailmon.c"
  "${GOOLIB_DIR}/src/goolib-mail.c"
  "${GOOLIB_DIR}/src/internal/smtp.c"
)

target_link_libraries(iw-mailmon PRIVATE
  ${OPENSSL_LIBRARIES}
  ${CURL_LIBRARIES}
)

## sync for maven project
add_executable(iw-mavensync)

target_sources(iw-mavensync PRIVATE
  "src/tool/iw-mavensync.c"
)

## unpack markdown tool
add_executable(iw-unpack)

target_sources(iw-unpack PRIVATE
  src/iw-unpack.c
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

add_executable(iw-node)

target_sources(iw-node PRIVATE
  src/iw-node.c
)

# 3. 链接库（最重要的一步）
target_link_libraries(iw-node PRIVATE
  iw_collab
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
  ${TOGETHER_LIBRARIES}
)

set_target_properties(iw-node PROPERTIES
  BUILD_RPATH "${IWORK_RUNTIME_RPATH}"
)

################################################################################
##                                                                            ##
##                                  iw-server                                 ##
##                                                                            ##
################################################################################

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
  BUILD_RPATH "$ORIGIN"
  INSTALL_RPATH "$ORIGIN"
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

add_executable(iw-send-test)

target_sources(iw-send-test PRIVATE
  test/iw-send-test.c
)

target_link_libraries(iw-send-test PRIVATE
  together
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
##                            iw-pkt-codec-test (gen)                         ##
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

add_executable(iw-test-coding)

target_sources(iw-test-coding PRIVATE
  test/iw-test-coding.c
)

target_link_libraries(iw-test-coding PRIVATE
  iw_collab
  ${LIBWEBSOCKETS_LIBRARIES}
  ${LIBUV_LIBRARIES}
)

set_target_properties(iw-test-coding PROPERTIES
  BUILD_RPATH "${IWORK_RUNTIME_RPATH}"
)

################################################################################
##                                                                            ##
##                                   DESKTOP                                  ##
##                                                                            ##
################################################################################
if (APPLE)
  add_executable(iw-abpms)

  target_sources(iw-abpms PRIVATE
    src/desktop/iw-abpms.c
  )

  target_link_libraries(iw-abpms PRIVATE
    ${CUI_LIBRARIES}
    "-framework Cocoa"
    "-framework CoreGraphics"
    "-framework OpenGL"
    "-framework QuartzCore"
  )

  add_executable(iw-gauss)

  target_sources(iw-gauss PRIVATE
    src/desktop/iw-gauss.c
  )


  target_link_libraries(iw-gauss PRIVATE
    ${CUI_LIBRARIES}
    "-framework Cocoa"
    "-framework CoreGraphics"
    "-framework OpenGL"
    "-framework QuartzCore"
  )
endif()