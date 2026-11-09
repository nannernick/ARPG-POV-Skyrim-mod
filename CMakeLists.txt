cmake_minimum_required(VERSION 3.24)

project(ARPGPointClick VERSION 1.1.0 LANGUAGES CXX)

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

add_commonlibsse_plugin(${PROJECT_NAME}
	AUTHOR "Nick"
	SOURCES
		src/main.cpp
		src/Game.cpp
		src/Hud.cpp
		src/Input.cpp
		src/Settings.cpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

# SKSEMenuFramework.h (SKSE Menu Framework's public header: draws the cursor, labels and settings page).
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

# Copy the built DLL into a ready-to-zip mod folder after each build.
set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist/SKSE/Plugins")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${DIST_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/ARPGPointClick.ini" "${DIST_DIR}"
)
