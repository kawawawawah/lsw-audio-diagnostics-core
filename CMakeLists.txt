# SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.22)

project(LSWAudioDiagnosticsCore VERSION 0.1.0 LANGUAGES CXX)

option(LSW_AUDIO_DIAG_BUILD_TESTS "Build unit tests" ON)
option(LSW_AUDIO_DIAG_BUILD_EXAMPLES "Build examples" ON)
option(LSW_AUDIO_DIAG_ENABLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/CompilerWarnings.cmake)

add_library(lsw_audio_diagnostics STATIC
    src/analyzer.cpp
    src/correlation_tracker.cpp
    src/finite_value_guard.cpp
    src/level_tracker.cpp)
add_library(LSW::AudioDiagnostics ALIAS lsw_audio_diagnostics)

target_compile_features(lsw_audio_diagnostics PUBLIC cxx_std_17)
set_target_properties(lsw_audio_diagnostics PROPERTIES
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME AudioDiagnostics)
target_include_directories(lsw_audio_diagnostics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
lsw_audio_diag_set_compiler_warnings(lsw_audio_diagnostics ${LSW_AUDIO_DIAG_ENABLE_WARNINGS_AS_ERRORS})

if(LSW_AUDIO_DIAG_BUILD_TESTS)
    enable_testing()
    find_package(Threads REQUIRED)
    add_executable(lsw_audio_diagnostics_tests
        tests/test_main.cpp
        tests/analyzer_tests.cpp
        tests/correlation_tests.cpp
        tests/dc_offset_tests.cpp
        tests/finite_value_tests.cpp
        tests/level_tests.cpp)
    target_link_libraries(lsw_audio_diagnostics_tests PRIVATE LSW::AudioDiagnostics Threads::Threads)
    target_compile_features(lsw_audio_diagnostics_tests PRIVATE cxx_std_17)
    set_target_properties(lsw_audio_diagnostics_tests PROPERTIES CXX_EXTENSIONS OFF)
    lsw_audio_diag_set_compiler_warnings(lsw_audio_diagnostics_tests ${LSW_AUDIO_DIAG_ENABLE_WARNINGS_AS_ERRORS})
    add_test(NAME lsw_audio_diagnostics_tests COMMAND lsw_audio_diagnostics_tests)
endif()

if(LSW_AUDIO_DIAG_BUILD_EXAMPLES)
    add_executable(lsw_audio_diagnostics_example examples/basic_usage.cpp)
    target_link_libraries(lsw_audio_diagnostics_example PRIVATE LSW::AudioDiagnostics)
    target_compile_features(lsw_audio_diagnostics_example PRIVATE cxx_std_17)
    set_target_properties(lsw_audio_diagnostics_example PROPERTIES CXX_EXTENSIONS OFF)
    lsw_audio_diag_set_compiler_warnings(lsw_audio_diagnostics_example ${LSW_AUDIO_DIAG_ENABLE_WARNINGS_AS_ERRORS})
endif()

install(TARGETS lsw_audio_diagnostics
    EXPORT LSWAudioDiagnosticsCoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT LSWAudioDiagnosticsCoreTargets
    FILE LSWAudioDiagnosticsCoreTargets.cmake
    NAMESPACE LSW::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LSWAudioDiagnosticsCore)

configure_package_config_file(
    cmake/LSWAudioDiagnosticsCoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LSWAudioDiagnosticsCoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LSWAudioDiagnosticsCore)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/LSWAudioDiagnosticsCoreConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LSWAudioDiagnosticsCoreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LSWAudioDiagnosticsCoreConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LSWAudioDiagnosticsCore)
