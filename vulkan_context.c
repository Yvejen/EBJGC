#include <assert.h>
#include <limits.h>
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

struct vulkan_context {
  GLFWwindow *window;
  VkInstance instance;
  VkPhysicalDevice phy_device;
  VkDevice device;
  VkSurfaceKHR surface;
  VkQueue graphics_queue;
  VkQueue present_queue;
  VkDebugUtilsMessengerEXT debug_messenger;
  int graphics_present_unified : 1;
};

static int init_window_glfw(struct vulkan_context *vkctx,
                            struct vulkan_context_opts *opts) {
  assert(vkctx);
  assert(opts);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE,
                 opts->w_opts.resizable ? GLFW_TRUE : GLFW_FALSE);

  GLFWwindow *window = glfwCreateWindow(opts->w_opts.width, opts->w_opts.height,
                                        opts->w_opts.title, NULL, NULL);
  if (!window) {
    log_warn("Failed to create a GLFW window\n");
    return -1;
  }
  vkctx->window = window;
  return 0;
}

static const char **get_instance_extensions(uint32_t *count,
                                            struct vulkan_context_opts *opts) {
  /*These are the glfw requested extension, like VK_KHR_surface*/
  uint32_t glfw_count = 0;
  const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_count);

  static const char *debug_extension[] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
  uint32_t debug_count = ASIZE(debug_extension);

  const char **ret = NULL;
  uint32_t ret_count = 0;

  if (opts->enable_validation) {
    ret_count = glfw_count + debug_count;
    ret = xarray(const char *, ret_count);
    memcpy(ret, glfw_extensions, sizeof(const char *) * glfw_count);
    memcpy(&ret[glfw_count], debug_extension,
           sizeof(const char *) * debug_count);
  } else {
    ret_count = glfw_count;
    ret = xarray(const char *, ret_count);
    memcpy(ret, glfw_extensions, sizeof(const char *) * glfw_count);
  }

  *count = ret_count;
  return ret;
}

static const char **get_instance_layers(uint32_t *count,
                                        struct vulkan_context_opts *opts) {

  static const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
  static const uint32_t validation_count = ASIZE(validation_layers);

  uint32_t ret_count = 0;
  const char **ret = NULL;

  if (opts->enable_validation) {
    ret_count = validation_count;
    ret = xarray(const char *, ret_count);
    memcpy(ret, validation_layers, sizeof(const char *) * ret_count);
  } else {
    ret_count = 0;
    ret = NULL;
  }

  *count = ret_count;
  return ret;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback, void *user_data) {

  log_verbose("[VALIDATION_LAYER]: %s\n", callback->pMessage);
  return VK_FALSE;
}

static int init_debug_messenger(vulkan_context *vkctx,
                                struct vulkan_context_opts *opts) {
  VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
  debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
  debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debug_info.pfnUserCallback = debug_callback;
  debug_info.pUserData = NULL;

  PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          vkctx->instance, "vkCreateDebugUtilsMessengerEXT");

  if (vkCreateDebugUtilsMessengerEXT(vkctx->instance, &debug_info, NULL,
                                     &vkctx->debug_messenger) != VK_SUCCESS) {
    log_warn("Failed to create debug messenger\n");
    return -1;
  } else {
    log_verbose("Created debug messenger\n");
    return 0;
  }
}

static int init_vulkan_instance(vulkan_context *vkctx,
                                struct vulkan_context_opts *opts) {
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
  const char **instance_extensions =
      get_instance_extensions(&create_info.enabledExtensionCount, opts);
  create_info.ppEnabledExtensionNames = instance_extensions;
  const char **layer_names =
      get_instance_layers(&create_info.enabledLayerCount, opts);
  create_info.ppEnabledLayerNames = layer_names;

  log_verbose("Creating vulkan instance with following extensions:\n");
  for (uint32_t i = 0; i < create_info.enabledExtensionCount; i++) {
    log_verbose("\t%s\n", create_info.ppEnabledExtensionNames[i]);
  }

  if (opts->enable_validation) {
    log_verbose("Validation Layers requsted:\n");
    for (uint32_t i = 0; i < create_info.enabledLayerCount; i++) {
      log_verbose("\t%s\n", create_info.ppEnabledLayerNames[i]);
    }
  }

  int success = 0;
  VkResult res = vkCreateInstance(&create_info, NULL, &vkctx->instance);
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
  xfree(instance_extensions);
  xfree(layer_names);
  return success;
}

static VkDeviceSize
get_local_memory(VkPhysicalDeviceMemoryProperties *mem_props) {
  VkDeviceSize local_memory = 0;
  for (uint32_t i = 0; i < mem_props->memoryHeapCount; i++) {
    local_memory +=
        mem_props->memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT
            ? mem_props->memoryHeaps[i].size
            : 0;
  }
  return local_memory;
}

