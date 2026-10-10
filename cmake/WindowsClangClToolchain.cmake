# Win32 cross build on Linux with clang-cl and lld-link (MSVC ABI, so Valve's win32 vgui.lib links).
# The MSVC CRT and Windows SDK come from xwin, laid out by:
#   xwin --accept-license --arch x86 splat --output ~/.cache/xwin/splat-x86
# Point XWIN_DIR elsewhere if the splat lives somewhere else. scripts/build-win.sh drives this.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR X86)
set(CMAKE_SYSTEM_VERSION 10.0)

if (DEFINED ENV{XWIN_DIR})
	set(XWIN_DIR "$ENV{XWIN_DIR}")
else()
	set(XWIN_DIR "$ENV{HOME}/.cache/xwin/splat-x86")
endif()

if (DEFINED ENV{LLVM_BIN})
	set(LLVM_BIN "$ENV{LLVM_BIN}")
else()
	set(LLVM_BIN "/usr/lib/llvm-21/bin")
endif()

set(CMAKE_C_COMPILER "${LLVM_BIN}/clang-cl")
set(CMAKE_CXX_COMPILER "${LLVM_BIN}/clang-cl")
set(CMAKE_LINKER "${LLVM_BIN}/lld-link")
set(CMAKE_AR "${LLVM_BIN}/llvm-lib")
set(CMAKE_RC_COMPILER "${LLVM_BIN}/llvm-rc")
set(CMAKE_MT "${LLVM_BIN}/llvm-mt")

set(CMAKE_C_COMPILER_TARGET i686-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET i686-pc-windows-msvc)

set(_xwin_inc
	"/imsvc${XWIN_DIR}/crt/include"
	"/imsvc${XWIN_DIR}/sdk/include/ucrt"
	"/imsvc${XWIN_DIR}/sdk/include/um"
	"/imsvc${XWIN_DIR}/sdk/include/shared")
string(JOIN " " _xwin_inc_flags ${_xwin_inc})
set(CMAKE_C_FLAGS_INIT "--target=i686-pc-windows-msvc ${_xwin_inc_flags}")
set(CMAKE_CXX_FLAGS_INIT "--target=i686-pc-windows-msvc ${_xwin_inc_flags}")

set(_xwin_lib
	"/libpath:${XWIN_DIR}/crt/lib/x86"
	"/libpath:${XWIN_DIR}/sdk/lib/um/x86"
	"/libpath:${XWIN_DIR}/sdk/lib/ucrt/x86")
string(JOIN " " _xwin_lib_flags ${_xwin_lib})
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_xwin_lib_flags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_xwin_lib_flags}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_xwin_lib_flags}")

# xwin ships no debug CRT, so CMake's compiler probes must link the release one.
set(CMAKE_TRY_COMPILE_CONFIGURATION Release)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
