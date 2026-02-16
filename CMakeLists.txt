cmake_minimum_required(VERSION 3.16)

project(CSIGA2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CSIGA2_FUN "Enable fun features (video player, virtual mic hook)" ON)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
set(SDL3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL")

if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
  message(FATAL_ERROR "Dear ImGui not found. Clone the Dear ImGui repo into external/imgui.")
endif()

add_library(imgui STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp
)

target_include_directories(imgui PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libswscale libswresample libavutil)

if(EXISTS "${SDL3_DIR}/CMakeLists.txt")
  add_subdirectory(${SDL3_DIR} EXCLUDE_FROM_ALL)
else()
  message(FATAL_ERROR "SDL3 not found. Clone https://github.com/libsdl-org/SDL into external/SDL.")
endif()

if(TARGET SDL3::SDL3)
  target_link_libraries(imgui PUBLIC SDL3::SDL3)
elseif(TARGET SDL3::SDL3-shared)
  target_link_libraries(imgui PUBLIC SDL3::SDL3-shared)
else()
  message(FATAL_ERROR "SDL3 target not found after adding external/SDL.")
endif()

target_link_libraries(imgui PUBLIC OpenGL::GL)


add_executable(CSIGA2
  src/main.cpp
  src/app/app.cpp
  src/platform/x11_helpers.cpp
  src/ui/menu.cpp
  src/ui/assets.cpp
  src/ui/ostin_style.cpp
  src/ui/state.cpp
  src/ui/theme.cpp
  src/ui/widgets.cpp
  src/audio/mvp_music.cpp
  src/config/config_manager.cpp
  src/platform/input.cpp
  src/functionalities/aimbot/aimbot.cpp
  src/functionalities/clickity/clickity.cpp
  src/functionalities/radar/radar.cpp
  src/functionalities/visuals/see-trough/see_trough_demo.cpp
  src/functionalities/visuals/see-trough/esp.cpp
  # Memory module
  src/memory/process.cpp
  src/memory/schema.cpp
  src/memory/find_offsets.cpp
  src/memory/entity/weapon.cpp
  src/memory/entity/player.cpp
  src/memory/entity/entity.cpp
)

set_target_properties(CSIGA2 PROPERTIES
  BUILD_RPATH "$ORIGIN"
  INSTALL_RPATH "$ORIGIN"
)

target_link_libraries(CSIGA2 PRIVATE imgui)

if(TARGET SDL3::SDL3)
  target_link_libraries(CSIGA2 PRIVATE SDL3::SDL3)
elseif(TARGET SDL3::SDL3-shared)
  target_link_libraries(CSIGA2 PRIVATE SDL3::SDL3-shared)
endif()

target_include_directories(CSIGA2 PRIVATE
  ${SDL3_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  "${CMAKE_CURRENT_SOURCE_DIR}/ignorer/Free ImGui Menu From Ostin/examples/example_win32_directx11"
  ${FFMPEG_INCLUDE_DIRS}
)

if(TARGET X11::X11)
  target_link_libraries(CSIGA2 PRIVATE X11::X11 X11::Xext)
else()
  target_include_directories(CSIGA2 PRIVATE ${X11_INCLUDE_DIR})
  target_link_libraries(CSIGA2 PRIVATE ${X11_LIBRARIES} ${X11_Xext_LIB})
endif()

target_link_libraries(CSIGA2 PRIVATE ${FFMPEG_LIBRARIES} rt)
target_compile_options(CSIGA2 PRIVATE ${FFMPEG_CFLAGS_OTHER})

if(CSIGA2_FUN)
  target_sources(CSIGA2 PRIVATE
    src/audio/virtual_mic.cpp
    src/video/video_player.cpp
  )
  target_compile_definitions(CSIGA2 PRIVATE CSIGA2_FUN=1)
  message(STATUS "Fun mode: ON (video player + virtual mic)")
else()
  message(STATUS "Fun mode: OFF (local-only MVP music, no video player)")
endif()

add_custom_target(prod
  COMMAND ${CMAKE_COMMAND}
    -DPROD_DIR=${CMAKE_BINARY_DIR}/prod
    -DTARGET_FILE=$<TARGET_FILE:CSIGA2>
    -DASSETS_DIR=${CMAKE_SOURCE_DIR}/assets
    -P ${CMAKE_SOURCE_DIR}/cmake/pack_prod.cmake
  DEPENDS CSIGA2
)

