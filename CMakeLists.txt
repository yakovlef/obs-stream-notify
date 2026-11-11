cmake_minimum_required(VERSION 3.28...3.30)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)
project(${_name} VERSION ${_version} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(compilerconfig)
include(defaults)
include(helpers)

add_library(
  ${CMAKE_PROJECT_NAME} MODULE
  src/plugin.cpp
  src/config.cpp
  src/config.hpp
  src/message-format.cpp
  src/message-format.hpp
  src/notifier.hpp
  src/secrets.cpp
  src/secrets.hpp
  src/settings-dialog.cpp
  src/settings-dialog.hpp
)
find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs OBS::obs-frontend-api Qt6::Core Qt6::Widgets)
set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(WIN32)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE NOMINMAX)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Crypt32)
endif()
