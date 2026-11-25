cmake_minimum_required(VERSION 3.27)

project(omarchy-hyprland-inertia
  DESCRIPTION "Touchpad inertia for Omarchy"
  VERSION 0.1.2
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

# Built as inertia.so (no lib prefix). Omarchy loads it from the fixed path
# /usr/lib/omarchy-hyprland-inertia/inertia.so.
add_library(inertia SHARED src/main.cpp)
set_target_properties(inertia PROPERTIES PREFIX "")
# Without this, GNU unique symbols keep the library mapped after
# `hyprctl plugin unload`, so turning inertia off would not really unload it.
target_compile_options(inertia PRIVATE -fno-gnu-unique)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET
  hyprland
  libdrm
  libinput
  libudev
  pixman-1
  wayland-server
  xkbcommon
)
target_link_libraries(inertia PRIVATE PkgConfig::deps)

install(TARGETS inertia LIBRARY DESTINATION lib/omarchy-hyprland-inertia)
