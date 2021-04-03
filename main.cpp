/****************************************************************************
 *                                                                          *
 *  Example of a simple 3D application using Vulkan API                     *
 *  Copyright (C) 2020 Artem Hlumov <artyom.altair@gmail.com>               *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.   *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *       The application shows a simple example of the ray tracing          *
 *                    pipeline drawing a solid cube.                        *
 *                                                                          *
 *    The example is designed to demonstrate pure sequence of actions       *
 *    required to create a ray tracing  application, so all code is done    *
 *      as a large main() function and some duplication is present.         *
 *                                                                          *
 *   You may thing about how to split this into modules in order to make    *
 *                    an engine for your application.                       *
 *                                                                          *
 *           The code is inspirited of the original tutorial:               *
 *                     https://vulkan-tutorial.com                          *
 *                                                                          *
 ****************************************************************************/

// Include GLFW (window SDK).
// Switch on support of Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Include GLM (linear algebra library).
// Use radians instead of degrees to disable deprecated functions.
#define GLM_FORCE_RADIANS
// GLM has been initially designed for OpenGL, so we have to apply a patch
// to make it work with Vulkan.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <set>
#include <array>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>

/**
 * Window width.
 */
constexpr int WINDOW_WIDTH = 800;
/**
 * Window height.
 */
constexpr int WINDOW_HEIGHT = 800;
/**
 * Application name.
 */
constexpr const char* APPLICATION_NAME = "VKExampleRTX";
/**
 * Maximal amount of frames processed at the same time.
 */
constexpr int MAX_FRAMES_IN_FLIGHT = 5;

/**
 * Callback function that will be called each time a validation level produces a message.
 * @param messageSeverity Bitmask specifying which severities of events cause a debug messenger callback.
 * @param messageType Bitmask specifying which types of events cause a debug messenger callback.
 * @param pCallbackData Structure specifying parameters returned to the callback.
 * @param pUserData Arbitrary user data provided when the callback is created.
 * @return True in case the Vulkan call should be aborted, false - otherwise.
 */
VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback
    (
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    )
{
    // Mark variables as not used to suppress warnings.
    (void) messageSeverity;
    (void) messageType;
    (void) pUserData;
    // Print the message.
    std::cerr << "[MSG]: " << pCallbackData->pMessage << std::endl;
    // Do only logging, do not abort the call.
    return VK_FALSE;
}

/**
 * Main function.
 * @return Return code of the application.
 */
int main()
{
    // ==========================================================================
    //                 STEP 1: Create a Window using GLFW
    // ==========================================================================
    // GLFW abstracts native calls to create the window and allows us to write
    // a cross-platform application.
    // ==========================================================================

    // Initialize GLFW context.
    glfwInit();
    // Do not create an OpenGL context - we use Vulkan.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Make the window not resizable.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    // Create a window instance.
    GLFWwindow* glfwWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APPLICATION_NAME, nullptr, nullptr);

    // ==========================================================================
    //                   STEP 2: Select Vulkan extensions
    // ==========================================================================
    // Vulkan has a list of extensions providing some functionality.
    // We should explicitly select extensions we need.
    // At least GLFW requires some graphical capabilities
    // in order to draw an image.
    // ==========================================================================

    // Take a minimal set of Vulkan extensions required by GLWF.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Fetch list of available Vulkan extensions.
    uint32_t vkNumAvailableExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &vkNumAvailableExtensions, nullptr);
    std::vector< VkExtensionProperties > vkAvailableExtensions(vkNumAvailableExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &vkNumAvailableExtensions, vkAvailableExtensions.data());

    // Construct a list of extensions we should request.
    std::vector< const char* > extensionsList(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef DEBUG_MODE

    // Add an extension to print debug messages.
    extensionsList.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    // Add extension required for RTX
    extensionsList.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    // ==========================================================================
    //              STEP 3: Select validation layers for debug
    // ==========================================================================
    // Validation layers is a mechanism to hook Vulkan API calls, validate
    // them and notify the user if something goes wrong.
    // Here we need to make sure that requested validation layers are available.
    // ==========================================================================

    // Specify desired validation layers.
    const std::vector< const char* > desiredValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    // Fetch available validation layers.
    uint32_t vkLayerCount;
    vkEnumerateInstanceLayerProperties(&vkLayerCount, nullptr);
    std::vector< VkLayerProperties > vkAvailableLayers(vkLayerCount);
    vkEnumerateInstanceLayerProperties(&vkLayerCount, vkAvailableLayers.data());

    // Check if desired validation layers are in the list.
    bool validationLayersAvailable = true;
    for (auto requestedLayer : desiredValidationLayers) {
        bool isLayerAvailable = false;
        for (auto availableLayer : vkAvailableLayers) {
            if (std::strcmp(requestedLayer, availableLayer.layerName) == 0) {
                isLayerAvailable = true;
                break;
            }
        }
        if (!isLayerAvailable) {
            validationLayersAvailable = false;
            break;
        }
    }
    if (!validationLayersAvailable) {
        std::cerr << "Desired validation levels are not available!";
        abort();
    }

    // ==========================================================================
    //                STEP 4: Create a debug message callback
    // ==========================================================================
    // Debug message callback allows us to display errors and warnings if
    // we do some mistake using Vulkan API.
    // ==========================================================================

    // Set up message logging.
    // See %VK_SDK_PATH%/Config/vk_layer_settings.txt for detailed information.
    VkDebugUtilsMessengerCreateInfoEXT vkMessangerCreateInfo {};
    vkMessangerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // Which severities of events cause a debug messenger callback.
    vkMessangerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // Which types of events cause a debug messenger callback.
    vkMessangerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    // Pointer to a callback function.
    vkMessangerCreateInfo.pfnUserCallback = messageCallback;
    // Here we can pass some arbitrary date to the callback function.
    vkMessangerCreateInfo.pUserData = nullptr;

#endif

    // ==========================================================================
    //                    STEP 5: Create a Vulkan instance
    // ==========================================================================
    // Vulkan instance is a starting point of using Vulkan API.
    // Here we specify API version and which extensions to use.
    // ==========================================================================

    // Specify application info and required Vulkan version.
    VkApplicationInfo vkAppInfo {};
    // Information about your application.
    vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vkAppInfo.pApplicationName = APPLICATION_NAME;
    vkAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    // Information about your 3D engine (if applicable).
    vkAppInfo.pEngineName = APPLICATION_NAME;
    vkAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    // Use v1.0 that is likely supported by the most of drivers.
    vkAppInfo.apiVersion = VK_API_VERSION_1_0;

    // Fill in an instance create structure.
    VkInstanceCreateInfo vkCreateInfo {};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkCreateInfo.pApplicationInfo = &vkAppInfo;
    // Specify which extensions we need.
    vkCreateInfo.enabledExtensionCount = extensionsList.size();
    vkCreateInfo.ppEnabledExtensionNames = extensionsList.data();

#ifdef DEBUG_MODE

    // Switch on all requested layers for debug mode.
    vkCreateInfo.enabledLayerCount = static_cast< uint32_t >(desiredValidationLayers.size());
    vkCreateInfo.ppEnabledLayerNames = desiredValidationLayers.data();
    // Debug message callbacks are attached after instance creation and should be destroyed
    // before instance destruction. Therefore they do not catch errors in these two calls.
    // To avoid this we can pass our debug messager duing instance creation.
    // This will apply it for vkCreateInstance() and vkDestroyInstance() calls only,
    // so in any case debug messages should be switched on after the instance is created.
    vkCreateInfo.pNext = reinterpret_cast< VkDebugUtilsMessengerCreateInfoEXT* >(&vkMessangerCreateInfo);

#else

    // Do not use layers in release mode.
    vkCreateInfo.enabledLayerCount = 0;

#endif

    // Create a Vulkan instance and check its validity.
    VkInstance vkInstance;
    if (vkCreateInstance(&vkCreateInfo, nullptr, &vkInstance) != VK_SUCCESS) {
        std::cerr << "Failed to creae a Vulkan instance!";
        abort();
    }

    // ==========================================================================
    //                    STEP 6: Create a window surface
    // ==========================================================================
    // Surface is an abstraction that works with the window system of your OS.
    // Athough it is possible to use platform-dependent calls to create
    // a surface, GLFW provides us a way to do this platform-agnostic.
    // ==========================================================================

    VkSurfaceKHR vkSurface;
    if (glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, &vkSurface) != VK_SUCCESS) {
        std::cerr << "Failed to create a surface!" << std::endl;
        abort();
    }

