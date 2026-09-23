vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO asfernandes/fb-cpp
    REF v1.0.0
    SHA512 3955dcbee777d9e97618fe82cfbfc495671a1abf28573c7232aba9bb25a454ff2755cd5e8d2ad19bead105728b2e1dcf36f9bb0a6a317b3fddc1160e746988ca
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
