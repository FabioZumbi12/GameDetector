cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(OBSGameDetector VERSION 1.0.0)

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        psapi
        version
    )
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    "src/plugin-main.cpp" "src/GameDetectorDock.cpp" "src/ConfigManager.cpp" "src/GameDetector.cpp" "src/GameDetectorSettingsDialog.cpp" "src/IconProvider.cpp")

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME "obs-game-detector")

install(
  FILES "data/locale/en-US.ini" "data/locale/pt-BR.ini"
  DESTINATION "${OBS_PLUGIN_DESTINATION_DATA}/obs-plugins/${CMAKE_PROJECT_NAME}/locale"
)