static int get_queue_indices(VkPhysicalDevice device, VkSurfaceKHR surface,
                             uint32_t *graphics_index,
                             uint32_t *present_index) {
  uint32_t qfamilies_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &qfamilies_count, NULL);
  VkQueueFamilyProperties *qfamilies =
      xarray(VkQueueFamilyProperties, qfamilies_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &qfamilies_count, qfamilies);

  int has_graphics = 0;
  int has_present = 0;
  uint32_t gindex_int = 0;
  uint32_t pindex_int = 0;

  for (uint32_t i = 0; i < qfamilies_count; i++) {
    if (qfamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      has_graphics = 1;
      gindex_int = i;
    }
    VkBool32 can_present = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &can_present);
    if (can_present) {
      has_present = 1;
      pindex_int = i;
    }
    if (has_graphics && has_present) {
      break;
    }
  }

  if (graphics_index) {
    *graphics_index = gindex_int;
  }
  if (present_index) {
    *present_index = pindex_int;
  }

  xfree(qfamilies);
  return has_graphics && has_present;
}

/* This rates devices with a pretty basic heuristic. Higher VRAM equals higher
 * score and discrete GPUs get a higher multiplier on their score than
 * integrated GPUs.
 */
static int examine_physical_device(VkPhysicalDevice device,
                                   vulkan_context *vkctx,
                                   struct vulkan_context_opts *opts) {
  VkPhysicalDeviceProperties props;
  VkPhysicalDeviceMemoryProperties memory;
  VkPhysicalDeviceFeatures features;
  uint32_t qfamilies_count = 0;
  VkQueueFamilyProperties *qfamilies;

  vkGetPhysicalDeviceProperties(device, &props);
  vkGetPhysicalDeviceMemoryProperties(device, &memory);
  vkGetPhysicalDeviceFeatures(device, &features);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &qfamilies_count, NULL);
  qfamilies = xarray(VkQueueFamilyProperties, qfamilies_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &qfamilies_count, qfamilies);

  log_verbose("%s\n", props.deviceName);
  log_verbose("Device memory:\n");
  for (uint32_t i = 0; i < memory.memoryHeapCount; i++) {
    log_verbose("\tHeap #%u: %uMB\n", i, TO_MBYTE(memory.memoryHeaps[i].size));
  }
  log_verbose("\t%uMB of memory is device local\n",
              TO_MBYTE(get_local_memory(&memory)));

  int device_metric = 0;
  switch (props.deviceType) {
  case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    device_metric += 4;
    break;
  case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
    device_metric += 2;
    break;
  default:
    device_metric += 1;
  }
  device_metric *= TO_GBYTE(get_local_memory(&memory));

  log_verbose("Queue families:\n");
  for (uint32_t i = 0; i < qfamilies_count; i++) {
    {
      int sep = 0;
      int tmp_sep = 0;
      log_verbose("\tQueueFamily #%u: ", i);
      log_verbose("%s%s", sep ? " | " : "",
                  qfamilies[i].queueFlags &VK_QUEUE_GRAPHICS_BIT ? tmp_sep = 1,
                  "GRAPHICS" : "");
      sep = tmp_sep;
      log_verbose("%s%s", sep ? " | " : "",
                  qfamilies[i].queueFlags &VK_QUEUE_COMPUTE_BIT ? tmp_sep = 1,
                  "COMPUTE " : "");
      sep = tmp_sep;
      log_verbose("%s%s", sep ? " | " : "",
                  qfamilies[i].queueFlags &VK_QUEUE_TRANSFER_BIT ? tmp_sep = 1,
                  "TRANSFER" : "");
      sep = tmp_sep;
      log_verbose("%s%s", sep ? " | " : "",
                  qfamilies[i].queueFlags &VK_QUEUE_SPARSE_BINDING_BIT
                  ? tmp_sep = 1,
                  "SPARSE  " : "");
      log_verbose("\n");
    }
  }
  if (!get_queue_indices(device, vkctx->surface, NULL, NULL)) {
    log_verbose("No proper queues found\n");
    device_metric = 0;
  }

  xfree(qfamilies);
  return device_metric;
}

static int init_physical_device(vulkan_context *vkctx,
                                struct vulkan_context_opts *opts) {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(vkctx->instance, &device_count, NULL);
  VkPhysicalDevice *devices = xarray(VkPhysicalDevice, device_count);
  vkEnumeratePhysicalDevices(vkctx->instance, &device_count, devices);

  if (device_count == 0) {
    log_warn("No GPU found\n");
  }

  int max_score = 0;
  int index = -1;

  for (uint32_t i = 0; i < device_count; i++) {
    log_verbose("Found device #%u: ", i);
    int score = examine_physical_device(devices[i], vkctx, opts);
    if (score > max_score) {
      max_score = score;
      index = i;
    }
  }

  VkPhysicalDevice picked_device = VK_NULL_HANDLE;
  if (index != -1) {
    picked_device = devices[index];
  }

  xfree(devices);
  if (picked_device != VK_NULL_HANDLE) {
    log_verbose("Picked physical device #%u\n", index);
    vkctx->phy_device = picked_device;
    return 0;
  } else {
    log_warn("Could not find appropriate physical device\n");
    return -1;
  }
}

