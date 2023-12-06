set(CMAKE_CXX_STANDARD 23)
set(CMAKE_SUPPRESS_REGENERATION true)
project(internal)
add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DZYDIS_STATIC_BUILD)
add_definitions(-D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING -D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)
file(GLOB_RECURSE kiero_src 
	"${CMAKE_SOURCE_DIR}/libraries/kiero/*.h"
	"${CMAKE_SOURCE_DIR}/libraries/kiero/minhook/src/*.h"
	"${CMAKE_SOURCE_DIR}/libraries/kiero/minhook/src/*.c"
	"${CMAKE_SOURCE_DIR}/libraries/kiero/minhook/src/hde/*.h"
	"${CMAKE_SOURCE_DIR}/libraries/kiero/minhook/src/hde/*.c"
	"${CMAKE_SOURCE_DIR}/libraries/kiero/minhook/include/*.h"
	"${CMAKE_SOURCE_DIR}/libraries/kiero/minhook/dll_resources/*.rc"
	"${CMAKE_SOURCE_DIR}/libraries/kiero/minhook/src/hde/*.def"
	"${CMAKE_SOURCE_DIR}/libraries/kiero/*.cpp")
file(GLOB_RECURSE core_sources "*.h" "*.cpp")
add_library(${PROJECT_NAME} SHARED ${core_sources} ${kiero_src})
include_directories($ENV{DXSDK_DIR}/Include)
target_link_directories(${PROJECT_NAME} PUBLIC "$ENV{DXSDK_DIR}/Lib/x64")
target_link_libraries(${PROJECT_NAME} PUBLIC imgui Dbghelp.lib d3dcompiler.lib d3d9.lib d3dx9.lib d3d11.lib d3dx11.lib dxgi.lib)
foreach(_source IN ITEMS ${core_sources})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()
foreach(_source IN ITEMS ${kiero_src})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()