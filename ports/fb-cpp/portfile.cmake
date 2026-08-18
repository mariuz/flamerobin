vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO asfernandes/fb-cpp
    REF v0.0.6
    SHA512 b414ecea83b3874aa39ddbdaded5f3357ce97b5855a4adcb9c6afbad2ecc8421aadb64ca7506799ecbddeba411e7828e247c35d29769c256dc1d70c0393a6f42
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
