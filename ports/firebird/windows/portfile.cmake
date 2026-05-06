vcpkg_acquire_msys(MSYS_ROOT PACKAGES unzip sed grep awk coreutils)
vcpkg_add_to_path(${MSYS_ROOT}/usr/bin)

find_program(MSBUILD_EXE msbuild)
if (NOT MSBUILD_EXE)
    message(STATUS "DEBUG: msbuild not found in path")
else()
    get_filename_component(MSBUILD_DIR "${MSBUILD_EXE}" DIRECTORY)
    vcpkg_add_to_path("${MSBUILD_DIR}")
endif()

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


message(STATUS "DEBUG: FB_PROCESSOR_ARCHITECTURE=${FB_PROCESSOR_ARCHITECTURE}")
message(STATUS "DEBUG: FB_ARCH_OUT=${FB_ARCH_OUT}")
message(STATUS "DEBUG: MSBUILD_EXE=${MSBUILD_EXE}")

if (NOT EXISTS "${SOURCE_PATH}/builds/win32/run_all.bat")
    message(FATAL_ERROR "run_all.bat NOT FOUND at ${SOURCE_PATH}/builds/win32/run_all.bat")
endif()

# Release build

vcpkg_execute_build_process(
    COMMAND ${CMAKE_COMMAND} -E env "FB_PROCESSOR_ARCHITECTURE=${FB_PROCESSOR_ARCHITECTURE}"
        cmd /c run_all.bat
        ${FB_ARCH_OUT}
    WORKING_DIRECTORY "${SOURCE_PATH}/builds/win32"
    LOGNAME configure-${TARGET_TRIPLET}-rel
)

