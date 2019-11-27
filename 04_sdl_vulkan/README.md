* Use `export VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_standard_validation` to activate validation layers

> Layers can also be activated via the VK_INSTANCE_LAYERS environment variable.
> All validation layers work with the DEBUG_REPORT extension to provide validation feedback. When a validation layer is enabled, it will look for a vk_layer_settings.txt file (see "Using Layers" section below for more details) to define its logging behavior, which can include sending output to a file, stdout, or debug output (Windows). Applications can also register debug callback functions via the DEBUG_REPORT extension to receive callbacks when validation events occur. Application callbacks are independent of settings in a vk_layer_settings.txt file which will be carried out separately. If no vk_layer_settings.txt file is present and no application callbacks are registered, error messages will be output through default logging callbacks.

* Dynamically loading Vulkan: https://wiki.libsdl.org/SDL_Vulkan_LoadLibrary, https://wiki.libsdl.org/SDL_Vulkan_GetVkInstanceProcAddr
