project('EBJGC', 'c', version : '0.0.1', default_options : ['warning_level=2','c_std=c11'])

#GLFW and Vulkan for Graphics
glfw_dep = dependency('glfw3')
vulkan_dep = dependency('vulkan')


engine_src = ['engine.c','vulkan_context.c','cfgstore.c']
engine_inc = include_directories('.')

executable('engine', engine_src, include_directories : engine_inc, dependencies : [glfw_dep, vulkan_dep])
