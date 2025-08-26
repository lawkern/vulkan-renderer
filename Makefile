CFLAGS = -g3 -Wall -Wextra -DDEBUG -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-missing-field-initializers
LDLIBS = -lvulkan -lm

# NOTE: For Wayland code generation:
WL_SCANNER    = $$(pkg-config wayland-scanner --variable=wayland_scanner)
WL_PROTOCOLS  = $$(pkg-config wayland-protocols --variable=pkgdatadir)
WL_CLIENT     = $$(pkg-config wayland-client --cflags --libs)
WL_SHELL      = stable/xdg-shell/xdg-shell.xml
WL_DECORATION = unstable/xdg-decoration/xdg-decoration-unstable-v1.xml

wayland:
	mkdir -p src/external
	wayland-scanner client-header < $(WL_PROTOCOLS)/$(WL_SHELL) > src/external/xdg-shell-client-protocol.h
	wayland-scanner private-code  < $(WL_PROTOCOLS)/$(WL_SHELL) > src/external/xdg-shell-protocol.c
	wayland-scanner client-header < $(WL_PROTOCOLS)/$(WL_DECORATION) > src/external/xdg-decoration-unstable-v1-client-protocol.h
	wayland-scanner private-code  < $(WL_PROTOCOLS)/$(WL_DECORATION) > src/external/xdg-decoration-unstable-v1-protocol.c

	mkdir -p build
	glslc shaders/basic.vert -o build/basic_vertex.spv
	glslc shaders/basic.frag -o build/basic_fragment.spv

	$(CC) -o build/vulkan_renderer src/main_wayland.c $(CFLAGS) $(LDLIBS) $(WL_CLIENT)

win32:
	mkdir -p build
	glslc shaders/basic.vert -o build/basic_vertex.spv
	glslc shaders/basic.frag -o build/basic_fragment.spv

	$(CC) -o build/vulkan_renderer src/main_win32.c $(CFLAGS) -lvulkan-1 -lm -lgdi32

run:
	cd build && ./vulkan_renderer

debug:
	cd build && gdb vulkan_renderer
