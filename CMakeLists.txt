cmake_minimum_required(VERSION 3.21)
project(QuirewisePipeline VERSION 0.1.0 LANGUAGES C)
option(QUIREWISE_BUILD_SHARED "Build Quirewise libraries as shared libraries" OFF)
if(QUIREWISE_BUILD_SHARED)
    set(QW_LIBRARY_TYPE SHARED)
else()
    set(QW_LIBRARY_TYPE STATIC)
endif()
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
if(NOT TARGET Quirewise::Runtime)
    find_package(QuirewiseRuntime CONFIG REQUIRED)
endif()
if(NOT TARGET Quirewise::Image)
    find_package(QuirewiseImage CONFIG REQUIRED)
endif()
if(NOT TARGET Quirewise::Page)
    find_package(QuirewisePage CONFIG REQUIRED)
endif()
if(NOT TARGET Quirewise::PDG)
    find_package(QuirewisePDG CONFIG REQUIRED)
endif()
if(NOT TARGET Quirewise::Document)
    find_package(QuirewiseDocument CONFIG REQUIRED)
endif()
if(NOT TARGET Quirewise::Data)
    find_package(QuirewiseData CONFIG REQUIRED)
endif()
if(NOT TARGET Quirewise::Comic)
    find_package(QuirewiseComic CONFIG REQUIRED)
endif()
if(NOT TARGET Quirewise::Subtitle)
    find_package(QuirewiseSubtitle CONFIG REQUIRED)
endif()
if(NOT TARGET Quirewise::Vector)
    find_package(QuirewiseVector CONFIG REQUIRED)
endif()
if(NOT TARGET Quirewise::Audio)
    find_package(QuirewiseAudio CONFIG REQUIRED)
endif()

file(GLOB_RECURSE QW_SOURCES CONFIGURE_DEPENDS src/*.c src/*.cpp)
add_library(quirewise_pipeline ${QW_LIBRARY_TYPE} ${QW_SOURCES})
add_library(Quirewise::Pipeline ALIAS quirewise_pipeline)
set_target_properties(quirewise_pipeline PROPERTIES
    EXPORT_NAME Pipeline
    POSITION_INDEPENDENT_CODE ON)
target_compile_features(quirewise_pipeline PUBLIC c_std_11)
target_include_directories(quirewise_pipeline PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/routes)
target_link_libraries(quirewise_pipeline PUBLIC
    Quirewise::Runtime
    Quirewise::Image
    Quirewise::Page
    Quirewise::PDG
    Quirewise::Document
    Quirewise::Data
    Quirewise::Comic
    Quirewise::Subtitle
    Quirewise::Vector
    Quirewise::Audio)
install(TARGETS quirewise_pipeline EXPORT QuirewisePipelineTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(EXPORT QuirewisePipelineTargets
    FILE QuirewisePipelineTargets.cmake
    NAMESPACE Quirewise::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QuirewisePipeline")
configure_package_config_file(
    cmake/QuirewisePipelineConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/QuirewisePipelineConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QuirewisePipeline")
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QuirewisePipelineConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QuirewisePipelineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QuirewisePipelineConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QuirewisePipeline")

if(QUIREWISE_BUILD_SHARED AND WIN32)
    set_target_properties(quirewise_pipeline PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(quirewise_pipeline_test tests/pipeline_test.c)
    target_compile_features(quirewise_pipeline_test PRIVATE c_std_11)
    target_link_libraries(quirewise_pipeline_test PRIVATE Quirewise::Pipeline)
    add_test(NAME quirewise_pipeline_routes COMMAND quirewise_pipeline_test)
endif()
