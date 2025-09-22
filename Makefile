.POSIX:
CFLAGS = -g3 -std=c99 -D_DEFAULT_SOURCE $(WARNINGS)
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
	glslc -o build/basic.vert.spv code/shaders/basic.vert
	glslc -o build/basic.frag.spv code/shaders/basic.frag

wayland:
	mkdir -p code/external
	eval wayland-scanner client-header < $(WL_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml > code/external/xdg-shell-client-protocol.h
	eval wayland-scanner private-code  < $(WL_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml > code/external/xdg-shell-protocol.c
	eval wayland-scanner client-header < $(WL_PROTOCOLS)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml > code/external/xdg-decoration-unstable-v1-client-protocol.h
	eval wayland-scanner private-code  < $(WL_PROTOCOLS)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml > code/external/xdg-decoration-unstable-v1-protocol.c

	eval $(CC) -o build/vulkan_renderer_debug   code/main_wayland.c -DDEBUG=1 $(CFLAGS) $(LDLIBS) -lvulkan $(WL_CLIENT)
	eval $(CC) -o build/vulkan_renderer_release code/main_wayland.c -DDEBUG=0 $(CFLAGS) $(LDLIBS) -lvulkan $(WL_CLIENT)

xlib:
	eval $(CC) -o build/vulkan_renderer_debug   code/main_xlib.c -DDEBUG=1 $(CFLAGS) $(LDLIBS) -lvulkan -lX11
	eval $(CC) -o build/vulkan_renderer_release code/main_xlib.c -DDEBUG=0 $(CFLAGS) $(LDLIBS) -lvulkan -lX11

win32:
	eval $(CC) -o build/vulkan_renderer_debug   code/main_win32.c -DDEBUG=1 $(CFLAGS) $(LDLIBS) -lvulkan-1 -lgdi32
	eval $(CC) -o build/vulkan_renderer_release code/main_win32.c -DDEBUG=0 $(CFLAGS) $(LDLIBS) -lvulkan-1 -lgdi32

run:
	cd build && ./vulkan_renderer_release

debug:
	cd build && gdb vulkan_renderer_debug

clean:
	rm -f build/*.spv
	rm -f build/*.exe
	rm -f build/*_debug
	rm -f build/*_release
