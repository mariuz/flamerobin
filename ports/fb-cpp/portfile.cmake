vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO asfernandes/fb-cpp
    REF a73eda3
    SHA512 c47cdcc9bd589bec3f868c804930bf5daa849abab2aa1813df003ffad7c974620b8dc756d6168bc2a2abbd12e660e0ae6c60b1f3cdb163b643f1fc69731f1238
    PATCHES
        fb-cpp-flamerobin.patch
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        boost-dll           FB_CPP_USE_BOOST_DLL
        boost-multiprecision FB_CPP_USE_BOOST_MULTIPRECISION
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DFB_CPP_BUILD_TESTS=OFF
        -DFB_CPP_SKIP_INSTALL=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME fb-cpp CONFIG_PATH lib/cmake/fb-cpp)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")
