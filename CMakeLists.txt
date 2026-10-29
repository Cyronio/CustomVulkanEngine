cmake_minimum_required (VERSION 3.5.0)
project (
  "slime_ranger"
  VERSION 2.0.0
  DESCRIPTION "A demo game showcasing a basic custom game engine using the Vulkan API"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/D IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
	add_compile_options(/D_CRT_SECURE_NO_WARNINGS) #for assimp
	add_compile_options(/EHsc /std:c++20 /DEBUG /Zi /Istb) #for assimp
else()
    add_compile_options(-D IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
	add_compile_options(-Wno-nullability-completeness)
	add_compile_options(-D_CRT_SECURE_NO_WARNINGS) #for assimp
endif()

find_package(Vulkan REQUIRED)
set(VULKAN $ENV{VULKAN_SDK}) #set to Vulkan SDK

set(BUILD ${PROJECT_SOURCE_DIR}/build)  # engine include directory
set(INCLUDE ${PROJECT_SOURCE_DIR}/include)  # engine include directory

#--------------------------------- Fetch ImGui ---------------------------------#
include(FetchContent) # enable Fetch Content

FetchContent_Declare(imgui
                     GIT_REPOSITORY https://github.com/ocornut/imgui.git 
                     GIT_TAG master)
FetchContent_MakeAvailable(Imgui)
#include_directories (${imgui_SOURCE_DIR})
#include_directories(${imgui_SOURCE_DIR}/backends)
set(IMGUI ${imgui_SOURCE_DIR})

set(TARGET slime_ranger)

set(SOURCE
  ${IMGUI}/imgui.cpp
  ${IMGUI}/imgui_demo.cpp
  ${IMGUI}/imgui_draw.cpp
  ${IMGUI}/backends/imgui_impl_sdl2.cpp
  ${IMGUI}/backends/imgui_impl_vulkan.cpp
  ${IMGUI}/imgui_tables.cpp
  ${IMGUI}/imgui_widgets.cpp
  slime_ranger.cpp
  )

set(HEADERS )

add_executable(${TARGET} ${SOURCE} ${HEADERS})

set(IRRKLANG_DIR "${PROJECT_SOURCE_DIR}/../irrKlang")

target_include_directories(${TARGET} PRIVATE "${IRRKLANG_DIR}/include")

target_link_libraries(${TARGET} PRIVATE "${IRRKLANG_DIR}/lib/irrKlang.lib")

target_include_directories(${TARGET} PUBLIC stb)
target_include_directories(${TARGET} PUBLIC ${VULKAN}/Include)
target_include_directories(${TARGET} PUBLIC ${VULKAN}/Include/SDL2)
target_include_directories(${TARGET} PUBLIC ${VULKAN}/Include/Volk)
target_include_directories(${TARGET} PUBLIC ${VULKAN}/Include/glm)
target_include_directories(${TARGET} PUBLIC ${VULKAN}/Include/vma)
target_include_directories(${TARGET} PUBLIC ${IMGUI})
target_include_directories(${TARGET} PUBLIC ${IMGUI}/backends)

target_link_libraries (${TARGET} PUBLIC ${VULKAN}/lib/SDL2.lib)
target_link_libraries (${TARGET} PUBLIC ${VULKAN}/lib/volk.lib)

