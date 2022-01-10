cmake_minimum_required(VERSION 3.13)

project(ExpView)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

if(NOT CMAKE_BUILD_TYPE)
    message("CMAKE_BUILD_TYPE not set, setting it to Release")
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

############################################################################
# Add OpenGL

find_package(OpenGL REQUIRED)


############################################################################
# Add SFML

find_package(SFML 2 COMPONENTS system window graphics)


############################################################################


set(EXPVIEW_SRC
    "source/Main.cpp"
    "source/imgui/imgui.cpp"
    "source/imgui/imgui_draw.cpp"
    "source/imgui/imgui_tables.cpp"
    "source/imgui/imgui_widgets.cpp"
    "source/imgui/imgui-SFML.cpp"
    "source/imgui/implot.cpp"
    "source/imgui/implot_items.cpp"
)

add_executable(ExpView ${EXPVIEW_SRC})

set_property(TARGET ExpView PROPERTY CXX_STANDARD 17)
set_property(TARGET ExpView PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories(ExpView PUBLIC ${SFML_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})

target_link_libraries(ExpView ${SFML_LIBRARIES} ${OPENGL_LIBRARIES} ${SFML_DEPENDENCIES})