#ifdef DEBUG_MODE

    // ==========================================================================
    //                    STEP 7: Attach a message handler
    // ==========================================================================
    // Attach a message handler to the Vulkan context in order to see
    // debug messages and warnings.
    // ==========================================================================

    // Since vkCreateDebugUtilsMessengerEXT is a function of an extension,
    // it is not explicitly declared in Vulkan header, because if we do not load
    // validation layers, it does not exist.
    // First of all we have to obtain a pointer to the function.
    auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast< PFN_vkCreateDebugUtilsMessengerEXT >(vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT"));
    if (vkCreateDebugUtilsMessengerEXT == nullptr) {
        std::cerr << "Function vkCreateDebugUtilsMessengerEXT not found!";
        abort();
    }

    // Create a messenger.
    VkDebugUtilsMessengerEXT vkDebugMessenger;
    vkCreateDebugUtilsMessengerEXT(vkInstance, &vkMessangerCreateInfo, nullptr, &vkDebugMessenger);

#endif

    // ==========================================================================
    //                      STEP 8: Pick a physical device
    // ==========================================================================
    // Physical devices correspond to graphical cards available in the system.
    // Before we continue, we should make sure the graphical card is suitable
    // for our needs and in case there is more than one card
    // in the system - select one of them.
    // We are going to perform some tests and fetch device information that
    // we will need after. So, to not request this information twice,
    // we will fill some structures declared below for the selected
    // physical device and use them in the rest of code.
    // ==========================================================================

    // --------------------------------------------------------------------------
    //                    Information about selected device
    // --------------------------------------------------------------------------
    // Here we will store a selected physical device.
    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    // Here we store information about queues supported by the selected physical device.
    // For our simple application device should support two queues:
    struct QueueFamilyIndices
    {
        // Graphics queue processes rendering requests and stores result into Vulkan images.
        std::optional< uint32_t > graphicsFamily;
        // Present queue tranfers images to the surface.
        std::optional< uint32_t > presentFamily;
    };
    QueueFamilyIndices queueFamilyIndices;
    // Here we take information about a swap chain.
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector< VkSurfaceFormatKHR > formats;
        std::vector< VkPresentModeKHR > presentModes;
    };
    SwapChainSupportDetails swapChainSupportDetails;
    // Here we keep a selected format for z-buffer.
    VkFormat vkDepthFormat = VK_FORMAT_UNDEFINED;
    // --------------------------------------------------------------------------

    // Desired extensions that should be supported by the graphical card.
    const std::vector< const char* > desiredDeviceExtensions = {
        // Swap chain extension is needed for drawing.
        // Any graphical card that aims to draw into a framebuffer
        // should support this extension.
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        // Ray tracing extensions.
        VK_NV_RAY_TRACING_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
    };

    // Get a list of available physical devices.
    uint32_t vkDeviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &vkDeviceCount, nullptr);
    if (vkDeviceCount == 0) {
        std::cerr << "No physical devices!" << std::endl;
        abort();
    }
    std::vector< VkPhysicalDevice > vkDevices(vkDeviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &vkDeviceCount, vkDevices.data());

    // Go through the list of physical device and select the first suitable one.
    // In advanced applications you may introduce a rating to choose
    // the best video card or let the user select one manually.
    for (auto device : vkDevices) {
        // Get properties of the physical device.
        VkPhysicalDeviceProperties vkDeviceProperties;
        vkGetPhysicalDeviceProperties(device, &vkDeviceProperties);

        // Get features of the physical device.
        VkPhysicalDeviceFeatures vkDeviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &vkDeviceFeatures);

        // Get extensions available for the physical device.
        uint32_t vkExtensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &vkExtensionCount, nullptr);
        std::vector< VkExtensionProperties > vkAvailableExtensions(vkExtensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &vkExtensionCount, vkAvailableExtensions.data());

        // ---------------------------------------------------
        // TEST 1: Check if all desired extensions are present
        // ---------------------------------------------------

        // Get list of extensions and compare it to desired one.
        std::set< std::string > requiredExtensions(desiredDeviceExtensions.begin(), desiredDeviceExtensions.end());
        for (const auto& extension : vkAvailableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        bool allExtensionsAvailable = requiredExtensions.empty();

        // ----------------------------------------------------------
        // TEST 2: Check if all required queue families are supported
        // ----------------------------------------------------------

        // Get list of available queue families.
        uint32_t vkQueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &vkQueueFamilyCount, nullptr);
        std::vector< VkQueueFamilyProperties > vkQueueFamilies(vkQueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &vkQueueFamilyCount, vkQueueFamilies.data());

        // Fill in QueueFamilyIndices structure to check that all required queue families are present.
        QueueFamilyIndices currentDeviceQueueFamilyIndices;
        for (uint32_t i = 0; i < vkQueueFamilyCount; i++) {
            // Take a queue family.
            const auto& queueFamily = vkQueueFamilies[i];

            // Check if this is a graphics family.
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                currentDeviceQueueFamilyIndices.graphicsFamily = i;
            }

            // Check if it supports presentation.
            // Note that graphicsFamily and presentFamily may refer to the same queue family
            // for some video cards and we should be ready to this.
            VkBool32 vkPresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vkSurface, &vkPresentSupport);
            if (vkPresentSupport) {
                currentDeviceQueueFamilyIndices.presentFamily = i;
            }
        }
        bool queuesOk = currentDeviceQueueFamilyIndices.graphicsFamily.has_value() &&
                        currentDeviceQueueFamilyIndices.presentFamily.has_value();

        // ---------------------------------------------------------
        // TEST 3: Check if the swap chain supports required formats
        // ---------------------------------------------------------

        // Fill swap chain information into a SwapChainSupportDetails structure.
        SwapChainSupportDetails currenDeviceSwapChainDetails;
        // We should do this only in case the device supports swap buffer.
        // To avoid extra flags and more complex logic, just make sure that all
        // desired extensions have been found.
        if (allExtensionsAvailable) {
            // Get surface capabilities.
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vkSurface, &currenDeviceSwapChainDetails.capabilities);
            // Get supported formats.
            uint32_t vkPhysicalDeviceFormatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &vkPhysicalDeviceFormatCount, nullptr);
            if (vkPhysicalDeviceFormatCount != 0) {
                currenDeviceSwapChainDetails.formats.resize(vkPhysicalDeviceFormatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &vkPhysicalDeviceFormatCount, currenDeviceSwapChainDetails.formats.data());
            }
            // Get supported present modes.
            uint32_t vkPhysicalDevicePresentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &vkPhysicalDevicePresentModeCount, nullptr);
            if (vkPhysicalDevicePresentModeCount != 0) {
                currenDeviceSwapChainDetails.presentModes.resize(vkPhysicalDevicePresentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &vkPhysicalDevicePresentModeCount, currenDeviceSwapChainDetails.presentModes.data());
            }
        }
        bool swapChainOk = !currenDeviceSwapChainDetails.formats.empty() &&
                           !currenDeviceSwapChainDetails.presentModes.empty();

        // ----------------------------------------------
        // TEST 4: Check if the depth buffer is avaialble
        // ----------------------------------------------

        // Select a format of depth buffer.
        // We have a list of formats we need to test and pick one.
        std::vector< VkFormat > depthFormatCandidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        VkFormat currentDepthFormat = VK_FORMAT_UNDEFINED;
        for (VkFormat format : depthFormatCandidates) {
            VkFormatProperties vkProps;
            vkGetPhysicalDeviceFormatProperties(device, format, &vkProps);
            if (vkProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                currentDepthFormat = format;
                break;
            }
        }
        bool depthFormatOk = (currentDepthFormat != VK_FORMAT_UNDEFINED);

        // Select the first suitable device.
        if (allExtensionsAvailable && queuesOk && swapChainOk && depthFormatOk) {
            vkPhysicalDevice = device;
            queueFamilyIndices = currentDeviceQueueFamilyIndices;
            swapChainSupportDetails = currenDeviceSwapChainDetails;
            vkDepthFormat = currentDepthFormat;
            break;
        }
    }

    // Check if we have found any suitable device.
    if (vkPhysicalDevice == VK_NULL_HANDLE) {
        std::cerr << "No suitable physical devices available!" << std::endl;
        abort();
    }

    // Request physical device memory properties that will be used to find a suitable memory type.
    VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkPhysicalDeviceMemoryProperties);

    // Query the ray tracing properties of the current implementation, we will need them later on.
    VkPhysicalDeviceRayTracingPropertiesNV rayTracingProperties{};
    rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
    VkPhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps2.pNext = &rayTracingProperties;
    vkGetPhysicalDeviceProperties2(vkPhysicalDevice, &deviceProps2);

    // ==========================================================================
    //                   STEP 9: Create a logical device
    // ==========================================================================
    // Logical devices are instances of the physical device created for
    // the particular application. We should create one in order to use it.
    // ==========================================================================

    // Logical device of a video card.
    VkDevice vkDevice;

    // As it was mentioned above, we might have two queue families referring
    // to the same index which means there is only one family that is suitable
    // for both needs.
    // Use std::set to filter out duplicates as we should mention each queue
    // only once during logical device creation.
    std::set< uint32_t > uniqueQueueFamilies = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value()
    };

    // Go through all remaining queues and make a creation info structure.
    std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Select physical device features we want to use.
    // As this is a quite simple application we need nothing special.
    // However, for more compex applications you might need to first
    // check if device supports features you need via
    // vkGetPhysicalDeviceFeatures() call in physical device suitability check.
    // If you specify something that is not supported - device
    // creation will fail, so you should check beforehand.
    VkPhysicalDeviceFeatures vkDeviceFeatures {};

    // Logical device creation info.
    VkDeviceCreateInfo vkDeviceCreateInfo {};
    vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkDeviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
    vkDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    vkDeviceCreateInfo.pEnabledFeatures = &vkDeviceFeatures;
    // Specify which extensions we want to enable.
    vkDeviceCreateInfo.enabledExtensionCount = static_cast< uint32_t >(desiredDeviceExtensions.size());
    vkDeviceCreateInfo.ppEnabledExtensionNames = desiredDeviceExtensions.data();

#ifdef DEBUG_MODE

    // Switch on all requested layers for debug mode.
    vkDeviceCreateInfo.enabledLayerCount = static_cast< uint32_t >(desiredValidationLayers.size());
    vkDeviceCreateInfo.ppEnabledLayerNames = desiredValidationLayers.data();

#else

    createInfo.enabledLayerCount = 0;

