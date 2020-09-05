#include <stdio.h>

#include "cfgstore.h"
#include "vulkan_context.h"

int main(int argc, char **argv) {
    cfg_store *cfg = cfg_create();

    CFG_SET_SIZE(cfg, WINDOW_WIDTH, 640);
    CFG_SET_SIZE(cfg, WINDOW_HEIGHT, 400);
    CFG_SET_CHAR_P(cfg, WINDOW_TITLE, "Hello World");
    CFG_SET_BOOLEAN(cfg, DEBUG_VERBOSE, 1);

    vulkan_context *vkctx = create_application_vulkan_context(cfg);
    temp_glfw_loop(vkctx);


    cfg_destroy(cfg);
}
