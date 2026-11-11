cmake_minimum_required(VERSION 3.24)

project(ARPGPOV VERSION 1.6.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Build CommonLibSSE-NG (alandtse fork, supports 1.5.97 / 1.6.x / 1.7.x) from extern/.
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/CommonLibSSE-NG CommonLibSSE EXCLUDE_FROM_ALL)
include(extern/CommonLibSSE-NG/cmake/CommonLibSSE.cmake)

# The outline renderer (src/render, src/base, src/config) is from Highlight Lootable Corpses
# by AmazingBuff, GPL-3.0: https://github.com/AmazingBuff/HighlightLootableCorpses
file(GLOB_RECURSE OUTLINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/render/*.cpp)

# Its HLSL shaders are embedded into a generated header and compiled when the game starts.
set(RENDER_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/render/shaders)
set(RENDER_SHADER_EMBED
	icon_overlay.hlsl:IconOverlay
	mask_geometry.hlsl:MaskGeometry
	mask_composite.hlsl:MaskComposite
	mask_glow.hlsl:MaskGlow
)
set(GENERATED_SHADER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/src/render/shader_sources.h)
add_custom_command(
	OUTPUT ${GENERATED_SHADER_HEADER}
	COMMAND ${CMAKE_COMMAND}
		-DSHADER_DIR=${RENDER_SHADER_DIR}
		-DOUTPUT_HEADER=${GENERATED_SHADER_HEADER}
		"-DEMBED_ENTRIES=${RENDER_SHADER_EMBED}"
		-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_shaders.cmake
	DEPENDS
		${RENDER_SHADER_DIR}/icon_overlay.hlsl
		${RENDER_SHADER_DIR}/mask_geometry.hlsl
		${RENDER_SHADER_DIR}/mask_composite.hlsl
		${RENDER_SHADER_DIR}/mask_glow.hlsl
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_shaders.cmake
	COMMENT "Embedding HLSL sources (render/shaders)"
	VERBATIM
)

add_commonlibsse_plugin(${PROJECT_NAME}
	AUTHOR "Nick"
	SOURCES
		src/main.cpp
		src/Camera.cpp
		src/Cut.cpp
		src/Game.cpp
		src/Globes.cpp
		src/Hud.cpp
		src/Input.cpp
		src/Menu.cpp
		src/Occlusion.cpp
		src/Outline.cpp
		src/Settings.cpp
		${OUTLINE_SOURCES}
		${GENERATED_SHADER_HEADER}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

# SKSEMenuFramework.h (SKSE Menu Framework's public header: draws the cursor, labels and settings pages).
target_include_directories(${PROJECT_NAME} PRIVATE include src ${CMAKE_CURRENT_BINARY_DIR}/src)

# Outline renderer: Direct3D, plus Microsoft Detours for its hook into the interface draw.
find_package(fmt CONFIG REQUIRED)
find_path(DETOURS_INCLUDE_DIRS "detours/detours.h" REQUIRED)
find_library(DETOURS_LIBRARY detours REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${DETOURS_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt d3d11 dxgi d3dcompiler ${DETOURS_LIBRARY})
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /wd4200)
endif()
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

# Copy the built DLL into a ready-to-zip mod folder after each build.
set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist/SKSE/Plugins")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${DIST_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/ARPGPOV.ini" "${DIST_DIR}"
)
