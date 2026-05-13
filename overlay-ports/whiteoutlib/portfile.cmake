vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FernandoS27/WhiteoutLib
    REF e0b415eaa066b30d801189a68a437529e0e63373
    SHA512 f8c509b509b8e62eb48b9557610f50ab4128fdff141edd64662f42b833a4fb319dea5a45fa9077809c2d1225d4608123f922e6cb1127302317fa880eb156dbdf
    HEAD_REF master
)

# Upstream CMakeLists.txt has no install() rules. Append our own so vcpkg can
# package the artifacts and downstream consumers can `find_package(Whiteoutlib)`.
file(APPEND "${SOURCE_PATH}/CMakeLists.txt" [=[

# === vcpkg install rules (appended by HiveWE overlay port) ===
include(GNUInstallDirs)

# Upstream sets INTERFACE_INCLUDE_DIRECTORIES to an absolute source path, which
# install(EXPORT) refuses. Rewrite it with BUILD_INTERFACE / INSTALL_INTERFACE
# generator expressions so the export works.
set(_whiteout_install_targets whiteout_strict_flags whiteout_lib)
if(TARGET whiteout_casc)
    list(APPEND _whiteout_install_targets whiteout_casc)
endif()
if(TARGET whiteout_mpq)
    list(APPEND _whiteout_install_targets whiteout_mpq)
endif()

foreach(_t IN LISTS _whiteout_install_targets)
    get_target_property(_t_type ${_t} TYPE)
    if(_t_type STREQUAL "INTERFACE_LIBRARY")
        continue()
    endif()
    set_property(TARGET ${_t} PROPERTY INTERFACE_INCLUDE_DIRECTORIES
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endforeach()

install(TARGETS ${_whiteout_install_targets}
    EXPORT WhiteoutlibTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(EXPORT WhiteoutlibTargets
    NAMESPACE Whiteoutlib::
    DESTINATION "${CMAKE_INSTALL_DATADIR}/Whiteoutlib"
    FILE WhiteoutlibTargets.cmake
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Whiteoutlib-config.cmake"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/WhiteoutlibTargets.cmake\")\n"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Whiteoutlib-config.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/Whiteoutlib"
)
]=])

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DWHITEOUT_BUILD_EXAMPLES=OFF
        -DWHITEOUT_BUILD_TESTS=OFF
        -DWHITEOUT_WARNINGS_AS_ERRORS=OFF
        -DWHITEOUT_ENABLE_CASC=ON
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME Whiteoutlib)
vcpkg_copy_pdbs()

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
