cmake_minimum_required(VERSION 3.16)

project(ImageMaskTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt MOC (Meta-Object Compiler)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Concurrent)

set(PROJECT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/ImageProcessor.cpp
    src/BlpReader.cpp
)

set(PROJECT_HEADERS
    include/MainWindow.h
    include/ImageProcessor.h
    include/BlpReader.h
)

add_executable(ImageMaskTool ${PROJECT_SOURCES} ${PROJECT_HEADERS})

target_include_directories(ImageMaskTool PRIVATE include)

target_link_libraries(ImageMaskTool PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core Qt6::Concurrent)

if(WIN32)
    set_property(TARGET ImageMaskTool PROPERTY WIN32_EXECUTABLE ON)
endif()
