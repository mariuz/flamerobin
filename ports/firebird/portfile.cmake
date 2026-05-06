set(VCPKG_POLICY_ALLOW_DEBUG_SHARE enabled)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FirebirdSQL/firebird
    REF v5.0.4
    SHA512 470e371733817b20d84a4dc45a23ca54fb82e150efbec17b3cd3e71d1867023c30dd0aaa83c807f4e03cf0403cd47eeb0b7df2a09a3be897a0f6dddc0bbeeb6a
    HEAD_REF master
    PATCHES
        windows-paths.diff
        windows-timeout.patch
        osx-icu-rpath.patch
        osx-unvcpkg.patch
)

if(VCPKG_TARGET_IS_WINDOWS)
    include("${CMAKE_CURRENT_LIST_DIR}/windows/portfile.cmake")
else()
    include("${CMAKE_CURRENT_LIST_DIR}/posix/portfile.cmake")
endif()

# Copy common files

file(
    INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
)

vcpkg_install_copyright(
    FILE_LIST
        "${SOURCE_PATH}/builds/install/misc/IPLicense.txt"
        "${SOURCE_PATH}/builds/install/misc/IDPLicense.txt"
)

file(
    INSTALL "${CMAKE_CURRENT_LIST_DIR}/${PORT}-config.cmake"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
)
