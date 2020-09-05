#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include "vulkan_context.h"
#include "xallocs.h"

struct vulkan_context {
    GLFWwindow *window;
    VkInstance  instance;
};

/*Initializes glfw and creates the application window*/
static void create_vulkan_window_glfw(cfg_store *cfg, vulkan_context *vkctx) {
    if(!glfwInit()) {
        fprintf(stderr, "Failed to initialize glfw3\n");
    }
    if(CFG_GET_BOOLEAN(cfg, DEBUG_VERBOSE)) {
        /*Print the version of glfw we are using*/
        int major, minor, revision;
        glfwGetVersion(&major, &minor, &revision);
        fprintf(stderr, "Using GLFW %i.%i.%i\n", major, minor, revision);
    }


    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = glfwCreateWindow(CFG_GET_SIZE(cfg, WINDOW_WIDTH), CFG_GET_SIZE(cfg, WINDOW_HEIGHT), CFG_GET_CHAR_P(cfg, WINDOW_TITLE), NULL, NULL);
    if(!window) {
        fprintf(stderr, "Failed to create a window with glfw.\n");
        exit(EXIT_FAILURE);
    }
    vkctx->window = window;
}

static const char **get_vulkan_extensions(cfg_store *cfg, uint32_t *count) {
    uint32_t glfw_count = 0;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_count);
    *count = glfw_count;
    return glfw_extensions;
}

static int create_vulkan_instance(cfg_store *cfg, vulkan_context *vkctx) {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = NULL;
    app_info.applicationVersion = VK_MAKE_VERSION(0,0,0);
    app_info.pEngineName = NULL;
    app_info.engineVersion = VK_MAKE_VERSION(0,0,0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.ppEnabledExtensionNames = get_vulkan_extensions(cfg, &create_info.enabledExtensionCount);
    create_info.enabledLayerCount = 0;

    if(CFG_GET_BOOLEAN(cfg, DEBUG_VERBOSE)) {
        fprintf(stderr, "Creating Vulkan instance with following extensions:\n");
        for(uint32_t i=0; i < create_info.enabledExtensionCount; i++) {
            fprintf(stderr, "\t%s\n", create_info.ppEnabledExtensionNames[i]);
        }
    }

    int success = 0;
    VkResult res = vkCreateInstance(&create_info, NULL, &vkctx->instance);
    if(VK_SUCCESS != res) {
        fprintf(stderr, "Vulkan Instance creation failed:");
        switch(res) {
            case VK_ERROR_LAYER_NOT_PRESENT:
                fprintf(stderr, "VK_ERROR_LAYER_NOT_PRESENT\n");
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                fprintf(stderr, "VK_ERROR_EXTENSION_NOT_PRESENT\n");
                break;
            default:
                fprintf(stderr, "Unknown reason\n");
        }
        success = -1;
    }
    return success;
}

vulkan_context *create_application_vulkan_context(cfg_store *cfg) {
    assert(cfg);
    vulkan_context *vkctx = xmalloc(sizeof *vkctx);
    memset(vkctx, 0, sizeof(*vkctx));

    create_vulkan_window_glfw(cfg, vkctx);
    if(create_vulkan_instance(cfg, vkctx) < 0) {
        fprintf(stderr, "Could not create a vulkan instance. exiting...\n");
        exit(EXIT_FAILURE);
    }
    puts("---SUCCESS---");
    return vkctx;
}


void temp_glfw_loop(vulkan_context *vkctx) {
    while(!glfwWindowShouldClose(vkctx->window)) {
        glfwPollEvents();
    }
}
