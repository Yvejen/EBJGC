#ifndef _H_VULKAN_CONTEXT_
#define _H_VULKAN_CONTEXT_
#include <stdint.h>

struct window_opts {
  uint32_t width, height;
  const char *title;
};

struct vulkan_device_opts {
  const char *device_name;
};

/* Handle representing a fully functional vulkan context. (Instance, Device,
 * Queues, etc.) */
typedef struct vulkan_context vulkan_context;

vulkan_context *create_application_vulkan_context(struct window_opts *cfg);

void temp_glfw_loop(vulkan_context *vkctx);
#endif