static int init_logical_device(vulkan_context *vkctx,
                               struct vulkan_context_opts *opts) {
  /*We already checked for the existance of proper queues in
   * init_physical_device*/
  uint32_t graphics_index, present_index;
  get_queue_indices(vkctx->phy_device, vkctx->surface, &graphics_index,
                    &present_index);
  if (graphics_index == present_index) {
    vkctx->graphics_present_unified = 1;
  }

  log_verbose("Using queue family #%u for Graphics\n", graphics_index);
  log_verbose("Using queue family #%u for Present\n", present_index);

  float qpriority = 1.0;
  VkDeviceQueueCreateInfo qcreate_info_temp = {};
  qcreate_info_temp.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  qcreate_info_temp.queueFamilyIndex = -1;
  qcreate_info_temp.queueCount = 1;
  qcreate_info_temp.pQueuePriorities = &qpriority;

  uint32_t qcreate_infos_count = 0;
  VkDeviceQueueCreateInfo qcreate_infos[4];

  if (vkctx->graphics_present_unified) {
    qcreate_infos[0] = qcreate_info_temp;
    qcreate_infos[0].queueFamilyIndex = graphics_index;
    qcreate_infos_count += 1;
  } else {
    qcreate_infos[0] = qcreate_info_temp;
    qcreate_infos[0].queueFamilyIndex = graphics_index;
    qcreate_infos[1] = qcreate_info_temp;
    qcreate_infos[1].queueFamilyIndex = present_index;
    qcreate_infos_count += 2;
  }

  VkPhysicalDeviceFeatures features = {};

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = qcreate_infos;
  create_info.queueCreateInfoCount = qcreate_infos_count;
  create_info.pEnabledFeatures = &features;

  VkResult res =
      vkCreateDevice(vkctx->phy_device, &create_info, NULL, &vkctx->device);
  if (res != VK_SUCCESS) {
    log_warn("Logical device creation failed: ");
    switch (res) {
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      log_warn("VK_ERROR_EXTENSION_NOT_PRESENT\n");
      break;
    case VK_ERROR_FEATURE_NOT_PRESENT:
      log_warn("VK_ERROR_FEATURE_NOT_PRESENT\n");
      break;
    default:
      log_warn("Unknown\n");
    }
    return -1;
  }
  /*Device was created, so retrieve the queue handles*/
  vkGetDeviceQueue(vkctx->device, graphics_index, 0, &vkctx->graphics_queue);
  vkGetDeviceQueue(vkctx->device, present_index, 0, &vkctx->present_queue);
  return 0;
}

static int init_window_surface(vulkan_context *vkctx,
                               struct vulkan_context_opts *opts) {
  if (glfwCreateWindowSurface(vkctx->instance, vkctx->window, NULL,
                              &vkctx->surface) != VK_SUCCESS) {
    log_warn("Failed to create window surface\n");
    return -1;
  } else {
    return 0;
  }
}

int init_vulkan_context(vulkan_context **vkctx_out,
                        struct vulkan_context_opts *opts) {
  assert(opts);

  vulkan_context *vkctx = xarray(vulkan_context, 1);
  xclear(vkctx, 1);

  if (init_window_glfw(vkctx, opts) < 0) {
    goto exit_free_context;
  }
  if (init_vulkan_instance(vkctx, opts) < 0) {
    goto exit_destroy_window;
  }
  if (opts->enable_validation) {
    init_debug_messenger(vkctx, opts);
  }
  if (init_window_surface(vkctx, opts) < 0) {
    goto exit_destroy_instance;
  }
  if (init_physical_device(vkctx, opts) < 0) {
    goto exit_destroy_surface;
  }
  if (init_logical_device(vkctx, opts) < 0) {
    goto exit_destroy_surface;
  }

  *vkctx_out = vkctx;
  return 0;
exit_destroy_surface:
  vkDestroySurfaceKHR(vkctx->instance, vkctx->surface, NULL);
exit_destroy_instance:
  if (vkctx->debug_messenger) {
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            vkctx->instance, "vkDestroyDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT(vkctx->instance, vkctx->debug_messenger,
                                    NULL);
  }
  vkDestroyInstance(vkctx->instance, NULL);
exit_destroy_window:
  glfwDestroyWindow(vkctx->window);
exit_free_context:
  log_warn("Could not create a vulcan context\n");
  xfree(vkctx);
  *vkctx_out = NULL;
  return -1;
}

void temp_glfw_loop(vulkan_context *vkctx) {
  assert(vkctx->window);
  while (!glfwWindowShouldClose(vkctx->window)) {
    glfwPollEvents();
  }
}
