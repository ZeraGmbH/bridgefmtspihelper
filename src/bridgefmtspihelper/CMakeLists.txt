#sum up project Files 
file(GLOB SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    )

file(GLOB HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QBridgeFmtSpiHelper
    )

#create library 
add_library(bridgefmtspihelper SHARED
    ${SOURCES}
    ${HEADER}
    )

#set target Version
set_target_properties(bridgefmtspihelper PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(bridgefmtspihelper PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

target_link_libraries(bridgefmtspihelper 
    PRIVATE
    Qt5::Core
    )

# announce headers - target perspective
target_include_directories(bridgefmtspihelper
    PUBLIC
    $<INSTALL_INTERFACE:include/bridgefmtspihelper>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

#generate export header
include(GenerateExportHeader)
generate_export_header(bridgefmtspihelper)

#install export header
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/bridgefmtspihelper_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bridgefmtspihelper
    )

# install library
install(TARGETS bridgefmtspihelper
    EXPORT bridgefmtspihelperExport
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

# configure *ConfigVersion.cmake
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    bridgefmtspihelperConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMinorVersion
    )

# configure *Config.cmake
configure_file(bridgefmtspihelperConfig.cmake.in bridgefmtspihelperConfig.cmake @ONLY)

# install *Config(Version).cmake
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/bridgefmtspihelperConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/bridgefmtspihelperConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bridgefmtspihelper
    )

# install targets cmake-files
install(EXPORT bridgefmtspihelperExport
    FILE bridgefmtspihelperTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bridgefmtspihelper
    )

# install public headers
install(
    FILES ${HEADER}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bridgefmtspihelper
    )
    
