vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO asfernandes/fb-cpp
    REF c318a4abc7e3cf259ed95c184b33a3f5c3a01655
    SHA512 d8c56d7c98069f5b99c2de1eef0f40c0251e8a5880b5757bb7a4e146dcbc6d10aff0604d99b1eb9e84d8a635a4f9d8903244e50c09540d2dc2dae5981a6707e2
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
