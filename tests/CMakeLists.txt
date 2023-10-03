set(gtest_force_shared_crt on)

# Add Google Test as a subdirectory
add_subdirectory(googletest)

add_definitions(-DCMAKE_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# Define the source files for the test executable
set(TEST_SOURCES
    test_main.cpp
    test_frame.cpp
    test_animation.cpp
)

# Create test executable using the sources and link it with necessary libraries
add_executable(${PROJECT_NAME}_test ${TEST_SOURCES} $<TARGET_OBJECTS:${PROJECT_NAME}Objs>)
target_link_libraries(${PROJECT_NAME}_test PRIVATE gtest_main Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia)
target_include_directories(${PROJECT_NAME}_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include) # Adjusted the path to include

# Register the tests
add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
