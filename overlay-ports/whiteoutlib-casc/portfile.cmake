vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FernandoS27/WhiteoutLib
    REF 92887f9156cdaca8966b3b58e2625628aa0c2c30
    SHA512 573e7530441f88234f70bfb016995a4057e42c970dca20dcc9e448b81199d8ffb8fcb725f886c28bcbd060e1439ab0da488a703b5b0212661ed3826dcef6cd47
    HEAD_REF main
)

# Upstream's build compiles the whole library (models, textures, ...) and fetches zlib-ng,
# so only the CASC reader is built, with this port's own CMakeLists.txt
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(CONFIG_PATH share/whiteoutlib-casc)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE" "${SOURCE_PATH}/LICENSE-AI.md")
