add_library(firebird INTERFACE)
add_library(firebird::firebird ALIAS firebird)

# On Linux/Unix, FlameRobin uses -lfbclient -ldl
if (UNIX AND NOT APPLE)
    target_link_libraries(firebird INTERFACE fbclient dl)
elseif (APPLE)
    # macOS logic from main CMakeLists.txt
    target_link_libraries(firebird INTERFACE ${FBCLIENT_LIBRARY})
elseif (WIN32)
    target_link_libraries(firebird INTERFACE fbclient)
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

install(TARGETS firebird EXPORT fb-cppTargets)
