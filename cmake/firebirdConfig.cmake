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

# Create a directory structure that satisfies the weird include in Interface.h:
# We fixed Interface.h to use a proper relative path, so we don't need the symlinks anymore.

target_include_directories(firebird INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../src/firebird/include>
    $<INSTALL_INTERFACE:include>
    /app/include
)

install(TARGETS firebird EXPORT fb-cppTargets)
