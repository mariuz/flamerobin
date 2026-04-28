# Check if firebird package is already provided by the environment (e.g. vcpkg)
find_package(firebird CONFIG QUIET)

if (firebird_FOUND)
    if (NOT TARGET firebird)
        add_library(firebird INTERFACE)
        target_link_libraries(firebird INTERFACE firebird::firebird)
    endif()
    if (NOT TARGET firebird::firebird)
        add_library(firebird::firebird ALIAS firebird)
    endif()
else()
    if (NOT TARGET firebird)
        add_library(firebird INTERFACE)
    endif()
    if (NOT TARGET firebird::firebird)
        add_library(firebird::firebird ALIAS firebird)
    endif()

    # On Linux/Unix, FlameRobin uses -lfbclient -ldl
    if (UNIX AND NOT APPLE)
        target_link_libraries(firebird INTERFACE fbclient dl)
    elseif (APPLE)
        # macOS logic from main CMakeLists.txt
        target_link_libraries(firebird INTERFACE ${FBCLIENT_LIBRARY})
    elseif (WIN32)
        find_library(FIREBIRD_LIBRARY fbclient)
        if (FIREBIRD_LIBRARY)
            message(STATUS "Found Firebird client library: ${FIREBIRD_LIBRARY}")
            target_link_libraries(firebird INTERFACE ${FIREBIRD_LIBRARY})
        else ()
            # Fallback to just name if find_library fails
            target_link_libraries(firebird INTERFACE fbclient)
        endif()
    endif()
endif()

# Firebird headers
if(EXISTS "/app/include/ibase.h")
    set(FIREBIRD_INCLUDE_DIRS "/app/include")
else()
    set(FIREBIRD_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../src/firebird/include")
endif()

# We MUST wrap source-prefixed paths in BUILD_INTERFACE to avoid CMake errors 
# when the target is used by other projects (like fb-cpp).
target_include_directories(firebird INTERFACE 
    $<BUILD_INTERFACE:${FIREBIRD_INCLUDE_DIRS}>
    $<INSTALL_INTERFACE:include>
)

# For Flatpak environment where paths are outside source/binary tree
if(FIREBIRD_INCLUDE_DIRS STREQUAL "/app/include")
    target_include_directories(firebird INTERFACE "/app/include")
endif()

if (NOT FB_CPP_SKIP_INSTALL)
    install(TARGETS firebird EXPORT fb-cppTargets)
endif()
