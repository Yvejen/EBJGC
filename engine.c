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

  struct window_opts opts = {.width = 640, .height = 400, .title = "Engine"};
  vulkan_context *vkctx = create_application_vulkan_context(&opts);
  temp_glfw_loop(vkctx);
}
