#ifndef _H_VULKAN_CONTEXT_
#define _H_VULKAN_CONTEXT_

#include "cfgstore.h"

/* Handle representing a fully functional vulkan instance. (Instance, Device, Queues, etc.) */
typedef struct vulkan_context vulkan_context;

vulkan_context *create_application_vulkan_context(cfg_store *cfg);

void temp_glfw_loop(vulkan_context *vkctx);
#endif
