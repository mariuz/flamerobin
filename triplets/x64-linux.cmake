set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(X_VCPKG_FORCE_VCPKG_WAYLAND_LIBRARIES ON)

if(PORT MATCHES "^(icu|wxwidgets|gtk3|glib|cairo|pango|atk|harfbuzz|gdk-pixbuf|epoxy|dbus|sdl2)$")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
