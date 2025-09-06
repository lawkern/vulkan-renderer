.POSIX:
CFLAGS = -g3 -std=c99 -DDEBUG -D_DEFAULT_SOURCE $(WARNINGS)
LDLIBS = -lm
WARNINGS = -Wall -Wextra -Werror\
-Wno-unused-function\
-Wno-unused-variable\
-Wno-unused-parameter\
-Wno-unused-but-set-variable\
-Wno-missing-field-initializers\

# NOTE: For Wayland code generation:
WL_PROTOCOLS  = $$(pkg-config wayland-protocols --variable=pkgdatadir)
WL_CLIENT     = $$(pkg-config wayland-client --cflags --libs)

compile: shaders wayland
# compile: shaders xlib
# compile: shaders win32

shaders:
	mkdir -p build
	glslc src/shaders/basic.vert -o build/basic_vertex.spv
	glslc src/shaders/basic.frag -o build/basic_fragment.spv

wayland:
	mkdir -p src/external
	eval wayland-scanner client-header < $(WL_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml > src/external/xdg-shell-client-protocol.h
	eval wayland-scanner private-code  < $(WL_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml > src/external/xdg-shell-protocol.c
	eval wayland-scanner client-header < $(WL_PROTOCOLS)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml > src/external/xdg-decoration-unstable-v1-client-protocol.h
	eval wayland-scanner private-code  < $(WL_PROTOCOLS)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml > src/external/xdg-decoration-unstable-v1-protocol.c

	eval $(CC) -o build/vulkan_renderer src/main_wayland.c $(CFLAGS) $(LDLIBS) -lvulkan $(WL_CLIENT)

xlib:
	eval $(CC) -o build/vulkan_renderer src/main_xlib.c $(CFLAGS) $(LDLIBS) -lvulkan -lX11

win32:
	eval $(CC) -o build/vulkan_renderer src/main_win32.c $(CFLAGS) $(LDLIBS) -lvulkan-1 -lgdi32

run:
	cd build && ./vulkan_renderer

debug:
	cd build && gdb vulkan_renderer