#endif

    // Create a logical device.
    if (vkCreateDevice(vkPhysicalDevice, &vkDeviceCreateInfo, nullptr, &vkDevice) != VK_SUCCESS) {
        std::cerr << "Failed to create a logical device!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                   STEP 10: Select surface configuration
    // ==========================================================================
    // We should select surface format, present mode and extent (size) from
    // the proposed values. They will be used in furhter calls.
    // ==========================================================================

    // Select a color format.
    VkSurfaceFormatKHR vkSelectedFormat = swapChainSupportDetails.formats[0];
    for (const auto& availableFormat : swapChainSupportDetails.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            vkSelectedFormat = availableFormat;
            break;
        }
    }

    // Select a present mode.
    VkPresentModeKHR vkSelectedPresendMode = swapChainSupportDetails.presentModes[0];
    for (const auto& availablePresentMode : swapChainSupportDetails.presentModes) {
        // Preferrable mode. If we find it, break the cycle immediately.
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            vkSelectedPresendMode = availablePresentMode;
            break;
        }
        // Fallback mode. If we find it, store to vkSelectedPresendMode,
        // but try to find something better.
        if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
            vkSelectedPresendMode = availablePresentMode;
        }
    }

    // Select a swap chain images resolution.
    VkExtent2D vkSelectedExtent;
    if (swapChainSupportDetails.capabilities.currentExtent.width != UINT32_MAX) {
        vkSelectedExtent = swapChainSupportDetails.capabilities.currentExtent;
    } else {
        // Some window managers do not allow to use resolution different from
        // the resolution of the window. In such cases Vulkan will report
        // UINT32_MAX as currentExtent.width and currentExtent.height.
        vkSelectedExtent = { WINDOW_WIDTH, WINDOW_HEIGHT };
        // Make sure the value is between minImageExtent.width and maxImageExtent.width.
        vkSelectedExtent.width = std::max(
                    swapChainSupportDetails.capabilities.minImageExtent.width,
                    std::min(
                        swapChainSupportDetails.capabilities.maxImageExtent.width,
                        vkSelectedExtent.width
                        )
                    );
        // Make sure the value is between minImageExtent.height and maxImageExtent.height.
        vkSelectedExtent.height = std::max(
                    swapChainSupportDetails.capabilities.minImageExtent.height,
                    std::min(
                        swapChainSupportDetails.capabilities.maxImageExtent.height,
                        vkSelectedExtent.height
                        )
                    );
    }

    // ==========================================================================
    //                     STEP 11: Create a swap chain
    // ==========================================================================
    // Swap chain is a chain of rendered images that are going to be displayed
    // on the screen. It is used to synchronize image rendering with refresh
    // rate of the screen (VSync). If the application generates frames faster
    // than they are displayed, it should wait.
    // ==========================================================================

    // First of all we should select a size of the swap chain.
    // It is recommended to use minValue + 1 but we also have to make sure
    // it does not exceed maxValue.
    // If maxValue is zero, it means there is no upper bound.
    uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
    if (swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilities.maxImageCount) {
        imageCount = swapChainSupportDetails.capabilities.maxImageCount;
    }

    // Fill in swap chain create info using selected surface configuration.
    VkSwapchainCreateInfoKHR vkSwapChainCreateInfo{};
    vkSwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vkSwapChainCreateInfo.surface = vkSurface;
    vkSwapChainCreateInfo.minImageCount = imageCount;
    vkSwapChainCreateInfo.imageFormat = vkSelectedFormat.format;
    vkSwapChainCreateInfo.imageColorSpace = vkSelectedFormat.colorSpace;
    vkSwapChainCreateInfo.imageExtent = vkSelectedExtent;
    vkSwapChainCreateInfo.imageArrayLayers = 1;
    vkSwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // We have two options for queue synchronization:
    // - VK_SHARING_MODE_EXCLUSIVE - An image ownership should be explicitly transferred
    //                               before using it in a differen queue. Best performance option.
    // - VK_SHARING_MODE_CONCURRENT - Images can be used in different queues without
    //                                explicit ownership transfer. Less performant, but simpler in implementation.
    // If we have only one queue family - we should use VK_SHARING_MODE_EXCLUSIVE as we do not need
    // to do any synchrnoization and can use the faster option for free.
    // If we have two queue families - we will use VK_SHARING_MODE_CONCURRENT mode to avoid
    // additional complexity of ownership transferring.
    uint32_t familyIndices[] = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value()
    };
    if (familyIndices[0] != familyIndices[1]) {
        vkSwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        vkSwapChainCreateInfo.queueFamilyIndexCount = 2;
        vkSwapChainCreateInfo.pQueueFamilyIndices = familyIndices;
    } else {
        vkSwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkSwapChainCreateInfo.queueFamilyIndexCount = 0;
        vkSwapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }
    vkSwapChainCreateInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
    vkSwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vkSwapChainCreateInfo.presentMode = vkSelectedPresendMode;
    vkSwapChainCreateInfo.clipped = VK_TRUE;
    // This option is only required if we recreate a swap chain.
    vkSwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create a swap chain.
    VkSwapchainKHR vkSwapChain;
    if (vkCreateSwapchainKHR(vkDevice, &vkSwapChainCreateInfo, nullptr, &vkSwapChain) != VK_SUCCESS) {
        std::cerr << "Failed to create a swap chain!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                 STEP 12: Create swap chain image views
    // ==========================================================================
    // After the swap chain is created, it contains Vulkan images that are
    // used to transfer rendered picture. In order to work with images
    // we should create image views.
    // ==========================================================================

    // Fetch Vulkan images associated to the swap chain.
    std::vector< VkImage > vkSwapChainImages;
    uint32_t vkSwapChainImageCount;
    vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &vkSwapChainImageCount, nullptr);
    vkSwapChainImages.resize(vkSwapChainImageCount);
    vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &vkSwapChainImageCount, vkSwapChainImages.data());

    // Create image views for each image.
    std::vector< VkImageView > vkSwapChainImageViews;
    vkSwapChainImageViews.resize(vkSwapChainImageCount);
    for (size_t i = 0; i < vkSwapChainImageCount; i++) {
        // Image view create info.
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = vkSwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vkSelectedFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        // Create an image view.
        if (vkCreateImageView(vkDevice, &createInfo, nullptr, &vkSwapChainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create an image view #" << i << "!" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    //                    STEP 14: Create a vertex buffer
    // ==========================================================================
    // Vertex buffer contains vertices of our model and will be used to construct
    // acceleration structures for ray tracing.
    // ==========================================================================

    // Create a cube specifying its vertices.
    // Each triplet of vertices represent one triangle.
    // We do not use index buffer, so some vertices are duplicated.
    std::vector< glm::vec3 > vertices
    {
        { -0.5f, -0.5f, -0.5f },
        { -0.5f,  0.5f, -0.5f },
        {  0.5f, -0.5f, -0.5f },
        {  0.5f,  0.5f, -0.5f },
        {  0.5f, -0.5f, -0.5f },
        { -0.5f,  0.5f, -0.5f },

        { -0.5f, -0.5f, -0.5f },
        { -0.5f, -0.5f,  0.5f },
        { -0.5f,  0.5f, -0.5f },
        { -0.5f,  0.5f,  0.5f },
        { -0.5f,  0.5f, -0.5f },
        { -0.5f, -0.5f,  0.5f },

        {  0.5f, -0.5f,  0.5f },
        {  0.5f, -0.5f, -0.5f },
        {  0.5f,  0.5f,  0.5f },
        {  0.5f,  0.5f, -0.5f },
        {  0.5f,  0.5f,  0.5f },
        {  0.5f, -0.5f, -0.5f },

        { -0.5f,  0.5f,  0.5f },
        { -0.5f, -0.5f,  0.5f },
        {  0.5f,  0.5f,  0.5f },
        {  0.5f, -0.5f,  0.5f },
        {  0.5f,  0.5f,  0.5f },
        { -0.5f, -0.5f,  0.5f },

        {  0.5f,  0.5f, -0.5f },
        { -0.5f,  0.5f, -0.5f },
        {  0.5f,  0.5f,  0.5f },
        { -0.5f,  0.5f,  0.5f },
        {  0.5f,  0.5f,  0.5f },
        { -0.5f,  0.5f, -0.5f },

        { -0.5f, -0.5f,  0.5f },
        { -0.5f, -0.5f, -0.5f },
        {  0.5f, -0.5f, -0.5f },
        { -0.5f, -0.5f,  0.5f },
        {  0.5f, -0.5f, -0.5f },
        {  0.5f, -0.5f,  0.5f },
    };

    // Calculate buffer size.
    VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

    // Describe a buffer.
    VkBufferCreateInfo vkVertexBufferInfo{};
    vkVertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkVertexBufferInfo.size = vertexBufferSize;
    vkVertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkVertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create a buffer.
    VkBuffer vkVertexBuffer;
    if (vkCreateBuffer(vkDevice, &vkVertexBufferInfo, nullptr, &vkVertexBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to create a vertex buffer!" << std::endl;
        abort();
    }

    // Retrieve memory requirements for the vertex buffer.
    VkMemoryRequirements vkVertexBufferMemRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vkVertexBuffer, &vkVertexBufferMemRequirements);

    // Define memory allocate info.
    VkMemoryAllocateInfo vkVertexBufferAllocInfo{};
    vkVertexBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkVertexBufferAllocInfo.allocationSize = vkVertexBufferMemRequirements.size;

    // Find a suitable memory type.
    uint32_t vkVertexBufferMemTypeIndex = UINT32_MAX;
    VkMemoryPropertyFlags vkVertexBufferMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((vkVertexBufferMemRequirements.memoryTypeBits & (1 << i)) &&
                (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & vkVertexBufferMemFlags) == vkVertexBufferMemFlags) {
            vkVertexBufferMemTypeIndex = i;
            break;
        }
    }
    vkVertexBufferAllocInfo.memoryTypeIndex = vkVertexBufferMemTypeIndex;

    // Allocate memory for the vertex buffer.
    VkDeviceMemory vkVertexBufferMemory;
    if (vkAllocateMemory(vkDevice, &vkVertexBufferAllocInfo, nullptr, &vkVertexBufferMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate memory for the vertex buffer!" << std::endl;
        abort();
    }

    // Bind the buffer to the allocated memory.
    vkBindBufferMemory(vkDevice, vkVertexBuffer, vkVertexBufferMemory, 0);

    // Copy our vertices to the allocated memory.
    void* vertexBufferMemoryData;
    vkMapMemory(vkDevice, vkVertexBufferMemory, 0, vertexBufferSize, 0, &vertexBufferMemoryData);
    memcpy(vertexBufferMemoryData, vertices.data(), (size_t) vertexBufferSize);
    vkUnmapMemory(vkDevice, vkVertexBufferMemory);

    // ==========================================================================
    //                    STEP 15: Import extension function
    // ==========================================================================
    // Function that belong to extensions are not available in the vulkan header
    // because the corresponding extension might be not enabled or not available
    // in the particular device. Instead we can retrieve a pointer to the
    // extension function via vkGetDeviceProcAddr() call.
    // For ray tracing we would need a couple of them.
    // ==========================================================================

    PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(vkDevice, "vkCreateAccelerationStructureNV"));
    PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(vkGetDeviceProcAddr(vkDevice, "vkDestroyAccelerationStructureNV"));
    PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(vkGetDeviceProcAddr(vkDevice, "vkBindAccelerationStructureMemoryNV"));
    PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(vkGetDeviceProcAddr(vkDevice, "vkGetAccelerationStructureHandleNV"));
    PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(vkGetDeviceProcAddr(vkDevice, "vkGetAccelerationStructureMemoryRequirementsNV"));
    PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(vkGetDeviceProcAddr(vkDevice, "vkCmdBuildAccelerationStructureNV"));
    PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(vkGetDeviceProcAddr(vkDevice, "vkCreateRayTracingPipelinesNV"));
    PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(vkGetDeviceProcAddr(vkDevice, "vkGetRayTracingShaderGroupHandlesNV"));
    PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV = reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(vkDevice, "vkCmdTraceRaysNV"));

    // ==========================================================================
    //                    STEP 16: Create a BLAS
    // ==========================================================================
    // Bottom level acceleration structure describes geometry of an object
    // regardles to its position in the world space.
    // So each unique type of objects is described by its own BLAS.
    // ==========================================================================

    // First we need to describe geometry of the object.
    // Geometry refers to the vertex buffer and could be either indexed or not indexed.
    // We use not indexed geometry for simplicity.
    VkGeometryNV geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    geometry.geometry.triangles.vertexData = vkVertexBuffer;
    geometry.geometry.triangles.vertexOffset = 0;
    geometry.geometry.triangles.vertexCount = static_cast<uint32_t>(vertices.size());
    geometry.geometry.triangles.vertexStride = sizeof(glm::vec3);
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_NV;
    geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
    geometry.geometry.triangles.transformOffset = 0;
    geometry.geometry.aabbs = {};
    geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

    // Fill in acceleration structure info.
    // For blas we provide geometry and ignore instances.
    VkAccelerationStructureInfoNV blasInfo{};
    blasInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    blasInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    blasInfo.instanceCount = 0;
    blasInfo.geometryCount = 1;
    blasInfo.pGeometries = &geometry;

    // Acceleration structure create info.
    VkAccelerationStructureCreateInfoNV blasCreateInfo{};
    blasCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    blasCreateInfo.info = blasInfo;
    VkAccelerationStructureNV vkBottomLevelAccelerationStructure;
    if (vkCreateAccelerationStructureNV(vkDevice, &blasCreateInfo, nullptr, &vkBottomLevelAccelerationStructure) != VK_SUCCESS) {
        std::cerr << "Failed to create a bottom level acceleration structure!" << std::endl;
        abort();
    }

    // Get memory requirements.
    VkAccelerationStructureMemoryRequirementsInfoNV blasMemoryRequirementsInfo{};
    blasMemoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    blasMemoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    blasMemoryRequirementsInfo.accelerationStructure = vkBottomLevelAccelerationStructure;
    VkMemoryRequirements2 blasMemoryRequirements2{};
    blasMemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    vkGetAccelerationStructureMemoryRequirementsNV(vkDevice, &blasMemoryRequirementsInfo, &blasMemoryRequirements2);

    // Find a suitable memory type.
    VkMemoryAllocateInfo blasMemoryAllocateInfo{};
    blasMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    blasMemoryAllocateInfo.allocationSize = blasMemoryRequirements2.memoryRequirements.size;
    uint32_t colorImageMemoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((blasMemoryRequirements2.memoryRequirements.memoryTypeBits & (1 << i)) && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            colorImageMemoryTypeIndex = i;
            break;
        }
    }
    blasMemoryAllocateInfo.memoryTypeIndex = colorImageMemoryTypeIndex;

    // Allocate memory for the acceleration structure.
    VkDeviceMemory vkBlasMemory;
    if (vkAllocateMemory(vkDevice, &blasMemoryAllocateInfo, nullptr, &vkBlasMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate memory!" << std::endl;
        abort();
    }

    // Bind bottom level acceleration structure to the memory.
    VkBindAccelerationStructureMemoryInfoNV blasMemoryInfo{};
    blasMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    blasMemoryInfo.accelerationStructure = vkBottomLevelAccelerationStructure;
    blasMemoryInfo.memory = vkBlasMemory;
    if (vkBindAccelerationStructureMemoryNV(vkDevice, 1, &blasMemoryInfo) != VK_SUCCESS) {
        std::cerr << "Failed to bind bottom level acceleration structure memory!" << std::endl;
        abort();
    }

    // Get acceleration structure handle.
    uint64_t vkBlasHandle;
    if (vkGetAccelerationStructureHandleNV(vkDevice, vkBottomLevelAccelerationStructure, sizeof(uint64_t), &vkBlasHandle) != VK_SUCCESS) {
        std::cerr << "Failed to get bottom level acceleration structure handle!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                    STEP 16: Create a TLAS
    // ==========================================================================
    // Top level acceleration structure refers to a blas and create a couple of
    // instances of the same geometry having its own transformation.
    // So BLAS defines pure geometry and TLAS applies transformation on top.
    // Therefore one geometry may be reused several times in different positions.
    // The geometry instance should be placed into an "instance buffer", so
    // it is accessible by the graphic card in the build stage.
    // ==========================================================================

    // -----------------------------
    // 1: Create a geometry instance
    // -----------------------------

    // Transformation that we want to apply to the created geometry.
    // So far it is just a unit matrix.
    VkTransformMatrixKHR transform = {
        {
            { 1.0f, 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f, 0.0f },
        }
    };

    // Create an instance of BLAS having the given transformation.
    VkAccelerationStructureInstanceKHR geometryInstance{};
    geometryInstance.transform = transform;
    geometryInstance.instanceCustomIndex = 0;
    geometryInstance.mask = 0xff;
    geometryInstance.instanceShaderBindingTableRecordOffset = 0;
    geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
    geometryInstance.accelerationStructureReference = vkBlasHandle;

    // Describe an intance buffer.
    VkBufferCreateInfo vkInstanceBufferInfo{};
    vkInstanceBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkInstanceBufferInfo.size = sizeof(VkAccelerationStructureInstanceKHR);
    vkInstanceBufferInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
    vkInstanceBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // ----------------------------
    // 2: Create an instance buffer
    // ----------------------------

    // Create an instance buffer.
    VkBuffer vkInstanceBufferHandle;
    if (vkCreateBuffer(vkDevice, &vkInstanceBufferInfo, nullptr, &vkInstanceBufferHandle) != VK_SUCCESS) {
        std::cerr << "Failed to create an instance buffer!" << std::endl;
        abort();
    }

    // Retrieve memory requirements for the instance buffer.
    VkMemoryRequirements vkInstanceBufferMemRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vkInstanceBufferHandle, &vkInstanceBufferMemRequirements);

    // Define memory allocate info.
    VkMemoryAllocateInfo vkInstanceBufferAllocInfo{};
    vkInstanceBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkInstanceBufferAllocInfo.allocationSize = vkInstanceBufferMemRequirements.size;

    // Find a suitable memory type.
    VkMemoryPropertyFlags vkInstanceBufferMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    uint32_t instanceBufferMemoryTypeInex = UINT32_MAX;
    for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((vkInstanceBufferMemRequirements.memoryTypeBits & (1 << i)) && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & vkInstanceBufferMemoryFlags) == vkInstanceBufferMemoryFlags) {
            instanceBufferMemoryTypeInex = i;
            break;
        }
    }
    vkInstanceBufferAllocInfo.memoryTypeIndex = instanceBufferMemoryTypeInex;

    // Allocate memory for the instance buffer.
    VkDeviceMemory vkInstanceBufferMemory;
    if (vkAllocateMemory(vkDevice, &vkInstanceBufferAllocInfo, nullptr, &vkInstanceBufferMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate memory for the instance buffer!" << std::endl;
        abort();
    }

    // Bind the buffer to the allocated memory.
    vkBindBufferMemory(vkDevice, vkInstanceBufferHandle, vkInstanceBufferMemory, 0);

    // Copy our geometry to the allocated memory.
    void* instanceBufferMemoryData;
    vkMapMemory(vkDevice, vkInstanceBufferMemory, 0, sizeof(geometryInstance), 0, &instanceBufferMemoryData);
    memcpy(instanceBufferMemoryData, &geometryInstance, sizeof(geometryInstance));
    vkUnmapMemory(vkDevice, vkInstanceBufferMemory);

    // --------------
    // 3: Create TLAS
    // --------------

    // Fill in acceleration structure info.
    // For tlas we provide intances and ignore geometry.
    VkAccelerationStructureInfoNV tlasInfo{};
    tlasInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    tlasInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    tlasInfo.instanceCount = 1;
    tlasInfo.geometryCount = 0;

    // Acceleration structure create info.
    VkAccelerationStructureCreateInfoNV tlasCreateInfo{};
    tlasCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    tlasCreateInfo.info = tlasInfo;
    VkAccelerationStructureNV vkTopLevelAccelerationStructure;
    if (vkCreateAccelerationStructureNV(vkDevice, &tlasCreateInfo, nullptr, &vkTopLevelAccelerationStructure) != VK_SUCCESS) {
        std::cerr << "Failed to create a top level acceleration structure!" << std::endl;
        abort();
    }

    // Get memory requirements.
    VkAccelerationStructureMemoryRequirementsInfoNV tlasMemoryRequirementsInfo{};
    tlasMemoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    tlasMemoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    tlasMemoryRequirementsInfo.accelerationStructure = vkTopLevelAccelerationStructure;
    VkMemoryRequirements2 tlasMemoryRequirements2{};
    tlasMemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    vkGetAccelerationStructureMemoryRequirementsNV(vkDevice, &tlasMemoryRequirementsInfo, &tlasMemoryRequirements2);

    // Find a suitable memory type.
    VkMemoryAllocateInfo tlasMemoryAllocateInfo{};
    tlasMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    tlasMemoryAllocateInfo.allocationSize = tlasMemoryRequirements2.memoryRequirements.size;
    uint32_t tlasMemoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((tlasMemoryRequirements2.memoryRequirements.memoryTypeBits & (1 << i)) && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            tlasMemoryTypeIndex = i;
            break;
        }
    }
    tlasMemoryAllocateInfo.memoryTypeIndex = tlasMemoryTypeIndex;

    // Allocate memory for the acceleration structure.
    VkDeviceMemory vkTlasMemory;
    if (vkAllocateMemory(vkDevice, &tlasMemoryAllocateInfo, nullptr, &vkTlasMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate memory!" << std::endl;
        abort();
    }

    // Bind top level acceleration structure to the memory.
    VkBindAccelerationStructureMemoryInfoNV tlasMemoryInfo{};
    tlasMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    tlasMemoryInfo.accelerationStructure = vkTopLevelAccelerationStructure;
    tlasMemoryInfo.memory = vkTlasMemory;
    if (vkBindAccelerationStructureMemoryNV(vkDevice, 1, &tlasMemoryInfo) != VK_SUCCESS) {
        std::cerr << "Failed to bind top level acceleration structure memory!" << std::endl;
        abort();
    }

    // Get acceleration structure handle.
    uint64_t vkTlasHandle;
    if (vkGetAccelerationStructureHandleNV(vkDevice, vkTopLevelAccelerationStructure, sizeof(uint64_t), &vkTlasHandle) != VK_SUCCESS) {
        std::cerr << "Failed to get top level acceleration structure handle!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                    STEP 17: Create a scratch buffer
    // ==========================================================================
    // Acceleration structures should be build on the graphical card before they
    // are used for ray tracing. The build process requires some additional
    // memory we have to allocate. This memory is called a scratch buffer.
    // ==========================================================================

    // Collect memory requirements for BLAS and TLAS.
    VkAccelerationStructureMemoryRequirementsInfoNV asMemoryRequirementsInfo{};
    asMemoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    asMemoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
    VkMemoryRequirements2 memReqBottomLevelAS{};
    memReqBottomLevelAS.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    asMemoryRequirementsInfo.accelerationStructure = vkBottomLevelAccelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(vkDevice, &asMemoryRequirementsInfo, &memReqBottomLevelAS);
    VkMemoryRequirements2 memReqTopLevelAS{};
    memReqTopLevelAS.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    asMemoryRequirementsInfo.accelerationStructure = vkTopLevelAccelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(vkDevice, &asMemoryRequirementsInfo, &memReqTopLevelAS);

    // Make the scratch buffer enough big for both of them.
    const VkDeviceSize scratchBufferSize = std::max(memReqBottomLevelAS.memoryRequirements.size, memReqTopLevelAS.memoryRequirements.size);

    // Describe a buffer.
    VkBufferCreateInfo vkScratchBufferInfo{};
    vkScratchBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkScratchBufferInfo.size = scratchBufferSize;
    vkScratchBufferInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
    vkScratchBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create a buffer.
    VkBuffer vkScratchBufferHandle;
    if (vkCreateBuffer(vkDevice, &vkScratchBufferInfo, nullptr, &vkScratchBufferHandle) != VK_SUCCESS) {
        std::cerr << "Failed to create a scratch buffer!" << std::endl;
        abort();
    }

    // Retrieve memory requirements for the vertex buffer.
    VkMemoryRequirements vkScratchBufferMemRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vkScratchBufferHandle, &vkScratchBufferMemRequirements);

    // Define memory allocate info.
    VkMemoryAllocateInfo vkScratchBufferAllocInfo{};
    vkScratchBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkScratchBufferAllocInfo.allocationSize = vkScratchBufferMemRequirements.size;

    // Find a suitable memory type.
    uint32_t scratchBufferMemoryTypeInex = UINT32_MAX;
    for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((vkScratchBufferMemRequirements.memoryTypeBits & (1 << i)) && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            scratchBufferMemoryTypeInex = i;
            break;
        }
    }
    vkScratchBufferAllocInfo.memoryTypeIndex = scratchBufferMemoryTypeInex;

    // Allocate memory for the scratch buffer.
    VkDeviceMemory vkScratchBufferMemory;
    if (vkAllocateMemory(vkDevice, &vkScratchBufferAllocInfo, nullptr, &vkScratchBufferMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate memory for the scratch buffer!" << std::endl;
        abort();
    }

    // Bind the buffer to the allocated memory.
    vkBindBufferMemory(vkDevice, vkScratchBufferHandle, vkScratchBufferMemory, 0);

    // ==========================================================================
    //                    STEP 18: Build acceleration structures
    // ==========================================================================
    // Acceleration structures should be built on the graphical card before
    // they are used first time. To do this we need to create a temporary
    // command pool and execute build commands.
    // ==========================================================================

    // Pick a graphics queue.
    VkQueue vkGraphicsQueue;
    vkGetDeviceQueue(vkDevice, queueFamilyIndices.graphicsFamily.value(), 0, &vkGraphicsQueue);

    // Create a command pool.
    VkCommandPoolCreateInfo vkBuildASPoolInfo{};
    vkBuildASPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vkBuildASPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    vkBuildASPoolInfo.flags = 0;
    VkCommandPool vkBuildASCommandPool;
    if (vkCreateCommandPool(vkDevice, &vkBuildASPoolInfo, nullptr, &vkBuildASCommandPool) != VK_SUCCESS) {
        std::cerr << "Failed to create a command pool!" << std::endl;
        abort();
    }

    // Create one command buffer.
    VkCommandBufferAllocateInfo vkBuildASCmdBufAllocateInfo{};
    vkBuildASCmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vkBuildASCmdBufAllocateInfo.commandPool = vkBuildASCommandPool;
    vkBuildASCmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkBuildASCmdBufAllocateInfo.commandBufferCount = 1;
    VkCommandBuffer vkBuildASCmdBuffer;
    if (vkAllocateCommandBuffers(vkDevice, &vkBuildASCmdBufAllocateInfo, &vkBuildASCmdBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to allocate command buffers!" << std::endl;
        abort();
    }

    // Begin command execution.
    VkCommandBufferBeginInfo vkBuildASCmdBufferBeginInfo {};
    vkBuildASCmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(vkBuildASCmdBuffer, &vkBuildASCmdBufferBeginInfo) != VK_SUCCESS) {
        std::cerr << "Failed to begin a command buffer!" << std::endl;
        abort();
    }

    // Build BLAS
    VkAccelerationStructureInfoNV buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;
    vkCmdBuildAccelerationStructureNV(
                vkBuildASCmdBuffer,
                &buildInfo,
                VK_NULL_HANDLE,
                0,
                VK_FALSE,
                vkBottomLevelAccelerationStructure,
                VK_NULL_HANDLE,
                vkScratchBufferHandle,
                0);

    // Wait until the build finishes because we use the same scratch buffer for both structures.
    VkMemoryBarrier memoryBarrier {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    vkCmdPipelineBarrier(vkBuildASCmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

    // Build TLAS.
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    buildInfo.pGeometries = 0;
    buildInfo.geometryCount = 0;
    buildInfo.instanceCount = 1;
    vkCmdBuildAccelerationStructureNV(
                vkBuildASCmdBuffer,
                &buildInfo,
                vkInstanceBufferHandle,
                0,
                VK_FALSE,
                vkTopLevelAccelerationStructure,
                VK_NULL_HANDLE,
                vkScratchBufferHandle,
                0);

    // End command execution.
    if (vkEndCommandBuffer(vkBuildASCmdBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to end a command buffer!" << std::endl;
        abort();
    }

    // Create fence that will suspend the execution until GPU finishes.
    VkFenceCreateInfo vkBuildASFenceInfo {};
    vkBuildASFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkBuildASFenceInfo.flags = 0;
    VkFence vkBuildASFence;
    if (vkCreateFence(vkDevice, &vkBuildASFenceInfo, nullptr, &vkBuildASFence) != VK_SUCCESS) {
        std::cerr << "Failed to create a fence!" << std::endl;
        abort();
    }

    // Submit the command buffer to the queue.
    VkSubmitInfo vkBuildASsubmitInfo{};
    vkBuildASsubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkBuildASsubmitInfo.commandBufferCount = 1;
    vkBuildASsubmitInfo.pCommandBuffers = &vkBuildASCmdBuffer;
    vkQueueSubmit(vkGraphicsQueue, 1, &vkBuildASsubmitInfo, vkBuildASFence);

    // Wait for the fence to signal that the command buffer has finished executing.
    if (vkWaitForFences(vkDevice, 1, &vkBuildASFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        std::cerr << "Failed to wait for a fence!" << std::endl;
        abort();
    }

    // Clean up the command pool and the fence.
    vkDestroyFence(vkDevice, vkBuildASFence, nullptr);
    vkFreeCommandBuffers(vkDevice, vkBuildASCommandPool, 1, &vkBuildASCmdBuffer);
    vkDestroyCommandPool(vkDevice, vkBuildASCommandPool, nullptr);

    // Destroy the scratch buffer and free the memory.
    vkFreeMemory(vkDevice, vkScratchBufferMemory, nullptr);
    vkDestroyBuffer(vkDevice, vkScratchBufferHandle, nullptr);

    // ==========================================================================
    //                    STEP 19: Create a storage image
    // ==========================================================================
    // Ray tracing pipeline does not contain usual color attachments, so
    // the rendering writes color output into an image and then this image is copied
    // to the framebuffers. The image we will use is called a storage image.
    // ==========================================================================

    // Description of a storage image.
    VkImageCreateInfo vkStorageImageInfo{};
    vkStorageImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkStorageImageInfo.imageType = VK_IMAGE_TYPE_2D;
    vkStorageImageInfo.extent.width = vkSelectedExtent.width;
    vkStorageImageInfo.extent.height = vkSelectedExtent.height;
    vkStorageImageInfo.extent.depth = 1;
    vkStorageImageInfo.mipLevels = 1;
    vkStorageImageInfo.arrayLayers = 1;
    vkStorageImageInfo.format = vkSelectedFormat.format;
    vkStorageImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    vkStorageImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkStorageImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    vkStorageImageInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    vkStorageImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create a storage image.
    VkImage vkStorageImage;
    if (vkCreateImage(vkDevice, &vkStorageImageInfo, nullptr, &vkStorageImage) != VK_SUCCESS) {
        std::cerr << "Failed to create an image!" << std::endl;
        abort();
    }

    // Get memory requirements.
    VkMemoryRequirements vkStorageImageMemRequirements;
    vkGetImageMemoryRequirements(vkDevice, vkStorageImage, &vkStorageImageMemRequirements);

    // Find suitable memory type.
    VkMemoryAllocateInfo vkStorageImageAllocInfo{};
    vkStorageImageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkStorageImageAllocInfo.allocationSize = vkStorageImageMemRequirements.size;
    uint32_t storageImageMemoryTypeInex = UINT32_MAX;
    for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((vkStorageImageMemRequirements.memoryTypeBits & (1 << i)) && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            storageImageMemoryTypeInex = i;
            break;
        }
    }
    vkStorageImageAllocInfo.memoryTypeIndex = storageImageMemoryTypeInex;

    // Allocate memory for the storage image.
    VkDeviceMemory vkStorageImageMemory;
    if (vkAllocateMemory(vkDevice, &vkStorageImageAllocInfo, nullptr, &vkStorageImageMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate image memory!" << std::endl;
        abort();
    }

    // Bind the image to the memory.
    vkBindImageMemory(vkDevice, vkStorageImage, vkStorageImageMemory, 0);

    // Describe an image view.
    VkImageViewCreateInfo vkStorageImageViewInfo{};
    vkStorageImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vkStorageImageViewInfo.image = vkStorageImage;
    vkStorageImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vkStorageImageViewInfo.format = vkSelectedFormat.format;
    vkStorageImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkStorageImageViewInfo.subresourceRange.baseMipLevel = 0;
    vkStorageImageViewInfo.subresourceRange.levelCount = 1;
    vkStorageImageViewInfo.subresourceRange.baseArrayLayer = 0;
    vkStorageImageViewInfo.subresourceRange.layerCount = 1;

    // Create an image view.
    VkImageView vkStorageImageView;
    if (vkCreateImageView(vkDevice, &vkStorageImageViewInfo, nullptr, &vkStorageImageView) != VK_SUCCESS) {
        std::cerr << "Failed to create texture image view!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                    STEP 19: Change image layout
    // ==========================================================================
    // We have to change the storage image layout in order to proceed.
    // Vulkan allows to do this via image memory barrier, so we have to create
    // a temporary command buffer again and execute vkCmdPipelineBarrier().
    //
    // See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#synchronization-image-layout-transitions
    // ==========================================================================

    // Create a command pool.
    VkCommandPoolCreateInfo vkSetImageLayoutPoolInfo{};
    vkSetImageLayoutPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vkSetImageLayoutPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    vkSetImageLayoutPoolInfo.flags = 0;
    VkCommandPool vkSetImageLayoutCommandPool;
    if (vkCreateCommandPool(vkDevice, &vkSetImageLayoutPoolInfo, nullptr, &vkSetImageLayoutCommandPool) != VK_SUCCESS) {
        std::cerr << "Failed to create a command pool!" << std::endl;
        abort();
    }

    // Create one command buffer.
    VkCommandBufferAllocateInfo vkSetImageLayoutCmdBufAllocateInfo{};
    vkSetImageLayoutCmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vkSetImageLayoutCmdBufAllocateInfo.commandPool = vkSetImageLayoutCommandPool;
    vkSetImageLayoutCmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkSetImageLayoutCmdBufAllocateInfo.commandBufferCount = 1;
    VkCommandBuffer vkSetImageLayoutCmdBuffer;
    if (vkAllocateCommandBuffers(vkDevice, &vkSetImageLayoutCmdBufAllocateInfo, &vkSetImageLayoutCmdBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to allocate command buffers!" << std::endl;
        abort();
    }

    // Begin command execution.
    VkCommandBufferBeginInfo vkSetImageLayoutCmdBufferBeginInfo {};
    vkSetImageLayoutCmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(vkSetImageLayoutCmdBuffer, &vkSetImageLayoutCmdBufferBeginInfo) != VK_SUCCESS) {
        std::cerr << "Failed to begin a command buffer!" << std::endl;
        abort();
    }

    // Change image layout.
    VkImageMemoryBarrier imageMemoryBarrier {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.image = vkStorageImage;
    imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    imageMemoryBarrier.srcAccessMask = 0;
    vkCmdPipelineBarrier(
        vkSetImageLayoutCmdBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);

    // End command execution.
    if (vkEndCommandBuffer(vkSetImageLayoutCmdBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to end a command buffer!" << std::endl;
        abort();
    }

    // Create fence that will suspend the execution until GPU finishes.
    VkFenceCreateInfo vkSetImageLayoutFenceInfo {};
    vkSetImageLayoutFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkSetImageLayoutFenceInfo.flags = 0;
    VkFence vkSetImageLayout;
    if (vkCreateFence(vkDevice, &vkSetImageLayoutFenceInfo, nullptr, &vkSetImageLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create a fence!" << std::endl;
        abort();
    }

    // Submit the command buffer to the queue.
    VkSubmitInfo vkSetImageLayoutsubmitInfo{};
    vkSetImageLayoutsubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSetImageLayoutsubmitInfo.commandBufferCount = 1;
    vkSetImageLayoutsubmitInfo.pCommandBuffers = &vkSetImageLayoutCmdBuffer;
    vkQueueSubmit(vkGraphicsQueue, 1, &vkSetImageLayoutsubmitInfo, vkSetImageLayout);

    // Wait for the fence to signal that the command buffer has finished executing.
    if (vkWaitForFences(vkDevice, 1, &vkSetImageLayout, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        std::cerr << "Failed to wait for a fence!" << std::endl;
        abort();
    }

    // Clean up the command pool and the fence.
    vkDestroyFence(vkDevice, vkSetImageLayout, nullptr);
    vkFreeCommandBuffers(vkDevice, vkSetImageLayoutCommandPool, 1, &vkSetImageLayoutCmdBuffer);
    vkDestroyCommandPool(vkDevice, vkSetImageLayoutCommandPool, nullptr);

    // ==========================================================================
    //                    STEP 20: Load shaders
    // ==========================================================================
    // The minimal example of the ray tracing requires three shaders:
    // - raygen, to generate a ray, trace it and write color output
    // - raymiss, to produce a color if the ray misses geometry
    // - rayhit, to produce a color if the ray hits geometry
    // ==========================================================================

    // ---------
    // 1: RayGen
    // ---------

    // Open file.
    std::ifstream raygenShaderFile("main.rgen.spv", std::ios::ate | std::ios::binary);
    if (!raygenShaderFile.is_open()) {
        std::cerr << "Raygen shader file not found!" << std::endl;
        abort();
    }

    // Calculate file size.
    size_t raygenFileSize = static_cast< size_t >(raygenShaderFile.tellg());

    // Jump to the beginning of the file.
    raygenShaderFile.seekg(0);

    // Read shader code.
    std::vector< char > raygenShaderBuffer(raygenFileSize);
    raygenShaderFile.read(raygenShaderBuffer.data(), raygenFileSize);

    // Close the file.
    raygenShaderFile.close();

    // Shader module creation info.
    VkShaderModuleCreateInfo vkRaygenShaderCreateInfo{};
    vkRaygenShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkRaygenShaderCreateInfo.codeSize = raygenShaderBuffer.size();
    vkRaygenShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(raygenShaderBuffer.data());

    // Create a shader module.
    VkShaderModule vkRaygenShaderModule;
    if (vkCreateShaderModule(vkDevice, &vkRaygenShaderCreateInfo, nullptr, &vkRaygenShaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create a shader!" << std::endl;
        abort();
    }

    // Create a pipeline stage for the shader.
    VkPipelineShaderStageCreateInfo vkRaygenShaderModuleCreateInfo{};
    vkRaygenShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vkRaygenShaderModuleCreateInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_NV;
    vkRaygenShaderModuleCreateInfo.module = vkRaygenShaderModule;
    vkRaygenShaderModuleCreateInfo.pName = "main";

    // ----------
    // 2: RayMiss
    // ----------

    // Open file.
    std::ifstream raymissShaderFile("main.rmiss.spv", std::ios::ate | std::ios::binary);
    if (!raymissShaderFile.is_open()) {
        std::cerr << "Raymiss shader file not found!" << std::endl;
        abort();
    }

    // Calculate file size.
    size_t raymissFileSize = static_cast< size_t >(raymissShaderFile.tellg());

    // Jump to the beginning of the file.
    raymissShaderFile.seekg(0);

    // Read shader code.
    std::vector< char > raymissShaderBuffer(raymissFileSize);
    raymissShaderFile.read(raymissShaderBuffer.data(), raymissFileSize);

    // Close the file.
    raymissShaderFile.close();

    // Shader module creation info.
    VkShaderModuleCreateInfo vkRaymissShaderCreateInfo{};
    vkRaymissShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkRaymissShaderCreateInfo.codeSize = raymissShaderBuffer.size();
    vkRaymissShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(raymissShaderBuffer.data());

    // Create a shader module.
    VkShaderModule vkRaymissShaderModule;
    if (vkCreateShaderModule(vkDevice, &vkRaymissShaderCreateInfo, nullptr, &vkRaymissShaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create a shader!" << std::endl;
        abort();
    }

    // Create a pipeline stage for the shader.
    VkPipelineShaderStageCreateInfo vkRaymissShaderModuleCreateInfo{};
    vkRaymissShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vkRaymissShaderModuleCreateInfo.stage = VK_SHADER_STAGE_MISS_BIT_NV;
    vkRaymissShaderModuleCreateInfo.module = vkRaymissShaderModule;
    vkRaymissShaderModuleCreateInfo.pName = "main";

    // ---------
    // 3: RayHit
    // ---------

    // Open file.
    std::ifstream rayhitShaderFile("main.rchit.spv", std::ios::ate | std::ios::binary);
    if (!rayhitShaderFile.is_open()) {
        std::cerr << "Rayhit shader file not found!" << std::endl;
        abort();
    }

    // Calculate file size.
    size_t rayhitFileSize = static_cast< size_t >(rayhitShaderFile.tellg());

    // Jump to the beginning of the file.
    rayhitShaderFile.seekg(0);

    // Read shader code.
    std::vector< char > rayhitShaderBuffer(rayhitFileSize);
    rayhitShaderFile.read(rayhitShaderBuffer.data(), rayhitFileSize);

    // Close the file.
    rayhitShaderFile.close();

    // Shader module creation info.
    VkShaderModuleCreateInfo vkRayhitShaderCreateInfo{};
    vkRayhitShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkRayhitShaderCreateInfo.codeSize = rayhitShaderBuffer.size();
    vkRayhitShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(rayhitShaderBuffer.data());

    // Create a shader module.
    VkShaderModule vkRayhitShaderModule;
    if (vkCreateShaderModule(vkDevice, &vkRayhitShaderCreateInfo, nullptr, &vkRayhitShaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create a shader!" << std::endl;
        abort();
    }

    // Create a pipeline stage for the shader.
    VkPipelineShaderStageCreateInfo vkRayhitShaderModuleCreateInfo{};
    vkRayhitShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vkRayhitShaderModuleCreateInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
    vkRayhitShaderModuleCreateInfo.module = vkRayhitShaderModule;
    vkRayhitShaderModuleCreateInfo.pName = "main";

    std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages {
        vkRaygenShaderModuleCreateInfo,
        vkRayhitShaderModuleCreateInfo,
        vkRaymissShaderModuleCreateInfo,
    };

    // ==========================================================================
    //                    STEP 21: Set up shader groups
    // ==========================================================================
    // Unlike other types of shaders, ray tracing ones do no have a strict order.
    // So we have to describe it creating shader groups.
    // ==========================================================================

    // Indices for the different ray tracing shader types used in this example and their total amount.
    constexpr const int INDEX_RAYGEN = 0;
    constexpr const int INDEX_CLOSEST_HIT = 1;
    constexpr const int INDEX_MISS = 2;

    constexpr const int NUM_SHADER_GROUPS = 3;

    // Declare ray tracing shader groups and initialize them with default values.
    std::array<VkRayTracingShaderGroupCreateInfoNV, NUM_SHADER_GROUPS> groups{};
    for (auto& group : groups) {
        group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
        group.generalShader = VK_SHADER_UNUSED_NV;
        group.closestHitShader = VK_SHADER_UNUSED_NV;
        group.anyHitShader = VK_SHADER_UNUSED_NV;
        group.intersectionShader = VK_SHADER_UNUSED_NV;
    }

    // Links shaders and types to ray tracing shader groups.
    groups[INDEX_RAYGEN].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_RAYGEN].generalShader = INDEX_RAYGEN;

    groups[INDEX_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_MISS].generalShader = INDEX_MISS;

    groups[INDEX_CLOSEST_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
    groups[INDEX_CLOSEST_HIT].generalShader = VK_SHADER_UNUSED_NV;
    groups[INDEX_CLOSEST_HIT].closestHitShader = INDEX_CLOSEST_HIT;

    // ==========================================================================
    //                    STEP 22: Create pipeline layout
    // ==========================================================================
    // Pipeline layout defines uniforms available in shaders while rendering.
    // The layout is required to create a pipeline but it contains no data.
    // After that particular values of uniforms should be written.
    // ==========================================================================

    // Binding of TLAS used by the ray gen shader to initiate ray tracing.
    VkDescriptorSetLayoutBinding vkAccelerationStructureLayoutBinding{};
    vkAccelerationStructureLayoutBinding.binding = 0;
    vkAccelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
    vkAccelerationStructureLayoutBinding.descriptorCount = 1;
    vkAccelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

    // Binding of a storage image to save output color.
    VkDescriptorSetLayoutBinding vkStorageImageLayoutBinding{};
    vkStorageImageLayoutBinding.binding = 1;
    vkStorageImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    vkStorageImageLayoutBinding.descriptorCount = 1;
    vkStorageImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

    // Binding of a uniform buffer that contains view and projection matrixes.
    VkDescriptorSetLayoutBinding vkUniformBufferBinding{};
    vkUniformBufferBinding.binding = 2;
    vkUniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vkUniformBufferBinding.descriptorCount = 1;
    vkUniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

    // Create descriptor set layout.
    std::vector<VkDescriptorSetLayoutBinding> bindings({
        vkAccelerationStructureLayoutBinding,
        vkStorageImageLayoutBinding,
        vkUniformBufferBinding
    });
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    descriptorSetLayoutInfo.pBindings = bindings.data();
    VkDescriptorSetLayout vkDescriptorSetLayout;
    if (vkCreateDescriptorSetLayout(vkDevice, &descriptorSetLayoutInfo, nullptr, &vkDescriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create a descriptor set layout!" << std::endl;
        abort();
    }

    // Create pipeline layout.
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &vkDescriptorSetLayout;
    VkPipelineLayout vkPipelineLayout;
    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create a pipeline layout!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                    STEP 23: Create a pipeline
    // ==========================================================================
    // Ray tracing pipeline binds together shaders, shader groups and uniforms.
    // ==========================================================================

    VkRayTracingPipelineCreateInfoNV vkRayTracingPipelineInfo{};
    vkRayTracingPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
    vkRayTracingPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    vkRayTracingPipelineInfo.pStages = shaderStages.data();
    vkRayTracingPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    vkRayTracingPipelineInfo.pGroups = groups.data();
    vkRayTracingPipelineInfo.maxRecursionDepth = 1;
    vkRayTracingPipelineInfo.layout = vkPipelineLayout;
    VkPipeline vkPipeline;
    if (vkCreateRayTracingPipelinesNV(vkDevice, VK_NULL_HANDLE, 1, &vkRayTracingPipelineInfo, nullptr, &vkPipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create a pipeline!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                STEP 24: Create a shader binding table
    // ==========================================================================
    // The Shader Binding Table consists of a set of shader function handles and
    // embedded parameters for these functions. The shaders in the table
    // are executed depending on whether or not a geometry was hit by a ray,
    // and which geometry was hit.
    //
    // See https://www.willusher.io/graphics/2019/11/20/the-sbt-three-ways
    // ==========================================================================

    // Size of the shader binding table.
    const uint32_t shaderBindingTableSize = rayTracingProperties.shaderGroupBaseAlignment * NUM_SHADER_GROUPS;

    // Describe a buffer.
    VkBufferCreateInfo vkSbtBufferInfo{};
    vkSbtBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkSbtBufferInfo.size = shaderBindingTableSize;
    vkSbtBufferInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
    vkSbtBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create a buffer.
    VkBuffer vkShaderBindingTable;
    if (vkCreateBuffer(vkDevice, &vkSbtBufferInfo, nullptr, &vkShaderBindingTable) != VK_SUCCESS) {
        std::cerr << "Failed to create a shader binding table buffer!" << std::endl;
        abort();
    }

    // Retrieve memory requirements for the buffer.
    VkMemoryRequirements vkSbtBufferMemRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vkShaderBindingTable, &vkSbtBufferMemRequirements);

    // Find suitable memory type.
    VkMemoryAllocateInfo vkSbtBufferAllocInfo{};
    vkSbtBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkSbtBufferAllocInfo.allocationSize = vkSbtBufferMemRequirements.size;
    uint32_t sbtMemoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((vkSbtBufferMemRequirements.memoryTypeBits & (1 << i)) && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            sbtMemoryTypeIndex = i;
            break;
        }
    }
    vkSbtBufferAllocInfo.memoryTypeIndex = sbtMemoryTypeIndex;

    // Allocate memory for the shader binding table.
    VkDeviceMemory vkShaderBindingTableMemory;
    if (vkAllocateMemory(vkDevice, &vkSbtBufferAllocInfo, nullptr, &vkShaderBindingTableMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate memory for the shader binding table!" << std::endl;
        abort();
    }

    // Bind the buffer to the allocated memory.
    vkBindBufferMemory(vkDevice, vkShaderBindingTable, vkShaderBindingTableMemory, 0);

    // Retrieve shader group handles.
    auto shaderHandleStorage = new uint8_t[shaderBindingTableSize];
    if (vkGetRayTracingShaderGroupHandlesNV(vkDevice, vkPipeline, 0, NUM_SHADER_GROUPS, shaderBindingTableSize, shaderHandleStorage) != VK_SUCCESS) {
        std::cerr << "Failed to getshader group handles!" << std::endl;
        abort();
    }

    // Copy shader group handles to the buffer.
    void* sbtBufferMemoryData;
    vkMapMemory(vkDevice, vkShaderBindingTableMemory, 0, shaderBindingTableSize, 0, &sbtBufferMemoryData);
    auto* sbtData = static_cast<uint8_t*>(sbtBufferMemoryData);
    for(uint32_t group = 0; group < NUM_SHADER_GROUPS; group++) {
        memcpy(sbtData, shaderHandleStorage + group * rayTracingProperties.shaderGroupHandleSize, rayTracingProperties.shaderGroupHandleSize);
        sbtData += rayTracingProperties.shaderGroupBaseAlignment;
    }
    vkUnmapMemory(vkDevice, vkShaderBindingTableMemory);

    // ==========================================================================
    //                      STEP 25: Create uniform buffers
    // ==========================================================================
    // Uniform buffer contains structures that are provided to shaders
    // as uniform variables. In our case we should provide a view and a projection
    // matrices in order to generate rays. To avoid unneeded calculations on GPU,
    // the matrices are already inversed.
    // ==========================================================================

    // Structure that we want to provide to the vertext shader.
    struct UniformBufferObject
    {
        glm::mat4 viewInv;
        glm::mat4 projInv;
    };

    // Get size of the uniform buffer.
    VkDeviceSize vkUniformBufferSize = sizeof(UniformBufferObject);

    // Create one uniform buffer per swap chain image.
    // Describe a buffer.
    VkBufferCreateInfo vkUniformBufferInfo{};
    vkUniformBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkUniformBufferInfo.size = vkUniformBufferSize;
    vkUniformBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    vkUniformBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create a buffer.
    VkBuffer vkUniformBuffer;
    if (vkCreateBuffer(vkDevice, &vkUniformBufferInfo, nullptr, &vkUniformBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to create a uniform buffer!" << std::endl;
        abort();
    }

    // Retrieve memory requirements for the uniform buffer.
    VkMemoryRequirements vkUniformBufferMemRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vkUniformBuffer, &vkUniformBufferMemRequirements);

    // Select suitable memory type.
    VkMemoryAllocateInfo vkUniformBufferAllocInfo{};
    vkUniformBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkUniformBufferAllocInfo.allocationSize = vkUniformBufferMemRequirements.size;
    uint32_t uniformBufferMemTypeIndex = UINT32_MAX;
    VkMemoryPropertyFlags vkUniformBufferMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((vkUniformBufferMemRequirements.memoryTypeBits & (1 << i)) && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & vkUniformBufferMemFlags) == vkUniformBufferMemFlags) {
            uniformBufferMemTypeIndex = i;
            break;
        }
    }
    vkUniformBufferAllocInfo.memoryTypeIndex = uniformBufferMemTypeIndex;

    // Allocate memory for the vertex buffer.
    VkDeviceMemory vkUniformBufferMemory;
    if (vkAllocateMemory(vkDevice, &vkUniformBufferAllocInfo, nullptr, &vkUniformBufferMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate buffer memory!" << std::endl;
        abort();
    }

    // Bind the buffer to the allocated memory.
    vkBindBufferMemory(vkDevice, vkUniformBuffer, vkUniformBufferMemory, 0);

    // Fill in the uniform buffer object.
    UniformBufferObject ubo{};
    ubo.viewInv = glm::inverse(glm::lookAt(glm::vec3(2.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    float aspectRatio = static_cast< float >(vkSelectedExtent.width) / vkSelectedExtent.height;
    ubo.projInv = glm::inverse(glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f));

    // Write the uniform buffer object data.
    void* data;
    vkMapMemory(vkDevice, vkUniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(vkDevice, vkUniformBufferMemory);

    // ==========================================================================
    //                      STEP 26: Write descriptor sets
    // ==========================================================================
    // In order to provide particular values of uniforms to shaders we should
    // write descriptor sets.
    // ==========================================================================

    // Create a descriptor pool.
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    };
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
    descriptorPoolCreateInfo.maxSets = 1;
    VkDescriptorPool vkDescriptorPool;
    if (vkCreateDescriptorPool(vkDevice, &descriptorPoolCreateInfo, nullptr, &vkDescriptorPool) != VK_SUCCESS) {
        std::cerr << "Failed to create a descriptor pool!" << std::endl;
        abort();
    }

    // Allocate descriptior set that corresponds to the defined layout.
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = vkDescriptorPool;
    descriptorSetAllocateInfo.pSetLayouts = &vkDescriptorSetLayout;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    VkDescriptorSet vkDescriptorSet;
    if (vkAllocateDescriptorSets(vkDevice, &descriptorSetAllocateInfo, &vkDescriptorSet) != VK_SUCCESS) {
        std::cerr << "Failed to allocate descriptor sets!" << std::endl;
        abort();
    }

    // Top level acceleration structure.
    VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo{};
    descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
    descriptorAccelerationStructureInfo.pAccelerationStructures = &vkTopLevelAccelerationStructure;
    VkWriteDescriptorSet accelerationStructureWrite{};
    accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
    accelerationStructureWrite.dstSet = vkDescriptorSet;
    accelerationStructureWrite.dstBinding = 0;
    accelerationStructureWrite.descriptorCount = 1;
    accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

    // Storage image.
    VkDescriptorImageInfo storageImageDescriptor{};
    storageImageDescriptor.imageView = vkStorageImageView;
    storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    VkWriteDescriptorSet storageImageWrite {};
    storageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    storageImageWrite.dstSet = vkDescriptorSet;
    storageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    storageImageWrite.dstBinding = 1;
    storageImageWrite.pImageInfo = &storageImageDescriptor;
    storageImageWrite.descriptorCount = 1;

    // Uniform buffer providing view and projection matrices.
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = vkUniformBuffer;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = vkUniformBufferSize;
    VkWriteDescriptorSet uniformBufferWrite {};
    uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uniformBufferWrite.dstSet = vkDescriptorSet;
    uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferWrite.dstBinding = 2;
    uniformBufferWrite.pBufferInfo = &uniformBufferInfo;
    uniformBufferWrite.descriptorCount = 1;

    // Write descriptor sets.
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        accelerationStructureWrite,
        storageImageWrite,
        uniformBufferWrite
    };
    vkUpdateDescriptorSets(vkDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);

    // ==========================================================================
    //                    STEP 27: Create command buffers
    // ==========================================================================
    // Command buffers describe a set of rendering commands submitted to Vulkan.
    // We need to have one buffer per each image in the swap chain.
    // Command buffers are taken from the command pool.
    // ==========================================================================

    // Describe a command pool.
    VkCommandPoolCreateInfo vkPoolInfo{};
    vkPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vkPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    vkPoolInfo.flags = 0;

    // Create a command pool.
    VkCommandPool vkCommandPool;
    if (vkCreateCommandPool(vkDevice, &vkPoolInfo, nullptr, &vkCommandPool) != VK_SUCCESS) {
        std::cerr << "Failed to create a command pool!" << std::endl;
        abort();
    }

    // Create a vector for all command buffers.
    std::vector< VkCommandBuffer > vkCommandBuffers;
    vkCommandBuffers.resize(vkSwapChainImageViews.size());

    // Describe a command buffer allocate info.
    VkCommandBufferAllocateInfo vkAllocInfo{};
    vkAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vkAllocInfo.commandPool = vkCommandPool;
    vkAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocInfo.commandBufferCount = static_cast< uint32_t >(vkCommandBuffers.size());

    // Allocate command buffers.
    if (vkAllocateCommandBuffers(vkDevice, &vkAllocInfo, vkCommandBuffers.data()) != VK_SUCCESS) {
        std::cerr << "Failed to create command buffers" << std::endl;
        abort();
    }

    // Describe a rendering sequence for each command buffer.
    for (size_t i = 0; i < vkCommandBuffers.size(); i++) {
        // Start adding commands into the buffer.
        VkCommandBufferBeginInfo vkBeginInfo{};
        vkBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginInfo.flags = 0;
        vkBeginInfo.pInheritanceInfo = nullptr;
        if (vkBeginCommandBuffer(vkCommandBuffers[i], &vkBeginInfo) != VK_SUCCESS) {
            std::cerr << "Failed to start command buffer recording" << std::endl;
            abort();
        }

        // Bind the ray tracing pipeline.
        vkCmdBindPipeline(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, vkPipeline);

        // Bind descriptor sets.
        vkCmdBindDescriptorSets(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, vkPipelineLayout, 0, 1, &vkDescriptorSet, 0, 0);

        // Calculate shader binding offsets, which is pretty straight forward in our example.
        VkDeviceSize bindingOffsetRayGenShader = rayTracingProperties.shaderGroupBaseAlignment * INDEX_RAYGEN;
        VkDeviceSize bindingOffsetMissShader = rayTracingProperties.shaderGroupBaseAlignment * INDEX_MISS;
        VkDeviceSize bindingOffsetHitShader = rayTracingProperties.shaderGroupBaseAlignment * INDEX_CLOSEST_HIT;
        VkDeviceSize bindingStride = rayTracingProperties.shaderGroupBaseAlignment;

        // Trace rays.
        vkCmdTraceRaysNV(vkCommandBuffers[i],
            vkShaderBindingTable, bindingOffsetRayGenShader,
            vkShaderBindingTable, bindingOffsetMissShader, bindingStride,
            vkShaderBindingTable, bindingOffsetHitShader, bindingStride,
            VK_NULL_HANDLE, 0, 0,
            vkSelectedExtent.width, vkSelectedExtent.height, 1);

        // Prepare current swapchain image as transfer destination.
        VkImageMemoryBarrier imageMemoryBarrier1 {};
        imageMemoryBarrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier1.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        imageMemoryBarrier1.image = vkSwapChainImages[i];
        imageMemoryBarrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier1.srcAccessMask = 0;
        imageMemoryBarrier1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(
            vkCommandBuffers[i],
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier1);

        // Prepare ray tracing output image as transfer source.
        VkImageMemoryBarrier imageMemoryBarrier2 {};
        imageMemoryBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier2.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        imageMemoryBarrier2.image = vkStorageImage;
        imageMemoryBarrier2.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier2.srcAccessMask = 0;
        imageMemoryBarrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(
            vkCommandBuffers[i],
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier2);

        // Copy the storage image into the swap chain image.
        VkImageCopy copyRegion{};
        copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.srcOffset = { 0, 0, 0 };
        copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.dstOffset = { 0, 0, 0 };
        copyRegion.extent = { vkSelectedExtent.width, vkSelectedExtent.height, 1 };
        vkCmdCopyImage(vkCommandBuffers[i], vkStorageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkSwapChainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        // Transition swap chain image back for presentation.
        VkImageMemoryBarrier imageMemoryBarrier3 {};
        imageMemoryBarrier3.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier3.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier3.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier3.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        imageMemoryBarrier3.image = vkSwapChainImages[i];
        imageMemoryBarrier3.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier3.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageMemoryBarrier3.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier3.dstAccessMask = 0;
        vkCmdPipelineBarrier(
            vkCommandBuffers[i],
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier3);

        // Transition ray tracing output image back to general layout.
        VkImageMemoryBarrier imageMemoryBarrier4 {};
        imageMemoryBarrier4.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier4.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier4.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier4.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        imageMemoryBarrier4.image = vkStorageImage;
        imageMemoryBarrier4.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier4.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier4.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier4.dstAccessMask = 0;
        vkCmdPipelineBarrier(
            vkCommandBuffers[i],
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier4);

        // Fihish adding commands into the buffer.
        if (vkEndCommandBuffer(vkCommandBuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to finish command buffer recording" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    //                   STEP 28: Synchronization primitives
    // ==========================================================================
    // Rendering and presentation are not synchronized. It means that if the
    // application renders frames faster then they are displayed, it will lead
    // to memory overflow. In order to avoid this, we should wait in case
    // rendering goes too fast and the chain is overflown.
    // ==========================================================================

    // Describe a semaphore.
    VkSemaphoreCreateInfo vkSemaphoreInfo{};
    vkSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // The first semaphore group signals that an image is aquired and ready for rendering.
    // Create semaphore per each image we expect to render in parallel.
    // These semaphores perform GPU-GPU synchronization.
    std::vector< VkSemaphore > vkImageAvailableSemaphores;
    vkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Create a semaphore.
        if (vkCreateSemaphore(vkDevice, &vkSemaphoreInfo, nullptr, &vkImageAvailableSemaphores[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a semaphore!" << std::endl;
            abort();
        }
    }

    // The second semaphore group signals that an image is rendered and ready for presentation.
    // Create semaphore per each image we expect to render in parallel.
    // These semaphores perform GPU-GPU synchronization.
    std::vector< VkSemaphore > vkRenderFinishedSemaphores;
    vkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Create a semaphore.
        if (vkCreateSemaphore(vkDevice, &vkSemaphoreInfo, nullptr, &vkRenderFinishedSemaphores[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a semaphore!" << std::endl;
            abort();
        }
    }

    // In order to not overflow the swap chain we need to wait on CPU side if there are too many images
    // produced by GPU. This CPU-GPU synchronization is performed by fences.

    // Free fences for images running in parallel.
    std::vector< VkFence > vkInFlightFences;
    // Buffer of feces, locked by the images running in parallel.
    std::vector< VkFence > vkImagesInFlight;

    // Describe a fence.
    VkFenceCreateInfo vkFenceInfo{};
    vkFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create fences.
    vkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    vkImagesInFlight.resize(vkSwapChainImages.size(), VK_NULL_HANDLE);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(vkDevice, &vkFenceInfo, nullptr, &vkInFlightFences[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a fence!" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    //                         STEP 29: Main loop
    // ==========================================================================
    // Main loop performs event hanlding and executes rendering.
    // ==========================================================================

    // Pick a present queue.
    VkQueue vkPresentQueue;
    vkGetDeviceQueue(vkDevice, queueFamilyIndices.presentFamily.value(), 0, &vkPresentQueue);

    // Index of a framce processed in the current loop.
    // We go through MAX_FRAMES_IN_FLIGHT indices.
    size_t currentFrame = 0;

    // Main loop.
    while(!glfwWindowShouldClose(glfwWindow)) {
        // Poll GLFW events.
        glfwPollEvents();

        // Wait for the current frame.
        vkWaitForFences(vkDevice, 1, &vkInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        // Aquire a next image from a swap chain to process.
        uint32_t imageIndex;
        vkAcquireNextImageKHR(vkDevice, vkSwapChain, UINT64_MAX, vkImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // If the image is locked - wait for it.
        if (vkImagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(vkDevice, 1, &vkImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        // Put a free fence to imagesInFlight array.
        vkImagesInFlight[imageIndex] = vkInFlightFences[currentFrame];

        // Describe a submit to the graphics queue.
        VkSubmitInfo vkSubmitInfo{};
        vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // Specify semaphores the GPU should wait before executing the submit.
        std::array< VkSemaphore, 1 > vkWaitSemaphores{ vkImageAvailableSemaphores[currentFrame] };
        // Pipeline stages corresponding to each semaphore.
        std::array< VkPipelineStageFlags, 1 > vkWaitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        vkSubmitInfo.waitSemaphoreCount = vkWaitSemaphores.size();
        vkSubmitInfo.pWaitSemaphores = vkWaitSemaphores.data();
        vkSubmitInfo.pWaitDstStageMask = vkWaitStages.data();
        vkSubmitInfo.commandBufferCount = 1;
        vkSubmitInfo.pCommandBuffers = &vkCommandBuffers[imageIndex];
        // Specify semaphores the GPU should unlock after executing the submit.
        std::array< VkSemaphore, 1 > vkSignalSemaphores{ vkRenderFinishedSemaphores[currentFrame] };
        vkSubmitInfo.signalSemaphoreCount = vkSignalSemaphores.size();
        vkSubmitInfo.pSignalSemaphores = vkSignalSemaphores.data();

        // Reset the fence.
        vkResetFences(vkDevice, 1, &vkInFlightFences[currentFrame]);

        // Submit to the queue.
        if (vkQueueSubmit(vkGraphicsQueue, 1, &vkSubmitInfo, vkInFlightFences[currentFrame]) != VK_SUCCESS) {
            std::cerr << "Failed to submit" << std::endl;
            abort();
        }

        // Prepare an image for presentation.
        VkPresentInfoKHR vkPresentInfo{};
        vkPresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        // Specify semaphores we need to wait before presenting the image.
        vkPresentInfo.waitSemaphoreCount = vkSignalSemaphores.size();
        vkPresentInfo.pWaitSemaphores = vkSignalSemaphores.data();
        std::array< VkSwapchainKHR, 1 > swapChains{ vkSwapChain };
        vkPresentInfo.swapchainCount = swapChains.size();
        vkPresentInfo.pSwapchains = swapChains.data();
        vkPresentInfo.pImageIndices = &imageIndex;
        vkPresentInfo.pResults = nullptr;

        // Submit and image for presentaion.
        vkQueuePresentKHR(vkPresentQueue, &vkPresentInfo);

        // Switch to the next frame in the loop.
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // ==========================================================================
    //                     STEP 30: Deinitialization
    // ==========================================================================
    // Destroy all created Vukan structures in a reverse order.
    // ==========================================================================

    // Wait until all pending render operations are finished.
    vkDeviceWaitIdle(vkDevice);

    // Destroy fences.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(vkDevice, vkInFlightFences[i], nullptr);
    }

    // Destroy semaphores.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkDevice, vkRenderFinishedSemaphores[i], nullptr);
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkDevice, vkImageAvailableSemaphores[i], nullptr);
    }

    // Destory command pool
    vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);

    // Destroy descriptor pool.
    vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);

    // Destroy uniform buffer.
    vkFreeMemory(vkDevice, vkUniformBufferMemory, nullptr);
    vkDestroyBuffer(vkDevice, vkUniformBuffer, nullptr);

    // Destroy shader binding table.
    vkFreeMemory(vkDevice, vkShaderBindingTableMemory, nullptr);
    vkDestroyBuffer(vkDevice, vkShaderBindingTable, nullptr);

    // Destroy pipeline.
    vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);

    // Destroy descriptor set layout.
    vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);

    // Destroy shaders.
    vkDestroyShaderModule(vkDevice, vkRayhitShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkRaymissShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkRaygenShaderModule, nullptr);

    // Destroy storage image.
    vkDestroyImageView(vkDevice, vkStorageImageView, nullptr);
    vkFreeMemory(vkDevice, vkStorageImageMemory, nullptr);
    vkDestroyImage(vkDevice, vkStorageImage, nullptr);

    // Destroy TLAS.
    vkFreeMemory(vkDevice, vkTlasMemory, nullptr);
    vkDestroyAccelerationStructureNV(vkDevice, vkTopLevelAccelerationStructure, nullptr);

    // Destroy instance buffer.
    vkFreeMemory(vkDevice, vkInstanceBufferMemory, nullptr);
    vkDestroyBuffer(vkDevice, vkInstanceBufferHandle, nullptr);

    // Destroy BLAS.
    vkFreeMemory(vkDevice, vkBlasMemory, nullptr);
    vkDestroyAccelerationStructureNV(vkDevice, vkBottomLevelAccelerationStructure, nullptr);

    // Destroy vertex buffer.
    vkFreeMemory(vkDevice, vkVertexBufferMemory, nullptr);
    vkDestroyBuffer(vkDevice, vkVertexBuffer, nullptr);

    // Destory swap chain image views.
    for (auto imageView : vkSwapChainImageViews) {
        vkDestroyImageView(vkDevice, imageView, nullptr);
    }

    // Destroy swap chain.
    vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);

    // Destory logical device.
    vkDestroyDevice(vkDevice, nullptr);

#ifdef DEBUG_MODE

    // Get pointer to an extension function vkDestroyDebugUtilsMessengerEXT.
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkDestroyDebugUtilsMessengerEXT == nullptr) {
        std::cerr << "Function vkDestroyDebugUtilsMessengerEXT not found!";
        abort();
    }

    // Destory debug messenger.
    vkDestroyDebugUtilsMessengerEXT(vkInstance, vkDebugMessenger, nullptr);

#endif

    // Destory surface.
    vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);

    // Destroy Vulkan instance.
    vkDestroyInstance(vkInstance, nullptr);

    // Destroy window.
    glfwDestroyWindow(glfwWindow);

    // Deinitialize GLFW library.
    glfwTerminate();

    return 0;
}