set(FB_RELEASE_INCLUDE_CANDIDATES
    "${SOURCE_PATH}/output_${FB_ARCH_OUT}_release/include"
    "${SOURCE_PATH}/output_${FB_ARCH_OUT}/include"
    "${SOURCE_PATH}/output_release/include"
    "${SOURCE_PATH}/output/include"
    "${SOURCE_PATH}/output_${VCPKG_TARGET_ARCHITECTURE}_release/include"
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
    message(STATUS "DEBUG: Listing top-level directories in SOURCE_PATH: ${SOURCE_PATH}")
    file(GLOB TOP_DIRS LIST_DIRECTORIES true "${SOURCE_PATH}/*")
    foreach(dir IN LISTS TOP_DIRS)
        message(STATUS "  ${dir}")
    endforeach()
    
    set(WIN_OUT_DIR "${SOURCE_PATH}/builds/win32")
    message(STATUS "DEBUG: Listing files in ${WIN_OUT_DIR}")
    file(GLOB WIN_FILES "${WIN_OUT_DIR}/*")
    foreach(file IN LISTS WIN_FILES)
        message(STATUS "  ${file}")
    endforeach()

    message(FATAL_ERROR "Firebird headers not found in expected locations. Check configure-${TARGET_TRIPLET}-rel-out.log for details.")
endif()

get_filename_component(FB_RELEASE_OUT_DIR "${FB_RELEASE_INCLUDE_DIR}" DIRECTORY)

file(
    INSTALL "${FB_RELEASE_INCLUDE_DIR}"
    DESTINATION "${CURRENT_PACKAGES_DIR}"
)

set(FB_RELEASE_LIB_CANDIDATES
    "${FB_RELEASE_OUT_DIR}/lib/fbclient_ms.lib"
    "${FB_RELEASE_OUT_DIR}/fbclient_ms.lib"
    "${SOURCE_PATH}/temp/${FB_ARCH_OUT}/release/yvalve/fbclient.lib"
    "${SOURCE_PATH}/temp/${FB_ARCH_OUT}/release/firebird/fbclient.lib"
)

set(FB_RELEASE_LIB_PATH "")
foreach(path IN LISTS FB_RELEASE_LIB_CANDIDATES)
    if(EXISTS "${path}" AND NOT IS_DIRECTORY "${path}")
        set(FB_RELEASE_LIB_PATH "${path}")
        break()
    endif()
endforeach()

if(FB_RELEASE_LIB_PATH STREQUAL "")
    # Broad recursive fallback: find fbclient.lib anywhere under temp/<arch>/
    file(GLOB_RECURSE FB_RELEASE_LIB_GLOB LIST_DIRECTORIES false
        "${SOURCE_PATH}/temp/${FB_ARCH_OUT}/*.lib"
    )
    foreach(lib IN LISTS FB_RELEASE_LIB_GLOB)
        get_filename_component(lib_name "${lib}" NAME)
        if(lib_name STREQUAL "fbclient.lib" OR lib_name STREQUAL "fbclient_ms.lib")
            set(FB_RELEASE_LIB_PATH "${lib}")
            break()
        endif()
    endforeach()
endif()

if(FB_RELEASE_LIB_PATH STREQUAL "")
    message(STATUS "DEBUG: Listing files in ${FB_RELEASE_OUT_DIR}/lib")
    file(GLOB LIB_FILES "${FB_RELEASE_OUT_DIR}/lib/*")
    foreach(f IN LISTS LIB_FILES)
        message(STATUS "  ${f}")
    endforeach()
    message(STATUS "DEBUG: Searching for fbclient*.lib under ${SOURCE_PATH}/temp/${FB_ARCH_OUT}")
    file(GLOB_RECURSE ALL_TEMP_LIBS LIST_DIRECTORIES false
        "${SOURCE_PATH}/temp/${FB_ARCH_OUT}/*.lib"
    )
    foreach(f IN LISTS ALL_TEMP_LIBS)
        get_filename_component(fn "${f}" NAME)
        if(fn MATCHES "^fbclient")
            message(STATUS "  ${f}")
        endif()
    endforeach()
    message(FATAL_ERROR "Firebird release client library (fbclient_ms.lib / fbclient.lib) not found. Check configure-${TARGET_TRIPLET}-rel-out.log for build details.")
endif()

file(
    INSTALL "${FB_RELEASE_LIB_PATH}"
    DESTINATION "${CURRENT_PACKAGES_DIR}/lib"
    RENAME "fbclient_ms.lib"
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
    COMMAND ${CMAKE_COMMAND} -E env "FB_PROCESSOR_ARCHITECTURE=${FB_PROCESSOR_ARCHITECTURE}"
        cmd /c run_all.bat
        DEBUG
        ${FB_ARCH_OUT}
    WORKING_DIRECTORY "${SOURCE_PATH}/builds/win32"
    LOGNAME configure-${TARGET_TRIPLET}-dbg
)

set(FB_DEBUG_LIB_CANDIDATES
    "${SOURCE_PATH}/output_${FB_ARCH_OUT}_debug/lib/fbclient_ms.lib"
    "${SOURCE_PATH}/output_${FB_ARCH_OUT}/lib/fbclient_ms.lib"
    "${SOURCE_PATH}/output_debug/lib/fbclient_ms.lib"
    "${SOURCE_PATH}/output/lib/fbclient_ms.lib"
    "${SOURCE_PATH}/output_${VCPKG_TARGET_ARCHITECTURE}_debug/lib/fbclient_ms.lib"
    "${SOURCE_PATH}/lib/fbclient_ms.lib"
    "${SOURCE_PATH}/output_${FB_ARCH_OUT}_debug/fbclient_ms.lib"
    "${SOURCE_PATH}/temp/${FB_ARCH_OUT}/debug/yvalve/fbclient.lib"
    "${SOURCE_PATH}/temp/${FB_ARCH_OUT}/debug/firebird/fbclient.lib"
)

set(FB_DEBUG_LIB_PATH "")
foreach(path IN LISTS FB_DEBUG_LIB_CANDIDATES)
    if(EXISTS "${path}" AND NOT IS_DIRECTORY "${path}")
        set(FB_DEBUG_LIB_PATH "${path}")
        break()
    endif()
endforeach()

if(FB_DEBUG_LIB_PATH STREQUAL "")
    # Broad recursive fallback: find fbclient.lib anywhere under temp/<arch>/
    file(GLOB_RECURSE FB_DEBUG_LIB_GLOB LIST_DIRECTORIES false
        "${SOURCE_PATH}/temp/${FB_ARCH_OUT}/*.lib"
    )
    foreach(lib IN LISTS FB_DEBUG_LIB_GLOB)
        get_filename_component(lib_name "${lib}" NAME)
        if(lib_name STREQUAL "fbclient.lib" OR lib_name STREQUAL "fbclient_ms.lib")
            set(FB_DEBUG_LIB_PATH "${lib}")
            break()
        endif()
    endforeach()
endif()

if(FB_DEBUG_LIB_PATH STREQUAL "")
    message(STATUS "DEBUG: Listing files in ${SOURCE_PATH}/output_${FB_ARCH_OUT}_debug/lib")
    file(GLOB DBG_LIB_FILES "${SOURCE_PATH}/output_${FB_ARCH_OUT}_debug/lib/*")
    foreach(f IN LISTS DBG_LIB_FILES)
        message(STATUS "  ${f}")
    endforeach()
    message(STATUS "DEBUG: Searching for fbclient*.lib under ${SOURCE_PATH}/temp/${FB_ARCH_OUT}")
    file(GLOB_RECURSE ALL_DBG_LIBS LIST_DIRECTORIES false
        "${SOURCE_PATH}/temp/${FB_ARCH_OUT}/*.lib"
    )
    foreach(f IN LISTS ALL_DBG_LIBS)
        get_filename_component(fn "${f}" NAME)
        if(fn MATCHES "^fbclient")
            message(STATUS "  ${f}")
        endif()
    endforeach()
    message(FATAL_ERROR "Firebird debug client library (fbclient_ms.lib / fbclient.lib) not found. Check configure-${TARGET_TRIPLET}-dbg-out.log for build details.")
endif()

# Get the base debug output directory (parent of 'lib' or 'output_..._debug')
get_filename_component(FB_DEBUG_LIB_DIR "${FB_DEBUG_LIB_PATH}" DIRECTORY)
get_filename_component(FB_DEBUG_OUT_DIR "${FB_DEBUG_LIB_DIR}" DIRECTORY)

file(
    INSTALL "${FB_DEBUG_LIB_PATH}"
    DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib"
    RENAME "fbclient_ms.lib"
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
