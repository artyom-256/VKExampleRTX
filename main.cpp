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
 *       The application shows initialization of Vulkan, creation of        *
 *     vertex buffers, shaders and uniform buffers in order to display      *
 *        a simple example of 3D graphics - rotating colored cube.          *
 *                                                                          *
 *    The example is designed to demonstrate pure sequence of actions       *
 *    required to create a Vulkan 3D application, so all code is done       *
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
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>

#include <set>
#include <array>
#include <chrono>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <algorithm>

/**
 * Switch on validation levels.
 * Validation levels provided by LunarG display error messages in case of
 * incorrect usage of Vulkan functions.
 * Comment this line to switching off DEBUG_MODE.
 * It makes the application faster but silent.
 */
#define DEBUG_MODE

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
constexpr const char* APPLICATION_NAME = "VKExample";
/**
 * Maximal amount of frames processed at the same time.
 */
constexpr int MAX_FRAMES_IN_FLIGHT = 5;






#include <functional>

















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
    std::cerr << "[MSG]:" << pCallbackData->pMessage << std::endl;
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
    //               STEP 12: Create a descriptor set layout
    // ==========================================================================
    // Descriptor set layout describes details of every uniform data binding
    // using in shaders. This is needed if we want to use uniforms in shaders.
    // ==========================================================================

    VkDescriptorSetLayoutBinding vkUboLayoutBinding{};
    vkUboLayoutBinding.binding = 0;
    vkUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vkUboLayoutBinding.descriptorCount = 1;
    vkUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vkUboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayout vkDescriptorSetLayout;

    VkDescriptorSetLayoutCreateInfo vkLayoutInfo{};
    vkLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vkLayoutInfo.bindingCount = 1;
    vkLayoutInfo.pBindings = &vkUboLayoutBinding;

    if (vkCreateDescriptorSetLayout(vkDevice, &vkLayoutInfo, nullptr, &vkDescriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create a descriptor set layout" << std::endl;
        abort();
    }

    // ==========================================================================
    //                        STEP 13: Load shaders
    // ==========================================================================
    // Shaders are special code that is executed directly on GPU.
    // For our simple example we use vertex and fragment shaders.
    // Unlike OpenGL, Vulcan uses a binary format which is called SPIR-V and
    // each shader should be compiled to this format using glslc compiler that
    // could be found in Vulkan SDK.
    // ==========================================================================

    // --------------------------------------------------------------------------
    // Create a vertex shader module.
    // --------------------------------------------------------------------------

    // Open file.
    std::ifstream vertexShaderFile("main.vert.spv", std::ios::ate | std::ios::binary);
    if (!vertexShaderFile.is_open()) {
        std::cerr << "Vertex shader file not found!" << std::endl;
        abort();
    }
    // Calculate file size.
    size_t vertexFileSize = static_cast< size_t >(vertexShaderFile.tellg());
    // Jump to the beginning of the file.
    vertexShaderFile.seekg(0);
    // Read shader code.
    std::vector< char > vertexShaderBuffer(vertexFileSize);
    vertexShaderFile.read(vertexShaderBuffer.data(), vertexFileSize);
    // Close the file.
    vertexShaderFile.close();
    // Shader module creation info.
    VkShaderModuleCreateInfo vkVertexShaderCreateInfo{};
    vkVertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkVertexShaderCreateInfo.codeSize = vertexShaderBuffer.size();
    vkVertexShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(vertexShaderBuffer.data());
    // Create a vertex shader module.
    VkShaderModule vkVertexShaderModule;
    if (vkCreateShaderModule(vkDevice, &vkVertexShaderCreateInfo, nullptr, &vkVertexShaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create a shader!" << std::endl;
        abort();
    }

    // --------------------------------------------------------------------------
    // Create a fragment shader module.
    // --------------------------------------------------------------------------

    // Open file.
    std::ifstream fragmentShaderFile("main.frag.spv", std::ios::ate | std::ios::binary);
    if (!fragmentShaderFile.is_open()) {
        std::cerr << "Fragment shader file not found!" << std::endl;
        abort();
    }
    // Calculate file size.
    size_t fragmentFileSize = static_cast< size_t >(fragmentShaderFile.tellg());
    // Jump to the beginning of the file.
    fragmentShaderFile.seekg(0);
    // Read shader code.
    std::vector< char > fragmentShaderBuffer(fragmentFileSize);
    fragmentShaderFile.read(fragmentShaderBuffer.data(), fragmentFileSize);
    // Close the file.
    fragmentShaderFile.close();
    // Shader module creation info.
    VkShaderModuleCreateInfo vkFragmentShaderCreateInfo{};
    vkFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkFragmentShaderCreateInfo.codeSize = fragmentShaderBuffer.size();
    vkFragmentShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(fragmentShaderBuffer.data());
    // Create a fragment shader module.
    VkShaderModule vkFragmentShaderModule;
    if (vkCreateShaderModule(vkDevice, &vkFragmentShaderCreateInfo, nullptr, &vkFragmentShaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create a shader!" << std::endl;
        abort();
    }

    // --------------------------------------------------------------------------

    // Create a pipeline stage for a vertex shader.
    VkPipelineShaderStageCreateInfo vkVertShaderStageInfo{};
    vkVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vkVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vkVertShaderStageInfo.module = vkVertexShaderModule;
    // main function name
    vkVertShaderStageInfo.pName = "main";

    // Create a pipeline stage for a fragment shader.
    VkPipelineShaderStageCreateInfo vkFragShaderStageInfo{};
    vkFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vkFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    vkFragShaderStageInfo.module = vkFragmentShaderModule;
    // main function name
    vkFragShaderStageInfo.pName = "main";

    // Put both shader modules into an array.
    std::array< VkPipelineShaderStageCreateInfo, 2 > shaderStages {
        vkVertShaderStageInfo,
        vkFragShaderStageInfo
    };

    // ==========================================================================
    //                    STEP 14: Create a vertex buffer
    // ==========================================================================
    // Vertex buffers provide vertices to shaders.
    // In our case this is a geometry of a cube.
    // ==========================================================================

    // Structure that represents a vertex of a cube.
    struct Vertex
    {
        // 3D coordinates of the vertex.
        glm::vec3 pos;
        // Color of the vertex.
        glm::vec3 color;
    };

    // Binding descriptor specifies how our array is splited into vertices.
    // In particular, we say that each sizeof(Vertex) bytes correspond to one vertex.
    VkVertexInputBindingDescription vkBindingDescription{};
    vkBindingDescription.binding = 0;
    vkBindingDescription.stride = sizeof(Vertex);
    vkBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Attribute description specifies how one vertext is split into separate variables.
    // In our case a vertext is a composition of two vec3 values: coordinates and color.
    // Property location corresponds to a location value in shader code.
    std::array< VkVertexInputAttributeDescription, 2 > vkAttributeDescriptions{};
    // Description of the first attribute (coordinates).
    vkAttributeDescriptions[0].binding = 0;
    vkAttributeDescriptions[0].location = 0;
    vkAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vkAttributeDescriptions[0].offset = offsetof(Vertex, pos);
    // Description of the second attribute (color).
    vkAttributeDescriptions[1].binding = 0;
    vkAttributeDescriptions[1].location = 1;
    vkAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vkAttributeDescriptions[1].offset = offsetof(Vertex, color);

    // Create a vertex input state for pipeline creation.
    VkPipelineVertexInputStateCreateInfo vkVertexInputInfo{};
    vkVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vkVertexInputInfo.vertexBindingDescriptionCount = 1;
    vkVertexInputInfo.pVertexBindingDescriptions = &vkBindingDescription;
    vkVertexInputInfo.vertexAttributeDescriptionCount = static_cast< uint32_t >(vkAttributeDescriptions.size());
    vkVertexInputInfo.pVertexAttributeDescriptions = vkAttributeDescriptions.data();


    // ==========================================================================
    //                    STEP 30: Create a vertex buffer
    // ==========================================================================
    // Vertex buffer contains vertices of our model we want to pass
    // to the vertex shader.
    // ==========================================================================

    // Create a cube specifying its vertices.
    // Each triplet of vertices represent one triangle.
    // We do not use index buffer, so some vertices are duplicated.
    // Each plane has its own color.
    /*std::vector< glm::vec3 > vertices
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
    };*/

    std::vector<Vertex> vertices = {
                        { {  1.0f,  1.0f, 0.0f } },
                        { { -1.0f,  1.0f, 0.0f } },
                        { {  0.0f, -1.0f, 0.0f } },

                        { { -1.0f,  1.0f, 0.0f } },
                        { {  1.0f,  1.0f, 0.0f } },
                        { {  0.0f, -1.0f, 0.0f } },
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
    uint32_t bufferMemTypeIndex = UINT32_MAX;
    VkMemoryPropertyFlags vkBufferMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkPhysicalDeviceMemoryProperties vkBufferMemProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkBufferMemProperties);
    for (uint32_t i = 0; i < vkBufferMemProperties.memoryTypeCount; i++) {
        if ((vkVertexBufferMemRequirements.memoryTypeBits & (1 << i)) &&
                (vkBufferMemProperties.memoryTypes[i].propertyFlags & vkBufferMemFlags) == vkBufferMemFlags) {
            bufferMemTypeIndex = i;
            break;
        }
    }
    vkVertexBufferAllocInfo.memoryTypeIndex = bufferMemTypeIndex;
    // Allocate memory for the vertex buffer.
    VkDeviceMemory vkVertexBufferMemory;
    if (vkAllocateMemory(vkDevice, &vkVertexBufferAllocInfo, nullptr, &vkVertexBufferMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate memroy for the vertex buffer!" << std::endl;
        abort();
    }

    // Bind the buffer to the allocated memory.
    vkBindBufferMemory(vkDevice, vkVertexBuffer, vkVertexBufferMemory, 0);

    // Copy our vertices to the allocated memory.
    void* vertexBufferMemoryData;
    vkMapMemory(vkDevice, vkVertexBufferMemory, 0, vertexBufferSize, 0, &vertexBufferMemoryData);
    memcpy(vertexBufferMemoryData, vertices.data(), (size_t) vertexBufferSize);
    vkUnmapMemory(vkDevice, vkVertexBufferMemory);




















    PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
    PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
    PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
    PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
    PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
    PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
    PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
    PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
    PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;


    // Ray tracing acceleration structure
    struct AccelerationStructure {
        VkDeviceMemory memory;
        VkAccelerationStructureNV accelerationStructure;
        uint64_t handle;
    };

    AccelerationStructure bottomLevelAS;
    AccelerationStructure topLevelAS;



    vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(vkDevice, "vkCreateAccelerationStructureNV"));
    vkDestroyAccelerationStructureNV = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(vkGetDeviceProcAddr(vkDevice, "vkDestroyAccelerationStructureNV"));
    vkBindAccelerationStructureMemoryNV = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(vkGetDeviceProcAddr(vkDevice, "vkBindAccelerationStructureMemoryNV"));
    vkGetAccelerationStructureHandleNV = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(vkGetDeviceProcAddr(vkDevice, "vkGetAccelerationStructureHandleNV"));
    vkGetAccelerationStructureMemoryRequirementsNV = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(vkGetDeviceProcAddr(vkDevice, "vkGetAccelerationStructureMemoryRequirementsNV"));
    vkCmdBuildAccelerationStructureNV = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(vkGetDeviceProcAddr(vkDevice, "vkCmdBuildAccelerationStructureNV"));
    vkCreateRayTracingPipelinesNV = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(vkGetDeviceProcAddr(vkDevice, "vkCreateRayTracingPipelinesNV"));
    vkGetRayTracingShaderGroupHandlesNV = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(vkGetDeviceProcAddr(vkDevice, "vkGetRayTracingShaderGroupHandlesNV"));
    vkCmdTraceRaysNV = reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(vkDevice, "vkCmdTraceRaysNV"));

    // Query the ray tracing properties of the current implementation, we will need them later on
    VkPhysicalDeviceRayTracingPropertiesNV rayTracingProperties{};
    rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
    VkPhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps2.pNext = &rayTracingProperties;
    vkGetPhysicalDeviceProperties2(vkPhysicalDevice, &deviceProps2);

    VkGeometryNV geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    geometry.geometry.triangles.vertexData = vkVertexBuffer;
    geometry.geometry.triangles.vertexOffset = 0;
    geometry.geometry.triangles.vertexCount = static_cast<uint32_t>(vertices.size());
    geometry.geometry.triangles.vertexStride = sizeof(glm::vec3);
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.indexData = 0;
    geometry.geometry.triangles.indexOffset = 0;
    geometry.geometry.triangles.indexCount = 0;
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_NV;
    geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
    geometry.geometry.triangles.transformOffset = 0;
    geometry.geometry.aabbs = {};
    geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

    // CREATE BLAS

    {
        VkAccelerationStructureInfoNV accelerationStructureInfo{};
        accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
        accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
        accelerationStructureInfo.instanceCount = 0;
        accelerationStructureInfo.geometryCount = 1;
        accelerationStructureInfo.pGeometries = &geometry;

        VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
        accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
        accelerationStructureCI.info = accelerationStructureInfo;
        /*VK_CHECK_RESULT(*/
        vkCreateAccelerationStructureNV(vkDevice, &accelerationStructureCI, nullptr, &bottomLevelAS.accelerationStructure);

        VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
        memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
        memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
        memoryRequirementsInfo.accelerationStructure = bottomLevelAS.accelerationStructure;

        VkMemoryRequirements2 memoryRequirements2{};
        memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        vkGetAccelerationStructureMemoryRequirementsNV(vkDevice, &memoryRequirementsInfo, &memoryRequirements2);

        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;

        VkPhysicalDeviceMemoryProperties vkBufferMemProperties;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkBufferMemProperties);

        uint32_t colorImageMemoryTypeInex = UINT32_MAX;
        for (uint32_t i = 0; i < vkBufferMemProperties.memoryTypeCount; i++) {
            if ((memoryRequirements2.memoryRequirements.memoryTypeBits & (1 << i)) && (vkBufferMemProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                colorImageMemoryTypeInex = i;
                break;
            }
        }

        memoryAllocateInfo.memoryTypeIndex = colorImageMemoryTypeInex;
        /*VK_CHECK_RESULT(*/
        vkAllocateMemory(vkDevice, &memoryAllocateInfo, nullptr, &bottomLevelAS.memory);

        VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
        accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
        accelerationStructureMemoryInfo.accelerationStructure = bottomLevelAS.accelerationStructure;
        accelerationStructureMemoryInfo.memory = bottomLevelAS.memory;
        /*VK_CHECK_RESULT(*/
        vkBindAccelerationStructureMemoryNV(vkDevice, 1, &accelerationStructureMemoryInfo);

        /*VK_CHECK_RESULT(*/
        vkGetAccelerationStructureHandleNV(vkDevice, bottomLevelAS.accelerationStructure, sizeof(uint64_t), &bottomLevelAS.handle);
    }

    // END CREATE BLAS

    VkTransformMatrixKHR transform = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
    };

    VkAccelerationStructureInstanceKHR geometryInstance{};
    geometryInstance.transform = transform;
    geometryInstance.instanceCustomIndex = 0;
    geometryInstance.mask = 0xff;
    geometryInstance.instanceShaderBindingTableRecordOffset = 0;
    geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
    geometryInstance.accelerationStructureReference = bottomLevelAS.handle;

    VkBuffer m_instance_buffer_handle;

    {
        VkBuffer m_handle;
        VkDeviceMemory m_memory;
        VkDeviceSize m_size;

        // Describe a buffer.
        VkBufferCreateInfo vkVertexBufferInfo{};
        vkVertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vkVertexBufferInfo.size = sizeof(VkAccelerationStructureInstanceKHR);
        vkVertexBufferInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
        vkVertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // Create a buffer.
        if (vkCreateBuffer(vkDevice, &vkVertexBufferInfo, nullptr, &m_instance_buffer_handle) != VK_SUCCESS) {
            std::cerr << "Failed to create a vertex buffer!" << std::endl;
            abort();
        }

        // Retrieve memory requirements for the vertex buffer.
        VkMemoryRequirements vkVertexBufferMemRequirements;
        vkGetBufferMemoryRequirements(vkDevice, m_instance_buffer_handle, &vkVertexBufferMemRequirements);

        // Define memory allocate info.
        VkMemoryAllocateInfo vkVertexBufferAllocInfo{};
        vkVertexBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkVertexBufferAllocInfo.allocationSize = vkVertexBufferMemRequirements.size;
        // Find a suitable memory type.



        VkPhysicalDeviceMemoryProperties vkBufferMemProperties;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkBufferMemProperties);

        uint32_t colorImageMemoryTypeInex = UINT32_MAX;
        for (uint32_t i = 0; i < vkBufferMemProperties.memoryTypeCount; i++) {
            if ((vkVertexBufferMemRequirements.memoryTypeBits & (1 << i)) && (vkBufferMemProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                                                                          && (vkBufferMemProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )) {
                colorImageMemoryTypeInex = i;
                break;
            }
        }



        vkVertexBufferAllocInfo.memoryTypeIndex = colorImageMemoryTypeInex;
        // Allocate memory for the vertex buffer.
        if (vkAllocateMemory(vkDevice, &vkVertexBufferAllocInfo, nullptr, &m_memory) != VK_SUCCESS) {
            std::cerr << "Failed to allocate memroy for the vertex buffer!" << std::endl;
            abort();
        }

        // Bind the buffer to the allocated memory.
        vkBindBufferMemory(vkDevice, m_instance_buffer_handle, m_memory, 0);







        // Copy our vertices to the allocated memory.
        void* vertexBufferMemoryData;
        vkMapMemory(vkDevice, m_memory, 0, sizeof(geometryInstance), 0, &vertexBufferMemoryData);
        memcpy(vertexBufferMemoryData, &geometryInstance, sizeof(geometryInstance));
        vkUnmapMemory(vkDevice, m_memory);
    }

    // CREATE TLAS
    {
        VkAccelerationStructureInfoNV accelerationStructureInfo{};
        accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
        accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
        accelerationStructureInfo.instanceCount = 2;
        accelerationStructureInfo.geometryCount = 0;

        VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
        accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
        accelerationStructureCI.info = accelerationStructureInfo;
        /*VK_CHECK_RESULT*/
        vkCreateAccelerationStructureNV(vkDevice, &accelerationStructureCI, nullptr, &topLevelAS.accelerationStructure);

        VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
        memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
        memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
        memoryRequirementsInfo.accelerationStructure = topLevelAS.accelerationStructure;

        VkMemoryRequirements2 memoryRequirements2{};
        memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        vkGetAccelerationStructureMemoryRequirementsNV(vkDevice, &memoryRequirementsInfo, &memoryRequirements2);

        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;

        VkPhysicalDeviceMemoryProperties vkBufferMemProperties;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkBufferMemProperties);

        uint32_t colorImageMemoryTypeInex = UINT32_MAX;
        for (uint32_t i = 0; i < vkBufferMemProperties.memoryTypeCount; i++) {
            if ((memoryRequirements2.memoryRequirements.memoryTypeBits & (1 << i)) && (vkBufferMemProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                colorImageMemoryTypeInex = i;
                break;
            }
        }

        /*VK_CHECK_RESULT*/
        vkAllocateMemory(vkDevice, &memoryAllocateInfo, nullptr, &topLevelAS.memory);

        VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
        accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
        accelerationStructureMemoryInfo.accelerationStructure = topLevelAS.accelerationStructure;
        accelerationStructureMemoryInfo.memory = topLevelAS.memory;
        /*VK_CHECK_RESULT*/
        vkBindAccelerationStructureMemoryNV(vkDevice, 1, &accelerationStructureMemoryInfo);

        /*VK_CHECK_RESULT*/
        vkGetAccelerationStructureHandleNV(vkDevice, topLevelAS.accelerationStructure, sizeof(uint64_t), &topLevelAS.handle);
    }
    // END CREATE TLAS

    // Acceleration structure build requires some scratch space to store temporary information
    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

    VkMemoryRequirements2 memReqBottomLevelAS;
    memReqBottomLevelAS.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    memoryRequirementsInfo.accelerationStructure = bottomLevelAS.accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(vkDevice, &memoryRequirementsInfo, &memReqBottomLevelAS);

    VkMemoryRequirements2 memReqTopLevelAS;
    memReqTopLevelAS.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    memoryRequirementsInfo.accelerationStructure = topLevelAS.accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(vkDevice, &memoryRequirementsInfo, &memReqTopLevelAS);

    const VkDeviceSize scratchBufferSize = std::max(memReqBottomLevelAS.memoryRequirements.size, memReqTopLevelAS.memoryRequirements.size);

    VkBuffer m_scratch_buffer_handle;

    {
        VkBuffer m_handle;
        VkDeviceMemory m_memory;
        VkDeviceSize m_size;

        // Describe a buffer.
        VkBufferCreateInfo vkVertexBufferInfo{};
        vkVertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vkVertexBufferInfo.size = scratchBufferSize;
        vkVertexBufferInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
        vkVertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // Create a buffer.
        if (vkCreateBuffer(vkDevice, &vkVertexBufferInfo, nullptr, &m_scratch_buffer_handle) != VK_SUCCESS) {
            std::cerr << "Failed to create a vertex buffer!" << std::endl;
            abort();
        }

        // Retrieve memory requirements for the vertex buffer.
        VkMemoryRequirements vkVertexBufferMemRequirements;
        vkGetBufferMemoryRequirements(vkDevice, m_scratch_buffer_handle, &vkVertexBufferMemRequirements);

        // Define memory allocate info.
        VkMemoryAllocateInfo vkVertexBufferAllocInfo{};
        vkVertexBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkVertexBufferAllocInfo.allocationSize = vkVertexBufferMemRequirements.size;
        // Find a suitable memory type.



        VkPhysicalDeviceMemoryProperties vkBufferMemProperties;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkBufferMemProperties);

        uint32_t colorImageMemoryTypeInex = UINT32_MAX;
        for (uint32_t i = 0; i < vkBufferMemProperties.memoryTypeCount; i++) {
            if ((vkVertexBufferMemRequirements.memoryTypeBits & (1 << i)) && (vkBufferMemProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                colorImageMemoryTypeInex = i;
                break;
            }
        }



        vkVertexBufferAllocInfo.memoryTypeIndex = colorImageMemoryTypeInex;
        // Allocate memory for the vertex buffer.
        if (vkAllocateMemory(vkDevice, &vkVertexBufferAllocInfo, nullptr, &m_memory) != VK_SUCCESS) {
            std::cerr << "Failed to allocate memroy for the vertex buffer!" << std::endl;
            abort();
        }

        // Bind the buffer to the allocated memory.
        vkBindBufferMemory(vkDevice, m_scratch_buffer_handle, m_memory, 0);
    }




















    // Pick a graphics queue.
    VkQueue vkGraphicsQueue;
    vkGetDeviceQueue(vkDevice, queueFamilyIndices.graphicsFamily.value(), 0, &vkGraphicsQueue);













    auto executeCommandBuffer = [=](std::function< void (const VkCommandBuffer&) > executeFunction)
    {
        // Describe a command pool.
        VkCommandPoolCreateInfo vkASPoolInfo{};
        vkASPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        vkASPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        vkASPoolInfo.flags = 0;

        VkCommandPool vkASCommandPool;
        if (vkCreateCommandPool(vkDevice, &vkASPoolInfo, nullptr, &vkASCommandPool) != VK_SUCCESS) {
            std::cerr << "Failed to create a command pool!" << std::endl;
            abort();
        }

        VkCommandBufferAllocateInfo cmdBufAllocateInfo {};
        cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool = vkASCommandPool;
        cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer cmdBuffer;
        /*VK_CHECK_RESULT*/(vkAllocateCommandBuffers(vkDevice, &cmdBufAllocateInfo, &cmdBuffer));
        VkCommandBufferBeginInfo cmdBufInfo {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        /*VK_CHECK_RESULT*/(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

        executeFunction(cmdBuffer);

        /*VK_CHECK_RESULT*/(vkEndCommandBuffer(cmdBuffer));

        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;
        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = 0;
        VkFence fence;
        /*VK_CHECK_RESULT*/(vkCreateFence(vkDevice, &fenceInfo, nullptr, &fence));
        // Submit to the queue
        vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, fence);
        // Wait for the fence to signal that command buffer has finished executing
        /*VK_CHECK_RESULT*/(vkWaitForFences(vkDevice, 1, &fence, VK_TRUE, UINT64_MAX));
        vkDestroyFence(vkDevice, fence, nullptr);
        vkFreeCommandBuffers(vkDevice, vkASCommandPool, 1, &cmdBuffer);
    };










    executeCommandBuffer([&](const VkCommandBuffer& cmdBuffer) {
                VkAccelerationStructureInfoNV buildInfo{};
                buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
                buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
                buildInfo.geometryCount = 1;
                buildInfo.pGeometries = &geometry;

                vkCmdBuildAccelerationStructureNV(
                            cmdBuffer,
                            &buildInfo,
                            VK_NULL_HANDLE,
                            0,
                            VK_FALSE,
                            bottomLevelAS.accelerationStructure,
                            VK_NULL_HANDLE,
                            m_scratch_buffer_handle,
                            0);

                VkMemoryBarrier memoryBarrier {};
                memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
                memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
                vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

                buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
                buildInfo.pGeometries = 0;
                buildInfo.geometryCount = 0;
                buildInfo.instanceCount = 1;

                vkCmdBuildAccelerationStructureNV(
                            cmdBuffer,
                            &buildInfo,
                            m_instance_buffer_handle,
                            0,
                            VK_FALSE,
                            topLevelAS.accelerationStructure,
                            VK_NULL_HANDLE,
                            m_scratch_buffer_handle,
                            0);

                vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);
            });










    VkImage m_storage_image;
    VkImageView m_storage_image_view;

    {
        VkDeviceMemory m_colorImageMemory;

        // Description of an image for resolve attachment.
        VkImageCreateInfo vkColorImageInfo{};
        vkColorImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        vkColorImageInfo.imageType = VK_IMAGE_TYPE_2D;
        vkColorImageInfo.extent.width = vkSelectedExtent.width;
        vkColorImageInfo.extent.height = vkSelectedExtent.height;
        vkColorImageInfo.extent.depth = 1;
        vkColorImageInfo.mipLevels = 1;
        vkColorImageInfo.arrayLayers = 1;
        vkColorImageInfo.format = vkSelectedFormat.format;
        vkColorImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        vkColorImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vkColorImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        vkColorImageInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
        vkColorImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // Create an image for resolve attachment.
        if (vkCreateImage(vkDevice, &vkColorImageInfo, nullptr, &m_storage_image) != VK_SUCCESS) {
            std::cerr << "Failed to create an image!" << std::endl;
            abort();
        }

        // Get memory requirements.
        VkMemoryRequirements vkMemRequirements;
        vkGetImageMemoryRequirements(vkDevice, m_storage_image, &vkMemRequirements);

        // Description of memory allocation.
        VkMemoryAllocateInfo vkColorImageAllocInfo{};
        vkColorImageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkColorImageAllocInfo.allocationSize = vkMemRequirements.size;
        // Find memory type.
        VkPhysicalDeviceMemoryProperties vkBufferMemProperties;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkBufferMemProperties);

        uint32_t colorImageMemoryTypeInex = UINT32_MAX;
        for (uint32_t i = 0; i < vkBufferMemProperties.memoryTypeCount; i++) {
            if ((vkMemRequirements.memoryTypeBits & (1 << i)) && (vkBufferMemProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                colorImageMemoryTypeInex = i;
                break;
            }
        }
        vkColorImageAllocInfo.memoryTypeIndex = colorImageMemoryTypeInex;

        // Allocate memory for resolve attachment.
        if (vkAllocateMemory(vkDevice, &vkColorImageAllocInfo, nullptr, &m_colorImageMemory) != VK_SUCCESS) {
            std::cerr << "Failed to allocate image memory!" << std::endl;
            abort();
        }

        // Bind the image to the memory.
        vkBindImageMemory(vkDevice, m_storage_image, m_colorImageMemory, 0);

        // Describe an image view for resolve attachment.
        VkImageViewCreateInfo vkColorImageViewInfo{};
        vkColorImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vkColorImageViewInfo.image = m_storage_image;
        vkColorImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vkColorImageViewInfo.format = vkSelectedFormat.format;
        vkColorImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkColorImageViewInfo.subresourceRange.baseMipLevel = 0;
        vkColorImageViewInfo.subresourceRange.levelCount = 1;
        vkColorImageViewInfo.subresourceRange.baseArrayLayer = 0;
        vkColorImageViewInfo.subresourceRange.layerCount = 1;

        // Create an image view for resolve attachment.
        if (vkCreateImageView(vkDevice, &vkColorImageViewInfo, nullptr, &m_storage_image_view) != VK_SUCCESS) {
            std::cerr << "Failed to create texture image view!" << std::endl;
            abort();
        }
    }












    auto setImageLayout = [=](
                VkCommandBuffer cmdbuffer,
                VkImage image,
                VkImageLayout oldImageLayout,
                VkImageLayout newImageLayout,
                VkImageSubresourceRange subresourceRange,
                VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
            {
                // Create an image barrier object
                VkImageMemoryBarrier imageMemoryBarrier {};
                imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.oldLayout = oldImageLayout;
                imageMemoryBarrier.newLayout = newImageLayout;
                imageMemoryBarrier.image = image;
                imageMemoryBarrier.subresourceRange = subresourceRange;

                // Source layouts (old)
                // Source access mask controls actions that have to be finished on the old layout
                // before it will be transitioned to the new layout
                switch (oldImageLayout)
                {
                case VK_IMAGE_LAYOUT_UNDEFINED:
                    // Image layout is undefined (or does not matter)
                    // Only valid as initial layout
                    // No flags required, listed only for completeness
                    imageMemoryBarrier.srcAccessMask = 0;
                    break;

                case VK_IMAGE_LAYOUT_PREINITIALIZED:
                    // Image is preinitialized
                    // Only valid as initial layout for linear images, preserves memory contents
                    // Make sure host writes have been finished
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    // Image is a color attachment
                    // Make sure any writes to the color buffer have been finished
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    // Image is a depth/stencil attachment
                    // Make sure any writes to the depth/stencil buffer have been finished
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    // Image is a transfer source
                    // Make sure any reads from the image have been finished
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    break;

                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    // Image is a transfer destination
                    // Make sure any writes to the image have been finished
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    // Image is read by a shader
                    // Make sure any shader reads from the image have been finished
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    break;
                default:
                    // Other source layouts aren't handled (yet)
                    break;
                }

                // Target layouts (new)
                // Destination access mask controls the dependency for the new image layout
                switch (newImageLayout)
                {
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    // Image will be used as a transfer destination
                    // Make sure any writes to the image have been finished
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    // Image will be used as a transfer source
                    // Make sure any reads from the image have been finished
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    break;

                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    // Image will be used as a color attachment
                    // Make sure any writes to the color buffer have been finished
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    // Image layout will be used as a depth/stencil attachment
                    // Make sure any writes to depth/stencil buffer have been finished
                    imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    // Image will be read in a shader (sampler, input attachment)
                    // Make sure any writes to the image have been finished
                    if (imageMemoryBarrier.srcAccessMask == 0)
                    {
                        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                    }
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    break;
                default:
                    // Other source layouts aren't handled (yet)
                    break;
                }

                // Put barrier inside setup command buffer
                vkCmdPipelineBarrier(
                    cmdbuffer,
                    srcStageMask,
                    dstStageMask,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &imageMemoryBarrier);
            };









    executeCommandBuffer([&](const VkCommandBuffer& cmdBuffer) {
                setImageLayout(cmdBuffer, m_storage_image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL,
                                { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
            });






















    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;


    {
        VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
        accelerationStructureLayoutBinding.binding = 0;
        accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
        accelerationStructureLayoutBinding.descriptorCount = 1;
        accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

        VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
        resultImageLayoutBinding.binding = 1;
        resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        resultImageLayoutBinding.descriptorCount = 1;
        resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

        VkDescriptorSetLayoutBinding uniformBufferBinding{};
        uniformBufferBinding.binding = 2;
        uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferBinding.descriptorCount = 1;
        uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

        std::vector<VkDescriptorSetLayoutBinding> bindings({
            accelerationStructureLayoutBinding,
            resultImageLayoutBinding,
            uniformBufferBinding
            });

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        /*VK_CHECK_RESULT*/(vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &descriptorSetLayout));

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

        /*VK_CHECK_RESULT*/(vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

        VkPipelineShaderStageCreateInfo vkRaygenShaderModuleCreateInfo{};
        VkShaderModule vkRaygenShaderModule;
        {
            // Open file.
            std::ifstream fragmentShaderFile("main2.rgen.spv", std::ios::ate | std::ios::binary);
            if (!fragmentShaderFile.is_open()) {
                std::cerr << "Raygen shader file not found!" << std::endl;
                abort();
            }
            // Calculate file size.
            size_t fragmentFileSize = static_cast< size_t >(fragmentShaderFile.tellg());
            // Jump to the beginning of the file.
            fragmentShaderFile.seekg(0);
            // Read shader code.
            std::vector< char > fragmentShaderBuffer(fragmentFileSize);
            fragmentShaderFile.read(fragmentShaderBuffer.data(), fragmentFileSize);
            // Close the file.
            fragmentShaderFile.close();
            // Shader module creation info.
            VkShaderModuleCreateInfo vkFragmentShaderCreateInfo{};
            vkFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            vkFragmentShaderCreateInfo.codeSize = fragmentShaderBuffer.size();
            vkFragmentShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(fragmentShaderBuffer.data());
            // Create a fragment shader module.
            if (vkCreateShaderModule(vkDevice, &vkFragmentShaderCreateInfo, nullptr, &vkRaygenShaderModule) != VK_SUCCESS) {
                std::cerr << "Failed to create a shader!" << std::endl;
                abort();
            }

            // Create a pipeline stage for a vertex shader.
            vkRaygenShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vkRaygenShaderModuleCreateInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_NV;
            vkRaygenShaderModuleCreateInfo.module = vkRaygenShaderModule;
            // main function name
            vkRaygenShaderModuleCreateInfo.pName = "main";
        }

        VkPipelineShaderStageCreateInfo vkRaymissShaderModuleCreateInfo{};
        VkShaderModule vkRaymissShaderModule;
        {
            // Open file.
            std::ifstream fragmentShaderFile("main2.rmiss.spv", std::ios::ate | std::ios::binary);
            if (!fragmentShaderFile.is_open()) {
                std::cerr << "Raygen shader file not found!" << std::endl;
                abort();
            }
            // Calculate file size.
            size_t fragmentFileSize = static_cast< size_t >(fragmentShaderFile.tellg());
            // Jump to the beginning of the file.
            fragmentShaderFile.seekg(0);
            // Read shader code.
            std::vector< char > fragmentShaderBuffer(fragmentFileSize);
            fragmentShaderFile.read(fragmentShaderBuffer.data(), fragmentFileSize);
            // Close the file.
            fragmentShaderFile.close();
            // Shader module creation info.
            VkShaderModuleCreateInfo vkFragmentShaderCreateInfo{};
            vkFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            vkFragmentShaderCreateInfo.codeSize = fragmentShaderBuffer.size();
            vkFragmentShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(fragmentShaderBuffer.data());
            // Create a fragment shader module.
            if (vkCreateShaderModule(vkDevice, &vkFragmentShaderCreateInfo, nullptr, &vkRaymissShaderModule) != VK_SUCCESS) {
                std::cerr << "Failed to create a shader!" << std::endl;
                abort();
            }

            // Create a pipeline stage for a vertex shader.
            vkRaymissShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vkRaymissShaderModuleCreateInfo.stage = VK_SHADER_STAGE_MISS_BIT_NV;
            vkRaymissShaderModuleCreateInfo.module = vkRaymissShaderModule;
            // main function name
            vkRaymissShaderModuleCreateInfo.pName = "main";
        }

        VkPipelineShaderStageCreateInfo vkRayhitShaderModuleCreateInfo{};
        VkShaderModule vkRayhitShaderModule;
        {
            // Open file.
            std::ifstream fragmentShaderFile("main2.rchit.spv", std::ios::ate | std::ios::binary);
            if (!fragmentShaderFile.is_open()) {
                std::cerr << "Raygen shader file not found!" << std::endl;
                abort();
            }
            // Calculate file size.
            size_t fragmentFileSize = static_cast< size_t >(fragmentShaderFile.tellg());
            // Jump to the beginning of the file.
            fragmentShaderFile.seekg(0);
            // Read shader code.
            std::vector< char > fragmentShaderBuffer(fragmentFileSize);
            fragmentShaderFile.read(fragmentShaderBuffer.data(), fragmentFileSize);
            // Close the file.
            fragmentShaderFile.close();
            // Shader module creation info.
            VkShaderModuleCreateInfo vkFragmentShaderCreateInfo{};
            vkFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            vkFragmentShaderCreateInfo.codeSize = fragmentShaderBuffer.size();
            vkFragmentShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(fragmentShaderBuffer.data());
            // Create a fragment shader module.
            if (vkCreateShaderModule(vkDevice, &vkFragmentShaderCreateInfo, nullptr, &vkRayhitShaderModule) != VK_SUCCESS) {
                std::cerr << "Failed to create a shader!" << std::endl;
                abort();
            }

            // Create a pipeline stage for a vertex shader.
            vkRayhitShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vkRayhitShaderModuleCreateInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
            vkRayhitShaderModuleCreateInfo.module = vkRayhitShaderModule;
            // main function name
            vkRayhitShaderModuleCreateInfo.pName = "main";
        }

        std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages {
            vkRaygenShaderModuleCreateInfo,
            vkRayhitShaderModuleCreateInfo,
            vkRaymissShaderModuleCreateInfo,
        };

        /*
            Setup ray tracing shader groups
        */
        // Indices for the different ray tracing shader types used in this example
        #define INDEX_RAYGEN 0
        #define INDEX_CLOSEST_HIT 1
        #define INDEX_MISS 2

        #define NUM_SHADER_GROUPS 3
        std::array<VkRayTracingShaderGroupCreateInfoNV, NUM_SHADER_GROUPS> groups{};
        for (auto& group : groups) {
            // Init all groups with some default values
            group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
            group.generalShader = VK_SHADER_UNUSED_NV;
            group.closestHitShader = VK_SHADER_UNUSED_NV;
            group.anyHitShader = VK_SHADER_UNUSED_NV;
            group.intersectionShader = VK_SHADER_UNUSED_NV;
        }

        // Links shaders and types to ray tracing shader groups
        groups[INDEX_RAYGEN].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
        groups[INDEX_RAYGEN].generalShader = INDEX_RAYGEN;
        groups[INDEX_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
        groups[INDEX_MISS].generalShader = INDEX_MISS;
        groups[INDEX_CLOSEST_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
        groups[INDEX_CLOSEST_HIT].generalShader = VK_SHADER_UNUSED_NV;
        groups[INDEX_CLOSEST_HIT].closestHitShader = INDEX_CLOSEST_HIT;

        VkRayTracingPipelineCreateInfoNV rayPipelineInfo{};
        rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
        rayPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        rayPipelineInfo.pStages = shaderStages.data();
        rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
        rayPipelineInfo.pGroups = groups.data();
        rayPipelineInfo.maxRecursionDepth = 1;
        rayPipelineInfo.layout = pipelineLayout;
        /*VK_CHECK_RESULT*/(vkCreateRayTracingPipelinesNV(vkDevice, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &pipeline));
    }













    VkBuffer m_shader_binding_table_handle;

    {
        // Create buffer for the shader binding table
        const uint32_t sbtSize = rayTracingProperties.shaderGroupBaseAlignment * NUM_SHADER_GROUPS;





        {
            VkBuffer m_handle;
            VkDeviceMemory m_memory;
            VkDeviceSize m_size;

            // Describe a buffer.
            VkBufferCreateInfo vkVertexBufferInfo{};
            vkVertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vkVertexBufferInfo.size = sbtSize;
            vkVertexBufferInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
            vkVertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            // Create a buffer.
            if (vkCreateBuffer(vkDevice, &vkVertexBufferInfo, nullptr, &m_shader_binding_table_handle) != VK_SUCCESS) {
                std::cerr << "Failed to create a vertex buffer!" << std::endl;
                abort();
            }

            // Retrieve memory requirements for the vertex buffer.
            VkMemoryRequirements vkVertexBufferMemRequirements;
            vkGetBufferMemoryRequirements(vkDevice, m_shader_binding_table_handle, &vkVertexBufferMemRequirements);

            // Define memory allocate info.
            VkMemoryAllocateInfo vkVertexBufferAllocInfo{};
            vkVertexBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            vkVertexBufferAllocInfo.allocationSize = vkVertexBufferMemRequirements.size;
            // Find a suitable memory type.



            VkPhysicalDeviceMemoryProperties vkBufferMemProperties;
            vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkBufferMemProperties);

            uint32_t colorImageMemoryTypeInex = UINT32_MAX;
            for (uint32_t i = 0; i < vkBufferMemProperties.memoryTypeCount; i++) {
                if ((vkVertexBufferMemRequirements.memoryTypeBits & (1 << i)) && (vkBufferMemProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
                    colorImageMemoryTypeInex = i;
                    break;
                }
            }



            vkVertexBufferAllocInfo.memoryTypeIndex = colorImageMemoryTypeInex;
            // Allocate memory for the vertex buffer.
            if (vkAllocateMemory(vkDevice, &vkVertexBufferAllocInfo, nullptr, &m_memory) != VK_SUCCESS) {
                std::cerr << "Failed to allocate memroy for the vertex buffer!" << std::endl;
                abort();
            }

            // Bind the buffer to the allocated memory.
            vkBindBufferMemory(vkDevice, m_shader_binding_table_handle, m_memory, 0);




            // Copy our vertices to the allocated memory.
            void* vertexBufferMemoryData;
            vkMapMemory(vkDevice, m_memory, 0, sbtSize, 0, &vertexBufferMemoryData);



            auto* data = static_cast<uint8_t*>(vertexBufferMemoryData);

            auto shaderHandleStorage = new uint8_t[sbtSize];
            /*VK_CHECK_RESULT*/(vkGetRayTracingShaderGroupHandlesNV(vkDevice, pipeline, 0, NUM_SHADER_GROUPS, sbtSize, shaderHandleStorage));

            for(uint32_t g = 0; g < NUM_SHADER_GROUPS; g++) {
                memcpy(data, shaderHandleStorage + g * rayTracingProperties.shaderGroupHandleSize, rayTracingProperties.shaderGroupHandleSize);  // raygen
                data += rayTracingProperties.shaderGroupBaseAlignment;
            }



            vkUnmapMemory(vkDevice, m_memory);
        }



}


    // ==========================================================================
    //                      STEP 31: Create uniform buffers
    // ==========================================================================
    // Uniform buffer contains structures that are provided to shaders
    // as uniform variable. In our case this is a couple of matrices.
    // As we expect to have more than one frame rendered at the same time
    // and we are going to update this buffer every frame, we should avoid
    // situation when one frame reads the uniform buffer while it is
    // being updated. So we should create one buffer per swap chain image.
    // ==========================================================================

    // Structure that we want to provide to the vertext shader.
    struct UniformBufferObject {
        glm::mat4 viewInv;
        glm::mat4 projInv;
    };

    // Get size of the uniform buffer.
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // Uniform buffers.
    VkBuffer vkUniformBuffer;

    // Memory of uniform buffers.
    VkDeviceMemory vkUniformBuffersMemory;


    {
        // Create one uniform buffer per swap chain image.
        // Describe a buffer.
        VkBufferCreateInfo vkBufferInfo{};
        vkBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vkBufferInfo.size = bufferSize;
        vkBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        vkBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // Create a buffer.
        if (vkCreateBuffer(vkDevice, &vkBufferInfo, nullptr, &vkUniformBuffer) != VK_SUCCESS) {
            std::cerr << "Failed to create a buffer!" << std::endl;
            abort();
        }

        // Retrieve memory requirements for the vertex buffer.
        VkMemoryRequirements vkMemRequirements;
        vkGetBufferMemoryRequirements(vkDevice, vkUniformBuffer, &vkMemRequirements);

        // Define memory allocate info.
        VkMemoryAllocateInfo vkAllocInfo{};
        vkAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkAllocInfo.allocationSize = vkMemRequirements.size;
        // Select suitable memory type.
        uint32_t memTypeIndex = UINT32_MAX;
        VkMemoryPropertyFlags vkMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        VkPhysicalDeviceMemoryProperties vkMemProperties;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkMemProperties);
        for (uint32_t i = 0; i < vkMemProperties.memoryTypeCount; i++) {
            if ((vkMemRequirements.memoryTypeBits & (1 << i)) && (vkMemProperties.memoryTypes[i].propertyFlags & vkMemFlags) == vkMemFlags) {
                memTypeIndex = i;
                break;
            }
        }
        vkAllocInfo.memoryTypeIndex = memTypeIndex;

        // Allocate memory for the vertex buffer.
        if (vkAllocateMemory(vkDevice, &vkAllocInfo, nullptr, &vkUniformBuffersMemory) != VK_SUCCESS) {
            std::cerr << "Failed to allocate buffer memory!" << std::endl;
            abort();
        }

        // Bind the buffer to the allocated memory.
        vkBindBufferMemory(vkDevice, vkUniformBuffer, vkUniformBuffersMemory, 0);
    }



    // Update uniform buffer object.
    UniformBufferObject ubo{};
    //ubo.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.viewInv = glm::inverse(glm::lookAt(glm::vec3(2.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    float aspectRatio = static_cast< float >(vkSelectedExtent.width) / vkSelectedExtent.height;
    ubo.projInv = glm::inverse(glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f));

    // Write the uniform buffer object.
    void* data;
    vkMapMemory(vkDevice, vkUniformBuffersMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(vkDevice, vkUniformBuffersMemory);



































        VkDescriptorPool descriptorPool;
        VkDescriptorSet descriptorSet;




        {
//            m_uniforms_rtx = std::make_shared< vka::uniforms >(this);
//            m_uniforms_rtx->add_uniform< UniformBufferObject >("ubo", VK_SHADER_STAGE_VERTEX_BIT, 1);
//            m_uniforms_rtx->create_descriptor_set_layout();
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
            /*VK_CHECK_RESULT*/(vkCreateDescriptorPool(vkDevice, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
            descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocateInfo.descriptorPool = descriptorPool;
            descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
            descriptorSetAllocateInfo.descriptorSetCount = 1;
            /*VK_CHECK_RESULT*/(vkAllocateDescriptorSets(vkDevice, &descriptorSetAllocateInfo, &descriptorSet));

            VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo{};
            descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
            descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
            descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.accelerationStructure;

            VkWriteDescriptorSet accelerationStructureWrite{};
            accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            // The specialized acceleration structure descriptor has to be chained
            accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
            accelerationStructureWrite.dstSet = descriptorSet;
            accelerationStructureWrite.dstBinding = 0;
            accelerationStructureWrite.descriptorCount = 1;
            accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

            VkDescriptorImageInfo storageImageDescriptor{};
            storageImageDescriptor.imageView = m_storage_image_view;
            storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet resultImageWrite {};
            resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            resultImageWrite.dstSet = descriptorSet;
            resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            resultImageWrite.dstBinding = 1;
            resultImageWrite.pImageInfo = &storageImageDescriptor;
            resultImageWrite.descriptorCount = 1;

            VkDescriptorBufferInfo vkBufferInfo{};
            vkBufferInfo.buffer = vkUniformBuffer;
            vkBufferInfo.offset = 0;
            vkBufferInfo.range = bufferSize;

            VkWriteDescriptorSet uniformBufferWrite {};
            uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uniformBufferWrite.dstSet = descriptorSet;
            uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniformBufferWrite.dstBinding = 2;
            uniformBufferWrite.pBufferInfo = &vkBufferInfo;
            uniformBufferWrite.descriptorCount = 1;

            std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                accelerationStructureWrite,
                resultImageWrite,
                uniformBufferWrite
            };
            vkUpdateDescriptorSets(vkDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
        }











        //shaderBindingTable = std::make_shared< vka::buffer >(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, sbtSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);



//        shaderBindingTable->map([&](void* mappedData){
//            auto* data = static_cast<uint8_t*>(mappedData);

//            auto shaderHandleStorage = new uint8_t[sbtSize];
//            VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesNV(vkDevice, pipeline, 0, NUM_SHADER_GROUPS, sbtSize, shaderHandleStorage));

//            for(uint32_t g = 0; g < NUM_SHADER_GROUPS; g++) {
//                memcpy(data, shaderHandleStorage + g * rayTracingProperties.shaderGroupHandleSize, rayTracingProperties.shaderGroupHandleSize);  // raygen
//                data += rayTracingProperties.shaderGroupBaseAlignment;
//            }
//        });





























        // ==========================================================================
        //                 STEP 26: Create swap chain image views
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
        //                    STEP 33: Create command buffers
        // ==========================================================================
        // Command buffers describe a set of rendering commands submitted to Vulkan.
        // We need to have one buffer per each image in the swap chain.
        // Command buffers are taken from the command pool, so we should
        // create one.
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


        VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

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


                /*
                    Dispatch the ray tracing commands
                */
                vkCmdBindPipeline(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline);
                vkCmdBindDescriptorSets(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

                // Calculate shader binding offsets, which is pretty straight forward in our example
                VkDeviceSize bindingOffsetRayGenShader = rayTracingProperties.shaderGroupBaseAlignment * INDEX_RAYGEN;
                VkDeviceSize bindingOffsetMissShader = rayTracingProperties.shaderGroupBaseAlignment * INDEX_MISS;
                VkDeviceSize bindingOffsetHitShader = rayTracingProperties.shaderGroupBaseAlignment * INDEX_CLOSEST_HIT;
                VkDeviceSize bindingStride = rayTracingProperties.shaderGroupBaseAlignment; // TODO: or shaderGroupBaseAlignment

                vkCmdTraceRaysNV(vkCommandBuffers[i],
                    m_shader_binding_table_handle, bindingOffsetRayGenShader,
                    m_shader_binding_table_handle, bindingOffsetMissShader, bindingStride,
                    m_shader_binding_table_handle, bindingOffsetHitShader, bindingStride,
                    VK_NULL_HANDLE, 0, 0,
                    vkSelectedExtent.width, vkSelectedExtent.height, 1);

                /*
                    Copy raytracing output to swap chain image
                */

                // Prepare current swapchain image as transfer destination
                setImageLayout(
                    vkCommandBuffers[i],
                    vkSwapChainImages[i],
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    subresourceRange);

                // Prepare ray tracing output image as transfer source
                setImageLayout(
                    vkCommandBuffers[i],
                    m_storage_image,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    subresourceRange);

                VkImageCopy copyRegion{};
                copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
                copyRegion.srcOffset = { 0, 0, 0 };
                copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
                copyRegion.dstOffset = { 0, 0, 0 };
                copyRegion.extent = { vkSelectedExtent.width, vkSelectedExtent.height, 1 };
                vkCmdCopyImage(vkCommandBuffers[i], m_storage_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkSwapChainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                // Transition swap chain image back for presentation
                setImageLayout(
                    vkCommandBuffers[i],
                    vkSwapChainImages[i],
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    subresourceRange);

                // Transition ray tracing output image back to general layout
                setImageLayout(
                    vkCommandBuffers[i],
                    m_storage_image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_GENERAL,
                    subresourceRange);


                // Fihish adding commands into the buffer.
                if (vkEndCommandBuffer(vkCommandBuffers[i]) != VK_SUCCESS) {
                    std::cerr << "Failed to finish command buffer recording" << std::endl;
                    abort();
                }
            }








    // ==========================================================================
    //                   STEP 34: Synchronization primitives
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
    //                     STEP 35: Pick graphics queues
    // ==========================================================================
    // We have checked that the device supports all required queues, but now
    // we need to pick their handles explicitly.
    // ==========================================================================


    // Pick a present queue.
    // It might happen that both handles refer to the same queue.
    VkQueue vkPresentQueue;
    vkGetDeviceQueue(vkDevice, queueFamilyIndices.presentFamily.value(), 0, &vkPresentQueue);

    // ==========================================================================
    //                         STEP 36: Main loop
    // ==========================================================================
    // Main loop performs event hanlding and executes rendering.
    // ==========================================================================

    // Index of a framce processed in the current loop.
    // We go through MAX_FRAMES_IN_FLIGHT indices.
    size_t currentFrame = 0;

    // Initial value of the system timer we use for rotation animation.
    auto startTime = std::chrono::high_resolution_clock::now();

    // Main loop.
    while(!glfwWindowShouldClose(glfwWindow)) {
        // Poll GLFW events.
        glfwPollEvents();

        // Wait for the current frame.
        vkWaitForFences(vkDevice, 1, &vkInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        // Aquire a next image from a swap chain to process.
        uint32_t imageIndex;
        vkAcquireNextImageKHR(vkDevice, vkSwapChain, UINT64_MAX, vkImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // Calculate time difference and rotation angle.
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration< float, std::chrono::seconds::period >(currentTime - startTime).count();
        float angle = time * glm::radians(90.0f);

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
    //                     STEP 37: Deinitialization
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

    // Destroy uniform buffer.
    vkDestroyBuffer(vkDevice, vkUniformBuffer, nullptr);
    vkFreeMemory(vkDevice, vkUniformBuffersMemory, nullptr);

    // Destroy vertex buffer.
    vkDestroyBuffer(vkDevice, vkVertexBuffer, nullptr);
    vkFreeMemory(vkDevice, vkVertexBufferMemory, nullptr);

    // Destory command pool
    vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);


    // Destroy shader modules.
    vkDestroyShaderModule(vkDevice, vkFragmentShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkVertexShaderModule, nullptr);

    // Destory swap chain image views.
    for (auto imageView : vkSwapChainImageViews) {
        vkDestroyImageView(vkDevice, imageView, nullptr);
    }

    // Destroy swap chain.
    vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);

    // Destory descriptor set layout for uniforms.
    vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);

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
