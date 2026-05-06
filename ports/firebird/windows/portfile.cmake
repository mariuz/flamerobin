vcpkg_acquire_msys(MSYS_ROOT PACKAGES unzip)
vcpkg_add_to_path(${MSYS_ROOT}/usr/bin)

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(FB_ARCH_OUT "Win32")
    set(FB_PROCESSOR_ARCHITECTURE "x86")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(FB_ARCH_OUT "x64")
    set(FB_PROCESSOR_ARCHITECTURE "AMD64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(FB_ARCH_OUT "arm64")
    set(FB_PROCESSOR_ARCHITECTURE "ARM64")
endif()


# Release build

vcpkg_execute_build_process(
    COMMAND ${CMAKE_COMMAND} -E env
        "FB_PROCESSOR_ARCHITECTURE=${FB_PROCESSOR_ARCHITECTURE}"
        run_all.bat
        JUSTBUILD
        RELEASE
        CLIENT_ONLY
    WORKING_DIRECTORY "${SOURCE_PATH}/builds/win32"
    LOGNAME configure-${TARGET_TRIPLET}-rel
)

set(FB_RELEASE_INCLUDE_CANDIDATES
    "${SOURCE_PATH}/output_${FB_ARCH_OUT}_release/include"
    "${SOURCE_PATH}/output_release/include"
    "${SOURCE_PATH}/include"
)

set(FB_RELEASE_INCLUDE_DIR "")
foreach(dir IN LISTS FB_RELEASE_INCLUDE_CANDIDATES)
    if(EXISTS "${dir}")
        set(FB_RELEASE_INCLUDE_DIR "${dir}")
        break()
    endif()
endforeach()

if(FB_RELEASE_INCLUDE_DIR STREQUAL "")
    message(FATAL_ERROR "Firebird headers not found in expected locations")
endif()

get_filename_component(FB_RELEASE_OUT_DIR "${FB_RELEASE_INCLUDE_DIR}" DIRECTORY)

file(
    INSTALL "${FB_RELEASE_INCLUDE_DIR}"
    DESTINATION "${CURRENT_PACKAGES_DIR}"
)

file(
    INSTALL "${FB_RELEASE_OUT_DIR}/lib/fbclient_ms.lib"
    DESTINATION "${CURRENT_PACKAGES_DIR}/lib"
)

file(
    INSTALL "${FB_RELEASE_OUT_DIR}/fbclient.dll"
    DESTINATION "${CURRENT_PACKAGES_DIR}/bin"
)

file(
    INSTALL "${SOURCE_PATH}/temp/${FB_ARCH_OUT}/release/yvalve/fbclient.pdb"
    DESTINATION "${CURRENT_PACKAGES_DIR}/bin"
)

file(GLOB PLUGINS_FILES_RELEASE
    "${FB_RELEASE_OUT_DIR}/plugins/*"
)
file(
    INSTALL ${PLUGINS_FILES_RELEASE}
    DESTINATION "${CURRENT_PACKAGES_DIR}/plugins/${PORT}"
)

file(
    INSTALL "${FB_RELEASE_OUT_DIR}/firebird.msg"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
)

file(
    INSTALL "${FB_RELEASE_OUT_DIR}/tzdata"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
)


# Debug build

vcpkg_execute_build_process(
    COMMAND ${CMAKE_COMMAND} -E env
        "FB_PROCESSOR_ARCHITECTURE=${FB_PROCESSOR_ARCHITECTURE}"
        run_all.bat
        JUSTBUILD
        DEBUG
        CLIENT_ONLY
    WORKING_DIRECTORY "${SOURCE_PATH}/builds/win32"
    LOGNAME configure-${TARGET_TRIPLET}-dbg
)

set(FB_DEBUG_LIB_CANDIDATES
    "${SOURCE_PATH}/output_${FB_ARCH_OUT}_debug/lib/fbclient_ms.lib"
    "${SOURCE_PATH}/output_debug/lib/fbclient_ms.lib"
    "${SOURCE_PATH}/lib/fbclient_ms.lib"
)

set(FB_DEBUG_LIB_PATH "")
foreach(path IN LISTS FB_DEBUG_LIB_CANDIDATES)
    if(EXISTS "${path}")
        set(FB_DEBUG_LIB_PATH "${path}")
        break()
    endif()
endforeach()

if(FB_DEBUG_LIB_PATH STREQUAL "")
    message(FATAL_ERROR "Firebird debug library not found in expected locations")
endif()

# Get the base debug output directory (parent of 'lib' or 'output_..._debug')
get_filename_component(FB_DEBUG_LIB_DIR "${FB_DEBUG_LIB_PATH}" DIRECTORY)
get_filename_component(FB_DEBUG_OUT_DIR "${FB_DEBUG_LIB_DIR}" DIRECTORY)

file(
    INSTALL "${FB_DEBUG_LIB_PATH}"
    DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib"
)

file(
    INSTALL "${FB_DEBUG_OUT_DIR}/fbclient.dll"
    DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin"
)

file(
    INSTALL "${SOURCE_PATH}/temp/${FB_ARCH_OUT}/debug/yvalve/fbclient.pdb"
    DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin"
)

file(GLOB PLUGINS_FILES_DEBUG
    "${FB_DEBUG_OUT_DIR}/plugins/*"
)
file(
    INSTALL ${PLUGINS_FILES_DEBUG}
    DESTINATION "${CURRENT_PACKAGES_DIR}/debug/plugins/${PORT}"
)

file(
    INSTALL "${FB_DEBUG_OUT_DIR}/firebird.msg"
    DESTINATION "${CURRENT_PACKAGES_DIR}/debug/share/${PORT}"
)

file(
    INSTALL "${FB_DEBUG_OUT_DIR}/tzdata"
    DESTINATION "${CURRENT_PACKAGES_DIR}/debug/share/${PORT}"
)
