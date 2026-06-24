cmake_minimum_required(VERSION 3.20)
project (AlgoLens CXX C)

# Set the global C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Windows-specific settings
if (MSVC)
  add_compile_options(/Z7)
  # Static CRT (/MT) everywhere to match CEF. MUST be before any
  # add_library / add_executable / FetchContent, or libs default to /MD.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Get the glad from external folder
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/external/glad")

include (FetchContent)

# Fetching SDL3 latest version from the repository
FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.10)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# Fetching NanoVG
FetchContent_Declare(nanovg
  GIT_REPOSITORY https://github.com/memononen/nanovg.git
  GIT_TAG ce3bf74)
FetchContent_MakeAvailable(nanovg)
set(NANOVG_DIR "${nanovg_SOURCE_DIR}")

#  Fetching json
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
  DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_MakeAvailable(json)

# ---- glad (OpenGL 3.3 loader)
add_library(glad STATIC "${GLAD_DIR}/src/glad.c")
target_include_directories(glad PUBLIC
  "${GLAD_DIR}/include"
)

# ---- NanoVG
add_library(nanovg STATIC "${NANOVG_DIR}/src/nanovg.c")
target_compile_definitions(nanovg PUBLIC NANOVG_GL3)
target_include_directories(nanovg PUBLIC
  "${NANOVG_DIR}/src"
)

# ---- json
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${json_SOURCE_DIR})

# ---- Creating a ipc shared memory between two processes
add_library(ipc STATIC source/ipc/SharedMemory.cpp)
target_include_directories(ipc PUBLIC source/ipc)

add_library(http STATIC source/http/HTTPServer.cpp)
target_include_directories(http PUBLIC source/http)
target_link_libraries(http PUBLIC ws2_32)

# ---- CEF (Chromium Embedded Framework)
set(CEF_ROOT "${CMAKE_SOURCE_DIR}/external/cef")
list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")

# Use the plain-executable path
set(USE_SANDBOX OFF CACHE BOOL "" FORCE)

set(CEF_DEBUG_INFO_FLAG "/Z7" CACHE STRING "" FORCE)

find_package(CEF REQUIRED)

add_subdirectory(${CEF_ROOT}/libcef_dll libcef_dll_wrapper)
# ---- CEF end

# Logical target that lnks libcef.lib (Debug/Release)
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

# Where CEF binaries + resources get copied.
SET_CEF_TARGET_OUT_DIR()

# ---- The shell (CEF host)
add_executable(AlgoLens WIN32
  source/shell/main.cpp
  source/shell/AlgoLensApp.cpp
  source/shell/AlgoLensClient.cpp
  source/bridge/IPCBridge.cpp
  source/process/ProcessManager.cpp
)

SET_EXECUTABLE_TARGET_PROPERTIES(AlgoLens)
set_target_properties(AlgoLens PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CEF_TARGET_OUT_DIR}")
add_dependencies(AlgoLens libcef_dll_wrapper)
target_link_libraries(AlgoLens libcef_lib libcef_dll_wrapper http ipc nlohmann_json ${CEF_STANDARD_LIBS})
target_include_directories(AlgoLens PRIVATE ${CEF_ROOT})

# Copy libcef.dll, .pak/.dat resources, locales next to AlgoLens.exe.
COPY_FILES(AlgoLens "${CEF_BINARY_FILES}"   "${CEF_BINARY_DIR}"   "${CEF_TARGET_OUT_DIR}")
COPY_FILES(AlgoLens "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

add_executable(AlgoLensRenderer
  source/renderers/components/variable/Variables.cpp
  source/renderers/components/array/Array.cpp
  source/renderers/components/grid/Grid.cpp
  source/renderers/components/stack/Stack.cpp
  source/renderers/components/queue/Queue.cpp
  source/ir/IRParser.cpp
  source/drawing/nanovg_impl.cpp
  source/app.cpp
  main.cpp
)

target_link_libraries(AlgoLensRenderer PRIVATE
  nlohmann_json
  SDL3::SDL3
  opengl32
  nanovg
  glad
  ipc
)

set_target_properties(AlgoLensRenderer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CEF_TARGET_OUT_DIR}")


# ---- The test writer (stands in for the CEF shell)
add_executable(IRWriter source/tools/ir_writer_main.cpp)
target_link_libraries(IRWriter PRIVATE ipc)

# ---- HTTP server smoke test
add_executable(HTTPServerTest source/tools/http_test_main.cpp)
target_link_libraries(HTTPServerTest PRIVATE http)

add_custom_command(TARGET AlgoLensRenderer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:AlgoLensRenderer>"
  COMMENT "Copying SDL3.dll....")
