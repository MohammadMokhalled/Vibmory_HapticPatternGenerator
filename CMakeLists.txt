# Minimum required version of CMake
cmake_minimum_required(VERSION 3.5)

# Define the project and its properties
project(Vibmory VERSION 1.1.0 LANGUAGES CXX)

# Enable automoc, autouic, and autorcc for Qt
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Set C++ standard and require it
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages and components
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Multimedia)

# MSVC & BUILD_SHARED_LIBS specific flags
if(MSVC AND BUILD_SHARED_LIBS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa /MT")
endif()

# Define source files, header files, and form files
set(SOURCES
    src/animation.cpp
    src/animationaudio.cpp
    src/createprojectwindow.cpp
    src/frame.cpp
    src/helper.cpp
    src/mainwindow.cpp
    src/paintingwidget.cpp
    src/projectsettingwindow.cpp
    src/animationcontroller.cpp
)

set(HEADERS
    include/animation.h
    include/animationaudio.h
    include/createprojectwindow.h
    include/frame.h
    include/helper.h
    include/mainwindow.h
    include/paintingwidget.h
    include/projectsettingwindow.h
    include/animationcontroller.h
)

set(FORMS
    ui/createprojectwindow.ui
    ui/mainwindow.ui
    ui/projectsettingwindow.ui
)

# Define object library with target properties and link libraries
add_library(${PROJECT_NAME}Objs OBJECT ${SOURCES} ${HEADERS} ${FORMS})
target_include_directories(${PROJECT_NAME}Objs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}Objs PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia)
set_target_properties(${PROJECT_NAME}Objs PROPERTIES AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

# Define the main executable
add_executable(${PROJECT_NAME} src/main.cpp $<TARGET_OBJECTS:${PROJECT_NAME}Objs>)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia)

# Configure Google Test
enable_testing()
add_subdirectory(tests)

# Define installation directories
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Special handling for Qt6 projects
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
