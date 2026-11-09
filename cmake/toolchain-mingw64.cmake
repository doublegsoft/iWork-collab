# =========================================================================
# CMake Toolchain File for MinGW-w64 on macOS
# Target: Windows 64-bit (x86_64)
# =========================================================================

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 1. 查找 Homebrew 安装的 MinGW-w64 路径
execute_process(
  COMMAND brew --prefix mingw-w64
  OUTPUT_VARIABLE MINGW_BREW_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

# 如果不是 brew 默认环境，回退到常见路径
if(NOT MINGW_BREW_PREFIX)
  if(EXISTS "/opt/homebrew/opt/mingw-w64")
    set(MINGW_BREW_PREFIX "/opt/homebrew/opt/mingw-w64")
  else()
    set(MINGW_BREW_PREFIX "/usr/local/opt/mingw-w64")
  endif()
endif()

# 2. 设置编译器与工具
set(TOOLCHAIN_PREFIX "x86_64-w64-mingw32")

find_program(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
find_program(CMAKE_RC_COMPILER  ${TOOLCHAIN_PREFIX}-windres)
find_program(CMAKE_AR           ${TOOLCHAIN_PREFIX}-ar)
find_program(CMAKE_RANLIB       ${TOOLCHAIN_PREFIX}-ranlib)

# 3. 设置 Windows 目标系统的根前缀与查找策略
set(CMAKE_FIND_ROOT_PATH 
  "${MINGW_BREW_PREFIX}/toolchain-x86_64"
  "${MINGW_BREW_PREFIX}"
  "/usr/local/Cellar/mingw-w64"
)

# 确保在交叉编译时不误引用 macOS 本地的库和头文件
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)