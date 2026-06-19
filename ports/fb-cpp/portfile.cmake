vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO asfernandes/fb-cpp
    REF 85227271e5d6581bdf834efb47cf900ce06ec0f7
    SHA512 800068522e1349031ce009fd34ad75b7cb76ea648d7182d8f5eb1d17128a5bd1c4587ce9b0cff50dc9c84ca0e69dc6ebc873ff340f0d3f0a08ecc6a2ede3f31f
    PATCHES
        fb-cpp-flamerobin.patch
)

# Force STATIC library build on Windows since fb-cpp has no exports for a DLL build
if(VCPKG_TARGET_IS_WINDOWS)
    file(READ "${SOURCE_PATH}/src/fb-cpp/CMakeLists.txt" CONTENTS)
    string(REPLACE "add_library(\${PROJECT_NAME}" "add_library(\${PROJECT_NAME} STATIC" CONTENTS "${CONTENTS}")
    file(WRITE "${SOURCE_PATH}/src/fb-cpp/CMakeLists.txt" "${CONTENTS}")
endif()

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        boost-dll           FB_CPP_USE_BOOST_DLL
        boost-multiprecision FB_CPP_USE_BOOST_MULTIPRECISION
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DBUILD_TESTING=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME fb-cpp CONFIG_PATH fb-cpp/cmake/fb-cpp)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")
