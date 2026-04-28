add_library(firebird INTERFACE)
add_library(firebird::firebird ALIAS firebird)

# On Linux/Unix, FlameRobin uses -lfbclient -ldl
if (UNIX AND NOT APPLE)
    target_link_libraries(firebird INTERFACE fbclient dl)
elseif (APPLE)
    # macOS logic from main CMakeLists.txt
    target_link_libraries(firebird INTERFACE ${FBCLIENT_LIBRARY})
elseif (WIN32)
    # Hint for vcpkg
    set(FB_VCPKG_HINTS "")
    if (VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
        list(APPEND FB_VCPKG_HINTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
    endif()
    list(APPEND FB_VCPKG_HINTS 
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/lib"
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/x86-windows/lib"
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows-static/lib"
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/x86-windows-static/lib"
    )

    find_library(FIREBIRD_LIBRARY 
        NAMES fbclient fbclient_ms gds32
        HINTS 
            ${FB_VCPKG_HINTS}
            $ENV{FIREBIRD}/lib
            $ENV{FIREBIRD_SDK}/lib
    )
    if (FIREBIRD_LIBRARY)
        message(STATUS "Found Firebird client library: ${FIREBIRD_LIBRARY}")
        target_link_libraries(firebird INTERFACE "${FIREBIRD_LIBRARY}")
    else ()
        # Fallback to just name if find_library fails
        message(WARNING "Could not find Firebird client library. Linker might fail if it's not in the system path.")
        target_link_libraries(firebird INTERFACE fbclient)
    endif()
endif()

# Firebird headers
find_path(FIREBIRD_INCLUDE_DIR 
    NAMES ibase.h firebird/Interface.h
    HINTS 
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/include"
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/x86-windows/include"
        "$ENV{FIREBIRD}/include"
        "$ENV{FIREBIRD_SDK}/include"
        "/app/include"
        "${CMAKE_CURRENT_LIST_DIR}/../src/firebird/include"
)

if (FIREBIRD_INCLUDE_DIR)
    message(STATUS "Found Firebird headers: ${FIREBIRD_INCLUDE_DIR}")
    target_include_directories(firebird INTERFACE 
        $<BUILD_INTERFACE:${FIREBIRD_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
    )
else()
    # Fallback for older behavior or bundled headers
    set(FIREBIRD_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../src/firebird/include")
    target_include_directories(firebird INTERFACE 
        $<BUILD_INTERFACE:${FIREBIRD_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include>
    )
endif()

install(TARGETS firebird EXPORT fb-cppTargets)
