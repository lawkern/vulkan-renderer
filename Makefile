CFLAGS = -g3 -Wall -Wextra -DDEBUG -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable
LDLIBS = -lvulkan -lm

# NOTE: For Wayland code generation:
WL_SCANNER    = $$(pkg-config wayland-scanner --variable=wayland_scanner)
WL_PROTOCOLS  = $$(pkg-config wayland-protocols --variable=pkgdatadir)
WL_CLIENT     = $$(pkg-config wayland-client --cflags --libs)
WL_SHELL      = stable/xdg-shell/xdg-shell.xml
WL_DECORATION = unstable/xdg-decoration/xdg-decoration-unstable-v1.xml

compile:
	mkdir -p src/external
	wayland-scanner client-header < $(WL_PROTOCOLS)/$(WL_SHELL) > src/external/xdg-shell-client-protocol.h
	wayland-scanner private-code  < $(WL_PROTOCOLS)/$(WL_SHELL) > src/external/xdg-shell-protocol.c
	wayland-scanner client-header < $(WL_PROTOCOLS)/$(WL_DECORATION) > src/external/xdg-decoration-unstable-v1-client-protocol.h
	wayland-scanner private-code  < $(WL_PROTOCOLS)/$(WL_DECORATION) > src/external/xdg-decoration-unstable-v1-protocol.c

	mkdir -p build
	glslc shaders/basic.vert -o build/basic_vertex.spv
	glslc shaders/basic.frag -o build/basic_fragment.spv

	$(CC) -o build/vulkan_renderer_wayland src/main_wayland.c $(CFLAGS) $(LDLIBS) $(WL_CLIENT)

run:
	cd build && ./vulkan_renderer_wayland

debug:
	cd build && gdb vulkan_renderer_wayland
