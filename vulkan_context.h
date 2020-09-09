#ifndef _H_VULKAN_CONTEXT_
#define _H_VULKAN_CONTEXT_
#include <stdint.h>

struct window_opts {
  uint32_t width, height;
  const char *title;
  int resizable : 1;
};

struct device_opts {
  const char *device_name;
};

struct vulkan_context_opts {
  struct window_opts w_opts;
  struct device_opts d_opts;
  int enable_validation : 1;
};

/* Handle representing a fully functional vulkan context. (Instance, Device,
 * Queues, etc.) */
typedef struct vulkan_context vulkan_context;

int init_vulkan_context(vulkan_context **vkctx,
                        struct vulkan_context_opts *opts);

void temp_glfw_loop(vulkan_context *vkctx);
#endif
