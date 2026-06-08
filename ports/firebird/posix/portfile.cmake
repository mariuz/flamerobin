	# In Linux, we adjust rpaths to be more precise than what vcpkg does by default.
set(VCPKG_FIXUP_ELF_RPATH OFF)
#set(VCPKG_FIXUP_MACHO_RPATH OFF)

if(NOT VCPKG_TARGET_IS_OSX)
vcpkg_find_acquire_program(PATCHELF)
endif()

if(VCPKG_TARGET_IS_OSX)
    set(ENV{LIBTOOLIZE} glibtoolize)
    set(ENV{LIBTOOL} glibtool)

    function(firebird_ensure_lc_rpath target_lib)
        execute_process(
            COMMAND otool -l "${target_lib}"
            OUTPUT_VARIABLE _otool_output
            RESULT_VARIABLE _otool_result
        )
        if(NOT _otool_result EQUAL 0)
            message(FATAL_ERROR "Failed to inspect ${target_lib} with otool.")
        endif()

        string(FIND "${_otool_output}" "path @loader_path/.. " _has_loader_parent_rpath)
        if(_has_loader_parent_rpath EQUAL -1)
            execute_process(
                COMMAND install_name_tool -add_rpath "@loader_path/.." "${target_lib}"
                RESULT_VARIABLE _add_rpath_result
            )
            if(NOT _add_rpath_result EQUAL 0)
                message(FATAL_ERROR "Failed adding LC_RPATH @loader_path/.. to ${target_lib}.")
            endif()
        endif()
    endfunction()
endif()

set(FIREBIRD_CONFIGURE_OPTIONS
    --enable-client-only
    --enable-binreloc
    --with-termlib=:libncurses.a
    --with-atomiclib=:libatomic.a
    --with-fbplugins=plugins/${PORT}
    --with-fbmsg=share/${PORT}
    --with-fbtzdata=share/${PORT}/tzdata
    "CPPFLAGS=-I${CURRENT_HOST_INSTALLED_DIR}/include"
    "CXXFLAGS=-I${CURRENT_HOST_INSTALLED_DIR}/include"
    "LDFLAGS=-L${CURRENT_HOST_INSTALLED_DIR}/lib"
)

vcpkg_configure_make(
    SOURCE_PATH "${SOURCE_PATH}"
    COPY_SOURCE
    AUTOCONFIG
    OPTIONS
        ${FIREBIRD_CONFIGURE_OPTIONS}
    OPTIONS_DEBUG
        --enable-developer
)

vcpkg_build_make()


# Release build

if(NOT VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    set(SOURCE_COPY_REL_PATH "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel")

    file(
        INSTALL "${SOURCE_COPY_REL_PATH}/gen/Release/firebird/include"
        DESTINATION "${CURRENT_PACKAGES_DIR}"
    )

    file(GLOB FIREBIRD_RELEASE_LIBS
        "${SOURCE_COPY_REL_PATH}/gen/Release/firebird/lib/libfbclient*"
    )

    file(
        INSTALL ${FIREBIRD_RELEASE_LIBS}
        DESTINATION "${CURRENT_PACKAGES_DIR}/lib"
        USE_SOURCE_PERMISSIONS
    )

    if(VCPKG_TARGET_IS_OSX)
        file(GLOB FIREBIRD_RELEASE_INSTALLED_DYLIBS
            "${CURRENT_PACKAGES_DIR}/lib/libfbclient*.dylib"
        )
        foreach(lib ${FIREBIRD_RELEASE_INSTALLED_DYLIBS})
            if(NOT IS_SYMLINK "${lib}")
                firebird_ensure_lc_rpath("${lib}")
            endif()
        endforeach()
    endif()

    file(GLOB PLUGINS_FILES_RELEASE
        "${SOURCE_COPY_REL_PATH}/gen/Release/firebird/plugins/*"
    )

    if(NOT VCPKG_TARGET_IS_OSX)
        foreach(plugin ${PLUGINS_FILES_RELEASE})
            execute_process(
                COMMAND "${PATCHELF}" --set-rpath "$ORIGIN/../../lib" ${plugin}
            )
        endforeach()
    endif()

    file(
        INSTALL ${PLUGINS_FILES_RELEASE}
        DESTINATION "${CURRENT_PACKAGES_DIR}/plugins/${PORT}"
        USE_SOURCE_PERMISSIONS
    )

    file(
        INSTALL "${SOURCE_COPY_REL_PATH}/gen/Release/firebird/firebird.msg"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    )

    file(
        INSTALL "${SOURCE_COPY_REL_PATH}/gen/Release/firebird/tzdata"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    )
endif()


# Debug build

if(NOT VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    set(SOURCE_COPY_DBG_PATH "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg")

    file(GLOB FIREBIRD_DEBUG_LIBS
        "${SOURCE_COPY_DBG_PATH}/gen/Debug/firebird/lib/libfbclient*"
    )

    file(
        INSTALL ${FIREBIRD_DEBUG_LIBS}
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib"
        USE_SOURCE_PERMISSIONS
    )

    if(VCPKG_TARGET_IS_OSX)
        file(GLOB FIREBIRD_DEBUG_INSTALLED_DYLIBS
            "${CURRENT_PACKAGES_DIR}/debug/lib/libfbclient*.dylib"
        )
        foreach(lib ${FIREBIRD_DEBUG_INSTALLED_DYLIBS})
            if(NOT IS_SYMLINK "${lib}")
                firebird_ensure_lc_rpath("${lib}")
            endif()
        endforeach()
    endif()

    file(GLOB PLUGINS_FILES_DEBUG
        "${SOURCE_COPY_DBG_PATH}/gen/Debug/firebird/plugins/*"
    )

    if(NOT VCPKG_TARGET_IS_OSX)
        foreach(plugin ${PLUGINS_FILES_DEBUG})
            execute_process(
                COMMAND "${PATCHELF}" --set-rpath "$ORIGIN/../../lib" ${plugin}
            )
        endforeach()
    endif()

    file(
        INSTALL ${PLUGINS_FILES_DEBUG}
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/plugins/${PORT}"
        USE_SOURCE_PERMISSIONS
    )

    file(
        INSTALL "${SOURCE_COPY_DBG_PATH}/gen/Debug/firebird/firebird.msg"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/share/${PORT}"
    )

    file(
        INSTALL "${SOURCE_COPY_DBG_PATH}/gen/Debug/firebird/tzdata"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/share/${PORT}"
    )
endif()
