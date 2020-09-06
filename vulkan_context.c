#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "log.h"
#include "vulkan_context.h"
#include "xallocs.h"

struct vulkan_context {
  GLFWwindow *window;
  VkInstance instance;
};

static int create_vulkan_window_glfw(struct window_opts *opts,
                                     vulkan_context *vkctx) {

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  GLFWwindow *window =
      glfwCreateWindow(opts->width, opts->height, opts->title, NULL, NULL);
  if (!window) {
    log_warn("Failed to create a window\n");
    return -1;
  }
  vkctx->window = window;
  return 0;
}

static const char **get_vulkan_extensions(struct window_opts *opts,
                                          uint32_t *count) {
  uint32_t glfw_count = 0;
  const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_count);
  *count = glfw_count;
  return glfw_extensions;
}

static int create_vulkan_instance(struct window_opts *opts,
                                  vulkan_context *vkctx) {
  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = NULL;
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
  app_info.pEngineName = NULL;
  app_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.ppEnabledExtensionNames =
      get_vulkan_extensions(opts, &create_info.enabledExtensionCount);
  create_info.enabledLayerCount = 0;

  log_verbose("Creating Vulkan instance with following extensions:\n");
  for (uint32_t i = 0; i < create_info.enabledExtensionCount; i++) {
    log_verbose("\t%s\n", create_info.ppEnabledExtensionNames[i]);
  }

  int success = 0;
  VkResult res = vkCreateInstance(&create_info, NULL, &vkctx->instance);
  if (VK_SUCCESS != res) {
    log_warn("Vulkan Instance creation failed:");
    switch (res) {
    case VK_ERROR_LAYER_NOT_PRESENT:
      log_warn("VK_ERROR_LAYER_NOT_PRESENT\n");
      break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      log_warn("VK_ERROR_EXTENSION_NOT_PRESENT\n");
      break;
    default:
      log_warn("Unknown reason\n");
    }
    success = -1;
  }
  return success;
}

vulkan_context *create_application_vulkan_context(struct window_opts *opts) {
  assert(opts);
  vulkan_context *vkctx = xmalloc(sizeof *vkctx);
  memset(vkctx, 0, sizeof(*vkctx));

  create_vulkan_window_glfw(opts, vkctx);
  if (create_vulkan_instance(opts, vkctx) < 0) {
    log_fatal("Could not create a vulkan instance. exiting...\n");
    exit(EXIT_FAILURE);
  }
  log_verbose("Vulkan instance created\n");
  return vkctx;
}

void temp_glfw_loop(vulkan_context *vkctx) {
  while (!glfwWindowShouldClose(vkctx->window)) {
    glfwPollEvents();
  }
}
