vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO asfernandes/fb-cpp
    REF b5fec85cc72008518e9686848938db2401231bc1
    SHA512 d888ba5f5611c052f4a866d93a52ed7e9db33a8339fa42581c05c3ff91e29646975455fd90eb2ce8888d081e10492f09e69782b17029ec846e03d1abb6dbc18a
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
