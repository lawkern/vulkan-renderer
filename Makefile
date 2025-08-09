CFLAGS = -g3 -Wall -Wextra
LDLIBS = -lvulkan

compile:
	$(CC) -o vulkan_renderer src/main.c $(CFLAGS) $(LDLIBS)
