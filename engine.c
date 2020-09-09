#include <stdio.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>

#include "log.h"
#include "vulkan_context.h"

static int init_global_libs() {
  if (!glfwInit()) {
    log_warn("Could not initialize GLFW\n");
    return -1;
  }
  int major, minor, revision;
  glfwGetVersion(&major, &minor, &revision);
  /*Print the version of glfw we are using*/
  log_verbose("Using GLFW %i.%i.%i\n", major, minor, revision);
  return 0;
}

int main(int argc, char **argv) {
  /*This sets the log output to stdout*/
  g_log->fp = stdout;
  g_log->verbosity = VER_VERBOSE;
  if (init_global_libs() < 0) {
    log_fatal("Could not initialize libraries\n");
    exit(EXIT_FAILURE);
  }

  struct vulkan_context_opts opts = {.w_opts = {.width = 640,
                                                .height = 400,
                                                .title = "Engine",
                                                .resizable = 0},
                                     .d_opts = {NULL},
                                     .enable_validation = 1};
  vulkan_context *vkctx;
  if (init_vulkan_context(&vkctx, &opts) < 0) {
    log_fatal("Could not create a graphics context\n");
    exit(EXIT_FAILURE);
  }
  temp_glfw_loop(vkctx);
}
