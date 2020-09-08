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

#define TO_MBYTE(s) ((s) / (1024 * 1024))
#define TO_GBYTE(s) ((s) / (1024 * 1024 * 1024))

struct vulkan_queue_info {
  uint32_t graphics_index, present_index, compute_index, transfer_index;
  int graphics_exist : 1;
  int present_exists : 1;
  int compute_exists : 1;
  int transfer_exists : 1;
  int unified_graphics_present : 1;
};

struct vulkan_context {
  GLFWwindow *window;
  VkInstance instance;
  VkPhysicalDevice phy_device;
  VkDevice device;
  VkSurfaceKHR surface;
  struct vulkan_queue_info queue_info;
};

static int create_vulkan_window_glfw(struct window_opts *opts,
                                     GLFWwindow **wpp) {

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  GLFWwindow *window =
      glfwCreateWindow(opts->width, opts->height, opts->title, NULL, NULL);
  if (!window) {
    log_warn("Failed to create a window\n");
    return -1;
  }
  *wpp = window;
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
                                  VkInstance *vkinst) {
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

  log_verbose("Creating vulkan instance with following extensions:\n");
  for (uint32_t i = 0; i < create_info.enabledExtensionCount; i++) {
    log_verbose("\t%s\n", create_info.ppEnabledExtensionNames[i]);
  }

  int success = 0;
  VkResult res = vkCreateInstance(&create_info, NULL, vkinst);
  if (VK_SUCCESS != res) {
    log_warn("Vulkan instance creation failed:");
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

static int rate_physical_device(struct vulkan_device_opts *opts,
                                VkPhysicalDevice device) {
  VkPhysicalDeviceProperties dev_props;
  vkGetPhysicalDeviceProperties(device, &dev_props);

  int device_metric = 0;

  switch (dev_props.deviceType) {
  case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    device_metric += 3;
    break;
  case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
    device_metric += 2;
    break;
  default:
    device_metric += 1;
  }

  VkPhysicalDeviceMemoryProperties mem_props;
  vkGetPhysicalDeviceMemoryProperties(device, &mem_props);

  VkDeviceSize local_memory = 0;
  for (uint32_t i = 0; i < mem_props.memoryHeapCount; i++) {
    local_memory +=
        mem_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT
            ? mem_props.memoryHeaps[i].size
            : 0;
  }
  device_metric *= TO_GBYTE(local_memory);
  log_verbose("Device name: %s | Score: %i\n", dev_props.deviceName,
              device_metric);

  return device_metric;
}

static VkPhysicalDevice pick_physical_device(struct vulkan_device_opts *opts,
                                             VkInstance vkinst) {
  assert(vkinst);
  /*assert(opts);*/

  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(vkinst, &device_count, NULL);
  VkPhysicalDevice *devices = xarray(VkPhysicalDevice, device_count);
  vkEnumeratePhysicalDevices(vkinst, &device_count, devices);

  VkPhysicalDevice phy_device = VK_NULL_HANDLE;
  for (uint32_t i = 0; i < device_count; i++) {
    log_verbose("Looking at device #%i: ", i);
    if (rate_physical_device(opts, devices[i])) {
      log_verbose("Picked Device #%i\n", i);
      phy_device = devices[i];
      break;
    }
  }

  xfree(devices);
  return phy_device;
}

static void query_device_queue_info(VkPhysicalDevice device,
                                    struct vulkan_queue_info *info,
                                    VkSurfaceKHR surface) {

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
  VkQueueFamilyProperties *queue_family_props =
      xarray(VkQueueFamilyProperties, queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_family_props);

  for (uint32_t i = 0; i < queue_family_count; i++) {
    VkBool32 can_present = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &can_present);

    log_verbose("QueueFamily #%u has:\n", i);
    if (queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      log_verbose("\tGRAPHICS_BIT\n");
      info->graphics_exist = 1;
      info->graphics_index = i;
    }
    if (queue_family_props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      log_verbose("\tTRANSFER_BIT\n");
      info->transfer_exists = 1;
      info->transfer_index = i;
    }
    if (queue_family_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      log_verbose("\tCOMPUTE_BIT\n");
      info->compute_exists = 1;
      info->compute_index = i;
    }
    if (can_present) {
      log_verbose("\tPRESENT_CAPABLE\n");
      info->present_exists = 1;
      info->present_index = i;
    }
  }

  xfree(queue_family_props);
}

static VkDevice vulkan_create_logical_device(struct vulkan_device_opts *opts,
                                             struct vulkan_queue_info *qinfo,
                                             VkPhysicalDevice phy_device) {
  VkDeviceQueueCreateInfo q_create_info = {};
  q_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  q_create_info.queueFamilyIndex = qinfo->graphics_index;
  q_create_info.queueCount = 1;
  float q_priority = 1.0f;
  q_create_info.pQueuePriorities = &q_priority;

  VkPhysicalDeviceFeatures device_features = {};

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = &q_create_info;
  create_info.queueCreateInfoCount = 1;
  create_info.pEnabledFeatures = &device_features;

  VkDevice device = VK_NULL_HANDLE;
  if (vkCreateDevice(phy_device, &create_info, NULL, &device) != VK_SUCCESS) {
    log_warn("Could not create a device\n");
    return VK_NULL_HANDLE;
  }
  return device;
}

vulkan_context *create_application_vulkan_context(struct window_opts *opts) {
  assert(opts);
  vulkan_context *vkctx = xarray(vulkan_context, 1);
  memset(vkctx, 0, sizeof(*vkctx));

  create_vulkan_window_glfw(opts, &vkctx->window);
  if (create_vulkan_instance(opts, &vkctx->instance) < 0) {
    log_fatal("Could not create a vulkan instance. exiting...\n");
    exit(EXIT_FAILURE);
  }
  log_verbose("Vulkan instance created\n");
  if ((vkctx->phy_device = pick_physical_device(NULL, vkctx->instance)) ==
      VK_NULL_HANDLE) {
    log_fatal("No suitable GPU found\n");
    exit(EXIT_FAILURE);
  }
  log_verbose("Found suitable GPU\n");
  if (VK_SUCCESS != glfwCreateWindowSurface(vkctx->instance, vkctx->window,
                                            NULL, &vkctx->surface)) {
    log_fatal("Could not create surface\n");
    exit(EXIT_FAILURE);
  }
  query_device_queue_info(vkctx->phy_device, &vkctx->queue_info,
                          vkctx->surface);

  if (!(vkctx->queue_info.graphics_exist && vkctx->queue_info.present_exists)) {
    log_fatal("Missing correct queue types\n");
    exit(EXIT_FAILURE);
  }
  if (vkctx->queue_info.graphics_index == vkctx->queue_info.present_index) {
    vkctx->queue_info.unified_graphics_present = 1;
  }
  log_verbose("Using queue family #%u for present\n",
              vkctx->queue_info.present_index);
  log_verbose("Using queue family #%u for graphics\n",
              vkctx->queue_info.graphics_index);

  if (VK_NULL_HANDLE == (vkctx->device = vulkan_create_logical_device(
                             opts, &vkctx->queue_info, vkctx->phy_device))) {
    log_fatal("Logical device creation failed");
    exit(EXIT_FAILURE);
  }
  log_verbose("Logical device created\n");

  return vkctx;
}

void temp_glfw_loop(vulkan_context *vkctx) {
  while (!glfwWindowShouldClose(vkctx->window)) {
    glfwPollEvents();
  }
}
