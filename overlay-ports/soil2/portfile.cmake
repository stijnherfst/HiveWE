vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
vcpkg_minimum_required(VERSION 2022-11-10)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO stijnherfst/SOIL2
    REF 32c62e15b61f599ea33d89ec904b618d217a1979
    SHA512 2d4df42f0f0412bd8c12d9b046be0cc2ebbe18af51096eec78c2c99e07e28ebd3fbab7ffe8d68095f91fdb2b9bcf7fb2fe42da2db0c761bc0cb9d21a22601362
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS_DEBUG
        -DSOIL2_SKIP_HEADERS=ON
)

vcpkg_cmake_install()

vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")