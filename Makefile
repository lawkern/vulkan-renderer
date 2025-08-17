CFLAGS = -g3 -Wall -Wextra -DDEBUG -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable
LDLIBS = -lvulkan

# NOTE: For Wayland code generation:
WL_SCANNER   = $$(pkg-config wayland-scanner --variable=wayland_scanner)
WL_PROTOCOLS = $$(pkg-config wayland-protocols --variable=pkgdatadir)
WL_CLIENT    = $$(pkg-config wayland-client --cflags --libs)

WL_SHELL_PATH = stable/xdg-shell/xdg-shell.xml
WL_DECORATION_PATH = unstable/xdg-decoration/xdg-decoration-unstable-v1.xml


compile:
	mkdir -p src/external
	eval $(WL_SCANNER) client-header $(WL_PROTOCOLS)/$(WL_SHELL_PATH) src/external/xdg-shell-client-protocol.h
	eval $(WL_SCANNER) private-code  $(WL_PROTOCOLS)/$(WL_SHELL_PATH) src/external/xdg-shell-protocol.c

	eval $(WL_SCANNER) client-header $(WL_PROTOCOLS)/$(WL_DECORATION_PATH) src/external/xdg-decoration-unstable-v1-client-protocol.h
	eval $(WL_SCANNER) private-code  $(WL_PROTOCOLS)/$(WL_DECORATION_PATH) src/external/xdg-decoration-unstable-v1-protocol.c

	glslc shaders/basic.vert -o basic_vertex.spv
	glslc shaders/basic.frag -o basic_fragment.spv

	$(CC) -o vulkan_renderer_wayland src/main_wayland.c $(CFLAGS) $(LDLIBS) $(WL_CLIENT)

debug:
	gdb vulkan_renderer_wayland
