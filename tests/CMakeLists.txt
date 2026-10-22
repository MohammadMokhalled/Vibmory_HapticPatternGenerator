# Core regressions need neither Qt nor downloaded test frameworks.
add_executable(vibmory_core_tests core_tests.cpp)
target_link_libraries(vibmory_core_tests PRIVATE vibmory_application vibmory_audio vibmory_persistence vibmory_options)
target_compile_definitions(vibmory_core_tests PRIVATE VIBMORY_TEST_RESOURCES="${CMAKE_CURRENT_SOURCE_DIR}/resources")
foreach(suite domain codec audio session playback)
    add_test(NAME ${suite} COMMAND vibmory_core_tests ${suite})
endforeach()

add_executable(vibmory_codec_properties codec_properties.cpp)
target_link_libraries(vibmory_codec_properties PRIVATE vibmory_persistence vibmory_options)
add_test(NAME codec_properties COMMAND vibmory_codec_properties)
set_tests_properties(codec_properties PROPERTIES TIMEOUT 60)

find_package(Python3 3.12 QUIET COMPONENTS Interpreter)
if(Python3_FOUND)
    add_executable(vibmory_wave_fixture wave_fixture.cpp)
    target_link_libraries(vibmory_wave_fixture PRIVATE vibmory_audio vibmory_options)
    add_test(NAME independent_wave_decode COMMAND Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/verify_wave.py" "$<TARGET_FILE:vibmory_wave_fixture>")
endif()

if(VIBMORY_BUILD_GUI)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    qt_add_executable(vibmory_ui_tests ui_tests.cpp)
    target_link_libraries(vibmory_ui_tests PRIVATE vibmory_ui vibmory_platform Qt6::Test vibmory_options)
    add_test(NAME ui COMMAND vibmory_ui_tests -platform offscreen)
    set_tests_properties(ui PROPERTIES ENVIRONMENT "QT_LOGGING_RULES=qt.multimedia.*=false")
endif()
