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
# #include "../firebird/include/ibase.h"
# If we add ${CMAKE_BINARY_DIR}/fb_hack to include path, and have:
# ${CMAKE_BINARY_DIR}/fb_hack/firebird/include/ibase.h
# Then relative to ${CMAKE_SOURCE_DIR}/src/firebird/include/firebird/Interface.h it won't work...
# Wait, the include is relative to the file.
# So we REALLY need it relative to Interface.h.

# Okay, let's try to add ${CMAKE_SOURCE_DIR}/src/firebird/include/firebird/include as a symlink
# but do it during configuration.
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/src/firebird/include/firebird/include")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "../../ibase.h" "${CMAKE_SOURCE_DIR}/src/firebird/include/firebird/include/ibase.h")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "../../iberror.h" "${CMAKE_SOURCE_DIR}/src/firebird/include/firebird/include/iberror.h")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "../../firebird" "${CMAKE_SOURCE_DIR}/src/firebird/include/firebird/include/firebird")

target_include_directories(firebird INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/firebird/include>)

install(TARGETS firebird EXPORT fb-cppTargets)
