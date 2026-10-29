

//#include "vulkan/vulkan.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define VOLK_IMPLEMENTATION
#include "Volk/volk.h"

#define VMA_IMPLEMENTATION
//#define VMA_STATIC_VULKAN_FUNCTIONS 0
//#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vma/vk_mem_alloc.h"

#define IMGUI_IMPL_VULKAN_USE_VOLK


#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"


#include <glm/gtx/string_cast.hpp>


#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>

#include "glfw3.h"

#include "C:/Users/cyro1/source/repos/irrKlang/include/irrKlang.h"

#include <glm/gtx/string_cast.hpp>

static bool first = true;


const std::string m_MODEL_PATH = "models/viking_room.obj";
const std::string m_TEXTURE_PATH = "textures/viking_room.png";

const std::string m_cube_MODEL_PATH = "models/cube.obj";
const std::string m_cube_TEXTURE_PATH = "models/wood_tex.png";

const std::string m_sphere_MODEL_PATH = "models/SphereUvWrapped.obj";
const std::string m_sphere_TEXTURE_PATH = "models/sphere_tex.png";

const std::string m_monkey_MODEL_PATH = "models/Other/MonkeyOBJ.obj";
const std::string m_monkey_TEXTURE_PATH = "models/Other/body_u1_v1.png";

const std::string m_plane_MODEL_PATH = "models/Other/plane_t_n_s.obj";
const std::string m_plane_TEXTURE_PATH = "models/Other/plane.jpg"; //plane.jpg grid.png grid_bad.jpg  Forest-Ground.png

const std::string m_tree_MODEL_PATH = "models/tree3.obj";
const std::string m_tree_TEXTURE_PATH = "models/tree3.png";

const std::string m_slime_MODEL_PATH = "models/slimemodel.obj";
const std::string m_slime_TEXTURE_PATH = "models/slime_texture.png";


const int MAX_FRAMES_IN_FLIGHT = 2;


struct Camera {
    glm::vec3 position = glm::vec3(3.0f, -3.0f, 2.0f);
    glm::vec3 lookat = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);

    glm::mat4 getViewmatrix() {
        return glm::lookAt(position, lookat, up);
    }
};

struct Player {
    glm::vec3 position = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 lookat = glm::vec3(0.0f, 0.0f, 1.0f);
};

Camera camera;
Player player;

static int spheres_restored = 0;
static float game_time = 50;
static float time_interval = 1;

static bool game_over = false;
static bool game_won = false;

static bool music_victory = false;
static bool music_lost = false;


irrklang::ISoundEngine* Sound = irrklang::createIrrKlangDevice();

glm::vec3 camera_direction = glm::normalize(camera.lookat - camera.position);

const std::vector<const char*> m_validationLayers = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_monitor"
};

std::vector<const char*> m_sdl_instance_extensions = {};

const std::vector<const char*> m_deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG 
const bool enableValidationLayers = true; //hier eigtl false
#else
const bool enableValidationLayers = true;
#endif


//ImGUI
static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

VkInstance _instance;

auto loadVolk(const char* name, void* context) {
    return vkGetInstanceProcAddr(_instance, name);
}


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo
    , const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger
    , const VkAllocationCallbacks* pAllocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
    }
};

namespace std {

    template<> struct hash<glm::vec2> {
        size_t operator()(glm::vec2 const& vec) const {
            return (hash<float>()(vec.x) ^ (hash<float>()(vec.y) << 1)) >> 1;
        }
    };

    template<> struct hash<glm::vec3> {
        size_t operator()(glm::vec3 const& vec) const {
            return ((hash<float>()(vec.x) ^ (hash<float>()(vec.y) << 1)) >> 1) ^ (hash<float>()(vec.z) << 1);
        }
    };

    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) 
                ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}


//--------------------------------------------------------------------------------------------------


class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    VmaAllocator m_vmaAllocator;

    SDL_Window* m_sdlWindow{nullptr};
    bool m_isMinimized = false;
    bool m_quit = false;

    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR m_surface;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device;

    QueueFamilyIndices m_queueFamilies;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    struct SwapChain {
        VkSwapchainKHR m_swapChain;
        std::vector<VkImage> m_swapChainImages;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImageView> m_swapChainImageViews;
        std::vector<VkFramebuffer> m_swapChainFramebuffers;
    } m_swapChain;

    VkRenderPass m_renderPass;
    VkDescriptorSetLayout m_descriptorSetLayout;

    struct Pipeline {
        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_pipeline;
    } m_graphicsPipeline;

    Pipeline m_specialGraphicsPipeline;

    Pipeline m_floorGraphicsPipeline;

    

    struct DepthImage {
        VkImage         m_depthImage;
        VmaAllocation   m_depthImageAllocation;
        VkImageView     m_depthImageView;
    } m_depthImage;

	//The texture of an object
    struct Texture {
        VkImage         m_textureImage;
        VmaAllocation   m_textureImageAllocation;
        VkImageView     m_textureImageView;
        VkSampler       m_textureSampler;
    };

	//Mesh of an object
    struct Geometry {
        std::vector<Vertex>     m_vertices;
        std::vector<uint32_t>   m_indices;
        VkBuffer                m_vertexBuffer;
        VmaAllocation           m_vertexBufferAllocation;
        VkBuffer                m_indexBuffer;
        VmaAllocation           m_indexBufferAllocation;
    };

	//Uniform buffers of an object
    struct UniformBuffers {
        std::vector<VkBuffer>       m_uniformBuffers;
        std::vector<VmaAllocation>  m_uniformBuffersAllocation;
        std::vector<void*>          m_uniformBuffersMapped;
    };

    VkDescriptorPool m_descriptorPool;

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
        alignas(16) glm::mat4 viewInv;
	};
		
	//This holds all information an object with texture needs!	
	struct Object {
		UniformBufferObject m_ubo; //holds model, view and proj matrix
		UniformBuffers m_uniformBuffers;
		Texture m_texture;
		Geometry m_geometry;
		std::vector<VkDescriptorSet> m_descriptorSets;
	};

	std::vector<Object> m_objects;

    std::vector<Object> standard_objects;
    std::vector<Object> special_objects;
    std::vector<Object> floor_objects;

    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;

    struct SyncObjects {
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence>     m_inFlightFences;
    } m_syncObjects;

    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;


    void initWindow() {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
        #ifdef SDL_HINT_IME_SHOW_UI
            SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
        #endif

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        m_sdlWindow = SDL_CreateWindow("Slime Ranger", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
    }

    void initVMA(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& allocator) {
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        allocatorCreateInfo.physicalDevice = physicalDevice;
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.instance = instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
        vmaCreateAllocator(&allocatorCreateInfo, &allocator);
    }

    //added texrepeat for texture repeating in wanted cases
	void createObject( VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator, 
			VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout,
			glm::mat4&& model, std::string modelPath, std::string texturePath, std::vector<Object>& objects, float tex_repeat) {

		Object object{{model}};
		createTextureImage(physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, texturePath, object.m_texture);
        createTextureImageView(device, object.m_texture);
        createTextureSampler(physicalDevice, device, object.m_texture);
        loadModel(object.m_geometry, modelPath, tex_repeat);
        std::vector<Vertex> vertices = object.m_geometry.m_vertices;
        std::vector<uint32_t> indices = object.m_geometry.m_indices;
        std::vector<Vertex> newVertices = {};
        for (uint32_t index : indices) newVertices.push_back(vertices[index]);
        object.m_geometry.m_vertices = newVertices;
        createVertexBuffer(physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, object.m_geometry);
        createIndexBuffer( physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, object.m_geometry);
        
        createUniformBuffers(physicalDevice, device, vmaAllocator, object.m_uniformBuffers);
        
        createDescriptorSets(device, object.m_texture, descriptorSetLayout, object.m_uniformBuffers, descriptorPool, object.m_descriptorSets);
		objects.push_back(object);
	}

    void createTree(float x, float y) {
        createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool,
            m_descriptorPool, m_descriptorSetLayout, glm::mat4{ 1.0f }, m_tree_MODEL_PATH, m_tree_TEXTURE_PATH, m_objects, 1.0);
        if (m_objects.empty() == false) {
            m_objects.back().m_ubo.model = glm::translate(glm::mat4(0.5f), glm::vec3(x, y, 0));
            m_objects.back().m_ubo.model = glm::rotate(m_objects.back().m_ubo.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

            standard_objects.push_back(m_objects.back());
        }
    }

    void createSphere(float x, float y) {
        createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool,
            m_descriptorPool, m_descriptorSetLayout, glm::mat4{ 1.0f }, m_sphere_MODEL_PATH, m_sphere_TEXTURE_PATH, m_objects, 1.0);

        if (m_objects.empty() == false) {

            glm::mat4 sphere_transform = glm::mat4(0.1f);
            sphere_transform = glm::translate(sphere_transform, glm::vec3(x, y, 10.0f));
            m_objects.back().m_ubo.model = sphere_transform * m_objects.back().m_ubo.model;

            //standard_objects = m_objects;
            //standard_objects.pop_back();
            special_objects.push_back(m_objects.back());
        }
    }

    void initVulkan() {
        createInstance(&m_instance, m_validationLayers);
        setupDebugMessenger(m_instance);
        createSurface(m_instance, m_surface);
        pickPhysicalDevice(m_instance, m_deviceExtensions, m_surface, m_physicalDevice);
        createLogicalDevice(m_surface, m_physicalDevice, m_queueFamilies, m_validationLayers, m_deviceExtensions, m_device, m_graphicsQueue, m_presentQueue);
        initVMA(m_instance, m_physicalDevice, m_device, m_vmaAllocator);  
        createSwapChain(m_surface, m_physicalDevice, m_device, m_swapChain);
        createImageViews(m_device, m_swapChain);
        createRenderPass(m_physicalDevice, m_device, m_swapChain, m_renderPass);
        createDescriptorSetLayout(m_device, m_descriptorSetLayout);
        createGraphicsPipeline(m_device, m_renderPass, m_descriptorSetLayout, m_graphicsPipeline);
        createSpecialGraphicsPipeline(m_device, m_renderPass, m_descriptorSetLayout, m_specialGraphicsPipeline);
        createFloorGraphicsPipeline(m_device, m_renderPass, m_descriptorSetLayout, m_floorGraphicsPipeline);
        createCommandPool(m_surface, m_physicalDevice, m_device, m_commandPool);
        createDepthResources(m_physicalDevice, m_device, m_vmaAllocator, m_swapChain, m_depthImage);
        createFramebuffers(m_device, m_swapChain, m_depthImage, m_renderPass);
        createDescriptorPool(m_device, m_descriptorPool);
        std::cout << std::endl;
        std::cout << "UNIFORM BUFFERS QUERY" << std::endl << std::endl;

		createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, 
			m_descriptorPool, m_descriptorSetLayout, glm::mat4{1.0f}, m_MODEL_PATH, m_TEXTURE_PATH, m_objects, 1.0);

        standard_objects.push_back(m_objects[0]);

        //glm::mat4 cube_transform = glm::mat4(1.0f);
        //cube_transform = glm::scale(cube_transform, glm::vec3(0.5f, 0.5f, 0.5f));
        //m_objects[0].m_ubo.model = cube_transform * m_objects[0].m_ubo.model;

        /*
        createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool,
            m_descriptorPool, m_descriptorSetLayout, glm::mat4{ 0.005f }, m_monkey_MODEL_PATH, m_monkey_TEXTURE_PATH, m_objects);

        
        glm::mat4 transformation = glm::mat4(1.0f);
        
        transformation = glm::rotate(transformation, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        transformation = glm::rotate(transformation, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        transformation = glm::translate(transformation, glm::vec3(120.0f, 0.0f, 220.0f));
        m_objects[1].m_ubo.model = transformation * m_objects[1].m_ubo.model;


        */
        /*
        createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool,
            m_descriptorPool, m_descriptorSetLayout, glm::mat4{ 1.0f }, m_cube_MODEL_PATH, m_cube_TEXTURE_PATH, m_objects, 1.0);

        glm::mat4 cube_transform = glm::mat4(1.0f);
        cube_transform = glm::scale(cube_transform, glm::vec3(0.5f, 0.5f, 0.5f));
        cube_transform = glm::translate(cube_transform, glm::vec3(4.0f, 4.0f, 0.0f));
        m_objects[1].m_ubo.model = cube_transform * m_objects[1].m_ubo.model;

        */
        
        createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool,
            m_descriptorPool, m_descriptorSetLayout, glm::mat4{ 100.0f }, m_plane_MODEL_PATH, m_plane_TEXTURE_PATH, m_objects, 100.0); //m_monkey_TEXTURE_PATH, 
        
        glm::mat4 transformation2 = glm::mat4(1.0f);
        transformation2 = glm::rotate(transformation2, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m_objects[1].m_ubo.model = transformation2 * m_objects[1].m_ubo.model;

        //standard_objects = m_objects;
        //standard_objects.pop_back();
        floor_objects.push_back(m_objects[1]);

        /*
        createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool,
            m_descriptorPool, m_descriptorSetLayout, glm::mat4{ 0.1f }, m_sphere_MODEL_PATH, m_sphere_TEXTURE_PATH, m_objects, 1.0);

        glm::mat4 sphere_transform = glm::mat4(1.0f);
        sphere_transform = glm::translate(sphere_transform, glm::vec3(0.0f, -15.0f, 10.0f));
        m_objects[3].m_ubo.model = sphere_transform * m_objects[3].m_ubo.model;

        //standard_objects = m_objects;
        //standard_objects.pop_back();
        special_objects.push_back(m_objects[3]);

        

        createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool,
            m_descriptorPool, m_descriptorSetLayout, glm::mat4{ 0.2f }, m_sphere_MODEL_PATH, m_sphere_TEXTURE_PATH, m_objects, 1.0);

        glm::mat4 sphere2_transform = glm::mat4(1.0f);
        sphere2_transform = glm::translate(sphere2_transform, glm::vec3(-10.0f, -10.0f, 5.0f));
        m_objects[4].m_ubo.model[3] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); //sphere2_transform* m_objects[4].m_ubo.model;

        standard_objects.push_back(m_objects[4]);

        

        createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool,
            m_descriptorPool, m_descriptorSetLayout, glm::mat4{ 1.0f }, m_tree_MODEL_PATH, m_tree_TEXTURE_PATH, m_objects, 1.0);
        m_objects[5].m_ubo.model = glm::translate(glm::mat4(0.5f), glm::vec3(5, 5, 0));
        m_objects[5].m_ubo.model = glm::rotate(m_objects[5].m_ubo.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        standard_objects.push_back(m_objects[5]);

        */

        createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool,
            m_descriptorPool, m_descriptorSetLayout, glm::mat4{ 1.0f }, m_slime_MODEL_PATH, m_slime_TEXTURE_PATH, m_objects, 1.0);
        m_objects[2].m_ubo.model = glm::translate(glm::mat4(0.1f), glm::vec3(15, 15, 1));
        m_objects[2].m_ubo.model = glm::rotate(m_objects[2].m_ubo.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        standard_objects.push_back(m_objects[2]);

        //standard_objects.push_back(m_objects[1]);

        createSphere(3, 3);
        createSphere(35, 35);
        createSphere(-47, -23);
        createSphere(20, -12);
        createSphere(80, -73);


        createTree(10, 10);
        createTree(14, 21);
        createTree(3, 7);
        createTree(51, 12);
        createTree(-12, -3);
        createTree(-24, -31);

        createTree(13, 11);
        createTree(19, 25);
        createTree(32, 71);
        createTree(51, 42);
        createTree(-22, -53);
        createTree(-4, -37);

        createTree(-10, -70);
        createTree(-14, -1);
        createTree(-3, 7);
        createTree(-51, -12);
        createTree(-12, 96);
        createTree(-34, -35);

        createTree(100, 100);
        createTree(1, 2);
        createTree(43, 47);
        createTree(21, 82);
        createTree(-17, -43);
        createTree(-84, -2);

        createTree(-13, -11);
        createTree(19, -25);
        createTree(-32, 71);
        createTree(51, -42);
        createTree(22, 53);
        createTree(-4, 87);

        createTree(-10, 170);
        createTree(124, -19);
        createTree(-33, 79);
        createTree(-81, -52);
        createTree(-62, 9);
        createTree(-24, -3);

        createTree(10, -17);
        createTree(29, -24);
        createTree(32, 75);
        createTree(41, -49);
        createTree(52, 23);
        createTree(64, 7);

        createTree(-70, 17);
        createTree(184, -129);
        createTree(-93, 9);
        createTree(-31, -2);
        createTree(-22, 94);




        
        std::cout << std::endl << "2 Buffers created for each object (house, ape, and plane)." << std::endl;
        std::cout << "This is presumably because of parallel operations in CPU and GPU (2 frames in flight). The GPU can render one image, while the CPU is preparing the next one. Both need one UBO for each object." << std::endl;
        std::cout << "The following flags are used: VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_ALLOCATION_CREATE_MAPPED_BIT" << std::endl;
        std::cout << "The VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT signals that the type of the buffer is uniform buffer." << std::endl;
        std::cout << std::endl;
        createCommandBuffers(m_device, m_commandPool, m_commandBuffers);
        createSyncObjects(m_device, m_syncObjects);
        setupImgui(m_instance, m_physicalDevice, m_queueFamilies, m_device, m_graphicsQueue, m_commandPool
            , m_descriptorPool, m_renderPass);

        
        
    }

    


    void mainLoop() {
        if (game_over == false && game_won == false) Sound->play2D("sound/forest.wav", true);
        SDL_Event event;
        SDL_PollEvent(&event);
        //Camera camera;
        //game_time -= event.dt;

        while (!m_quit) {
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event); // Forward your event to backend
                
                if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE )
                    m_quit = true;

                if (event.type == SDL_WINDOWEVENT) {
                    switch (event.window.event) {
                    case SDL_WINDOWEVENT_MINIMIZED: 
                        m_isMinimized = true;
                        break;

                    case SDL_WINDOWEVENT_MAXIMIZED: 
                        m_isMinimized = false;
                        break;

                    case SDL_WINDOWEVENT_RESTORED:
                        m_isMinimized = false;
                        break;
                    }
                }

               
            }
            if (game_over == false && game_won == false) {
                getKeyboardInput(camera, player, standard_objects, special_objects, floor_objects, space_pressed); //event, 
            }
            
            if(!m_isMinimized) {
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplSDL2_NewFrame();
                ImGui::NewFrame();

                //ImGui::ShowDemoWindow(); // Show demo window! :)

                ImGui::SetNextWindowSize(ImVec2(200, 100));
                ImGui::Begin("Game Status");

                ImGui::Text("Spheres Restored: %d", spheres_restored);
                ImGui::Text("Time Left: %.2f", game_time);

                if (game_over == true) ImGui::Text("GAME OVER");

                if (game_over == true && music_lost == false) {
                    Sound->removeAllSoundSources();
                    Sound->play2D("sound/lost.wav", false);
                    
                    music_lost = true;
                }

                if (game_won == true) ImGui::Text("GAME WON");

                if (game_won == true && music_victory == false) {
                    Sound->removeAllSoundSources();
                    Sound->play2D("sound/victory.wav", true);
                    
                    music_victory = true;
                }

                ImGui::End();

                


                
                drawFrame(m_sdlWindow, m_surface, m_physicalDevice, m_device, m_vmaAllocator
                    , m_graphicsQueue, m_presentQueue, m_swapChain, m_depthImage
                    , m_renderPass, m_graphicsPipeline, m_specialGraphicsPipeline, m_floorGraphicsPipeline, standard_objects, special_objects, floor_objects, m_commandBuffers
					, m_syncObjects, m_currentFrame, m_framebufferResized);

                //bad original idea: calling drawframe() two times -> extreme flickering

                /*
                drawFrame(m_sdlWindow, m_surface, m_physicalDevice, m_device, m_vmaAllocator
                    , m_graphicsQueue, m_presentQueue, m_swapChain, m_depthImage
                    , m_renderPass, m_specialGraphicsPipeline, special_objects, m_commandBuffers
                    , m_syncObjects, m_currentFrame, m_framebufferResized);
                */
            }
        }
        vkDeviceWaitIdle(m_device);
    }

    void cleanupSwapChain(VkDevice device, VmaAllocator vmaAllocator, SwapChain& swapChain, DepthImage& depthImage) {
        vkDestroyImageView(device, depthImage.m_depthImageView, nullptr);

        destroyImage(device, vmaAllocator, depthImage.m_depthImage, depthImage.m_depthImageAllocation);

        for (auto framebuffer : swapChain.m_swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (auto imageView : swapChain.m_swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain.m_swapChain, nullptr);
    }

    void cleanup() {

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        cleanupSwapChain(m_device, m_vmaAllocator, m_swapChain, m_depthImage);

        vkDestroyPipeline(m_device, m_graphicsPipeline.m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicsPipeline.m_pipelineLayout, nullptr);
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);


		for( auto& object : m_objects) {
	        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
	            destroyBuffer(m_device, m_vmaAllocator, object.m_uniformBuffers.m_uniformBuffers[i], object.m_uniformBuffers.m_uniformBuffersAllocation[i]);
	        }

	        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

	        vkDestroySampler(m_device, object.m_texture.m_textureSampler, nullptr);
	        vkDestroyImageView(m_device, object.m_texture.m_textureImageView, nullptr);

	        destroyImage(m_device, m_vmaAllocator, object.m_texture.m_textureImage, object.m_texture.m_textureImageAllocation);

	        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

	        destroyBuffer(m_device, m_vmaAllocator, object.m_geometry.m_indexBuffer, object.m_geometry.m_indexBufferAllocation);

	        destroyBuffer(m_device, m_vmaAllocator, object.m_geometry.m_vertexBuffer, object.m_geometry.m_vertexBufferAllocation);
		}

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_device, m_syncObjects.m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_syncObjects.m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_device, m_syncObjects.m_inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(m_device, m_commandPool, nullptr);

        vmaDestroyAllocator(m_vmaAllocator);

        vkDestroyDevice(m_device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);

        SDL_DestroyWindow(m_sdlWindow);
        SDL_Quit();

    }

    void recreateSwapChain(SDL_Window* window, VkSurfaceKHR surface
        , VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator, SwapChain& swapChain
        , DepthImage& depthImage, VkRenderPass renderPass) {
        int width = 0, height = 0;
        
        SDL_GetWindowSize(window, &width, &height);
        while (width == 0 || height == 0) {
            SDL_Event event;
            SDL_WaitEvent(&event);
            SDL_GetWindowSize(window, &width, &height);
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain(device, vmaAllocator, swapChain, depthImage);

        createSwapChain(surface, physicalDevice, device, swapChain);
        createImageViews(device, swapChain);
        createDepthResources(physicalDevice, device, vmaAllocator, swapChain, depthImage);
        createFramebuffers(device, swapChain, depthImage, renderPass);
    }


    void createInstance(VkInstance *instance, const std::vector<const char*>& validationLayers) {
        volkInitialize();

        if (enableValidationLayers && !checkValidationLayerSupport( validationLayers)) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Tutorial";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        uint32_t major = VK_VERSION_MAJOR(appInfo.apiVersion);
        uint32_t minor = VK_VERSION_MINOR(appInfo.apiVersion);
        uint32_t patch = VK_VERSION_PATCH(appInfo.apiVersion);
        // Output the highest supported Vulkan API version.
        std::cout << "\n" << "Highest Vulkan API Version Supported: " << major << "." << minor << "." << patch << std::endl << std::endl;

        

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
        _instance = *instance;

   		volkLoadInstance(*instance);       
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger(VkInstance instance) {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface(VkInstance instance, VkSurfaceKHR& surface) {
        if (SDL_Vulkan_CreateSurface(m_sdlWindow, instance, &surface) == 0) {
            printf("Failed to create Vulkan surface.\n");
        }
    }

    void pickPhysicalDevice(VkInstance instance, const std::vector<const char*>& deviceExtensions, VkSurfaceKHR surface, VkPhysicalDevice& physicalDevice) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::cout << "\n" << "Number of physical devices: " << deviceCount << "\n";

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            VkPhysicalDeviceProperties2 deviceProperties2{};
            deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            vkGetPhysicalDeviceProperties2(device, &deviceProperties2);

            std::cout << "\nDevice Name: ";
            for (char c : deviceProperties2.properties.deviceName) {
                std::cout << c;
                
            }
            std::cout << std::endl;
            std::cout << "Vendor ID: " << deviceProperties2.properties.vendorID << "\n";
            if (deviceProperties2.properties.vendorID == 4318) std::cout << "-> this is NVidia\n";
            std::cout << "Device Type: ";
            switch (deviceProperties2.properties.deviceType) {
            case 0: std::cout << "other"; 
                break;
            case 1: std::cout << "integrated GPU"; 
                break;
            case 2: std::cout << "discrete GPU"; 
                break;
            case 3: std::cout << "virtual GPU"; 
                break;
            case 4: std::cout << "CPU"; 
                break;
            }
            std::cout << std::endl;

            VkPhysicalDeviceFeatures supportedFeatures;
            vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

            std::cout << "Anisotropic Sampling supported: ";
            if (supportedFeatures.samplerAnisotropy == VK_TRUE) std::cout << "Yes\n";
            else std::cout << "No\n";
            std::cout << std::endl;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            std::cout << "Number of Queue Families: " << queueFamilyCount << std::endl;
            int county = 1;
            for (auto qfp : queueFamilies) {
                std::cout << "Queue Number " << county << ":\n";
                std::cout << "Queue Flag Value: " << qfp.queueFlags << std::endl;
                if (qfp.queueFlags == 15) std::cout << "Graphics & Compute & Transfer & Sparse Memory Operation Support\n";
                else if (qfp.queueFlags == 4) std::cout << "Transfer Operation Support\n";
                std::cout << "Number of possible instances: " << qfp.queueCount << std::endl;
                county++;
            }
            std::cout << std::endl;

            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::cout << "Number of available Extensions: " << extensionCount << "\n\n";

            int e_count = 1;

            std::vector<std::string> e_descriptions;
            e_descriptions.push_back("The VK_KHR_swapchain extension is the device - level companion to the VK_KHR_surface extension.It introduces VkSwapchainKHR objects, which provide the ability to present rendering results to a surface.");
            e_descriptions.push_back("The VK_KHR_16bit_storage extension allows use of 16-bit types in shader input and output interfaces, and push constant blocks. This extension introduces several new optional features which map to SPIR-V capabilities and allow access to 16-bit data in Block-decorated objects in the Uniform and the StorageBuffer storage classes, and objects in the PushConstant storage class. This extension allows 16-bit variables to be declared and used as user-defined shader inputs and outputs but does not change location assignment and component assignment rules.");
            e_descriptions.push_back("This extension provides versions of vkBindBufferMemory and vkBindImageMemory that allow multiple bindings to be performed at once, and are extensible.");
            for (const auto& extension : availableExtensions) {
                std::cout << "Extension " << e_count << ": ";
                for (char c : extension.extensionName) std::cout << c;
                std::cout << "\n";
                std::cout << "Description: " << e_descriptions[e_count - 1] << "\n\n";
                e_count++;
                if (e_count > 3) break;
            }

            /*
            // Provided by VK_VERSION_1_0
            typedef struct VkQueueFamilyProperties {
                VkQueueFlags    queueFlags;
                uint32_t        queueCount;
                uint32_t        timestampValidBits;
                VkExtent3D      minImageTransferGranularity;
            } VkQueueFamilyProperties;
            */

                
                //<< deviceProperties2.properties.deviceType << "\n";
            //std::cout << std::endl << std::endl "Device: " << deviceProperties2.properties.deviceName << "\n\n";
            VkPhysicalDeviceMemoryProperties memPropertiesPrint;
            vkGetPhysicalDeviceMemoryProperties(device, &memPropertiesPrint);

            std::cout << "Memory Query" << std::endl;
            std::cout << "Device Name: ";
            for (char c : deviceProperties2.properties.deviceName) {
                std::cout << c;

            }
            std::cout << std::endl;
            std::cout << "Memory Type Count: " << memPropertiesPrint.memoryTypeCount << std::endl;

            for (uint32_t i = 0; i < memPropertiesPrint.memoryTypeCount; i++) {
                std::cout << "MEMORY-TYPE " << i << ":�- MEMORY - FLAGS: ";
                auto propertyFlags = memPropertiesPrint.memoryTypes[i].propertyFlags;
                std::cout << propertyFlags << " => ";
                if (propertyFlags == 0) std::cout << "no flag raised";
                if (propertyFlags & 1) std::cout << "device local ";
                if (propertyFlags & 2) std::cout << "host visible ";
                if (propertyFlags & 4) std::cout << "host coherent ";
                if (propertyFlags & 8) std::cout << "host cached ";
                std::cout << std::endl;
            }
            std::cout << std::endl;
            std::cout << "Heap Count: " << memPropertiesPrint.memoryHeapCount << std::endl;

            for (uint32_t i = 0; i < memPropertiesPrint.memoryHeapCount; i++) {
                std::cout << "HEAP " << i << ":�- HEAP - FLAGS: ";
                auto heapFlags = memPropertiesPrint.memoryHeaps[i].flags;
                std::cout << heapFlags << " => ";
                if (heapFlags == 0) std::cout << "no flag raised";
                if (heapFlags & 1) std::cout << "device local ";
                if (heapFlags & 2) std::cout << "multi instance ";

                std::cout << std::endl;
            }

            
            std::cout << "The funcions vkFlushMappedMemoryRanges() and vkInvalidateMappedMemoryRanges() are not necessary for host coherent. In my case Memory Types 9 and 10 (see above). The functions are needed to flush before writing and to turn invalid beforde reading memory. So essentially for synchronization.\n\n";

            
            
            if (deviceProperties2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU 
                && isDeviceSuitable(device, deviceExtensions, surface)) {

                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            for (const auto& device : devices) {
                if (isDeviceSuitable(device, deviceExtensions, surface)) {
                    physicalDevice = device;
                    break;
                }
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, QueueFamilyIndices& queueFamilies
        , const std::vector<const char*>& validationLayers, const std::vector<const char*>& deviceExtensions
        , VkDevice& device, VkQueue& graphicsQueue, VkQueue& presentQueue) {

        queueFamilies = findQueueFamilies(physicalDevice, surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { queueFamilies.graphicsFamily.value(), queueFamilies.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        volkLoadDevice(device);

        vkGetDeviceQueue(device, queueFamilies.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilies.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkDevice device, SwapChain& swapChain) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain.m_swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain.m_swapChain, &imageCount, nullptr);
        swapChain.m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain.m_swapChain, &imageCount, swapChain.m_swapChainImages.data());

        swapChain.m_swapChainImageFormat = surfaceFormat.format;
        swapChain.m_swapChainExtent = extent;
    }

    void createImageViews(VkDevice device, SwapChain& swapChain) {
        swapChain.m_swapChainImageViews.resize(swapChain.m_swapChainImages.size());

        for (uint32_t i = 0; i < swapChain.m_swapChainImages.size(); i++) {
            swapChain.m_swapChainImageViews[i] = createImageView(device, swapChain.m_swapChainImages[i]
                , swapChain.m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void createRenderPass(VkPhysicalDevice physicalDevice, VkDevice device, SwapChain& swapChain, VkRenderPass& renderPass) {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChain.m_swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat(physicalDevice);
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout) {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        layoutInfo.flags = 0; //wegen fehlermeldung. davor keine zeile

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void createGraphicsPipeline(VkDevice device, VkRenderPass renderPass
        , VkDescriptorSetLayout descriptorSetLayout, Pipeline& graphicsPipeline) {

        auto vertShaderCode = readFile("shaders/vert_blinnphong.spv");
        auto fragShaderCode = readFile("shaders/frag_blinnphong.spv");

        VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE; // VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT 
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipeline.m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = graphicsPipeline.m_pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline.m_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    //just copied the original function and added different shaders; probably better: making shaders an input for one and the same function
    void createSpecialGraphicsPipeline(VkDevice device, VkRenderPass renderPass
        , VkDescriptorSetLayout descriptorSetLayout, Pipeline& graphicsPipeline) {

        //vertex shader stretches the object
        auto vertShaderCode = readFile("shaders/vert.spv"); //statt vert_special
        //frag shader sets red value to zero
        auto fragShaderCode = readFile("shaders/frag_grey.spv");

        VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE; // VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipeline.m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = graphicsPipeline.m_pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline.m_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createFloorGraphicsPipeline(VkDevice device, VkRenderPass renderPass
        , VkDescriptorSetLayout descriptorSetLayout, Pipeline& graphicsPipeline) {

        //vertex shader stretches the object
        auto vertShaderCode = readFile("shaders/vert.spv");
        //frag shader sets red value to zero
        auto fragShaderCode = readFile("shaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE; // VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipeline.m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = graphicsPipeline.m_pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline.m_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
    

    void createFramebuffers(VkDevice device, SwapChain& swapChain, DepthImage& depthImage, VkRenderPass renderPass) {
        swapChain.m_swapChainFramebuffers.resize(swapChain.m_swapChainImageViews.size());

        for (size_t i = 0; i < swapChain.m_swapChainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                swapChain.m_swapChainImageViews[i],
                depthImage.m_depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChain.m_swapChainExtent.width;
            framebufferInfo.height = swapChain.m_swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChain.m_swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool& commandPool) {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }
    }

    void createDepthResources(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator, SwapChain& swapChain, DepthImage& depthImage) {
        VkFormat depthFormat = findDepthFormat(physicalDevice);

        createImage(physicalDevice, device, vmaAllocator, swapChain.m_swapChainExtent.width
            , swapChain.m_swapChainExtent.height, depthFormat
            , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage.m_depthImage, depthImage.m_depthImageAllocation);
        depthImage.m_depthImageView = createImageView(device, depthImage.m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates
        , VkImageTiling tiling, VkFormatFeatureFlags features) {

        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
        return findSupportedFormat(physicalDevice, 
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }


    void MemCopy(VkDevice device, void* source, VmaAllocationInfo& allocInfo, VkDeviceSize size) {
        memcpy(allocInfo.pMappedData, source, size);
    }


  void createTextureImage(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
        , VkQueue graphicsQueue, VkCommandPool commandPool, const std::string& TEXTURE_PATH, Texture& texture) {

        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VmaAllocation stagingBufferAllocation;
        VmaAllocationInfo allocInfo;
        createBuffer(physicalDevice, device, vmaAllocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
            , stagingBuffer, stagingBufferAllocation, &allocInfo);

        MemCopy(device, pixels, allocInfo, imageSize);

        stbi_image_free(pixels);

        createImage(physicalDevice, device, vmaAllocator, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB
            , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.m_textureImage, texture.m_textureImageAllocation); 

        transitionImageLayout(device, graphicsQueue, commandPool, texture.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB
            , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            copyBufferToImage(device, graphicsQueue, commandPool, stagingBuffer, texture.m_textureImage
                , static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(device, graphicsQueue, commandPool, texture.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB
            , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        destroyBuffer(device, vmaAllocator, stagingBuffer, stagingBufferAllocation);

        
    }

    void createTextureImageView(VkDevice device, Texture& texture) {
        texture.m_textureImageView = createImageView(device, texture.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB
                                        , VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void createTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device, Texture &texture) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;


        if (vkCreateSampler(device, &samplerInfo, nullptr, &texture.m_textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }

        return imageView;
    }


    void createImage(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator, uint32_t width, uint32_t height
        , VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties
        , VkImage& image, VmaAllocation& imageAllocation) {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocInfo.priority = 1.0f;
        vmaCreateImage(vmaAllocator, &imageInfo, &allocInfo, &image, &imageAllocation, nullptr);
    }

    void destroyImage(VkDevice device, VmaAllocator vmaAllocator, VkImage image, VmaAllocation& imageAllocation) {
        vmaDestroyImage(vmaAllocator, image, imageAllocation);
    }


    void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool
        , VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
    }

    void copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool
        , VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
    }

    void loadModel( Geometry& geometry, const std::string& MODEL_PATH, float tex_repeat) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0] * tex_repeat,
                    (1.0f - attrib.texcoords[2 * index.texcoord_index + 1]) * tex_repeat
                };

                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
                
                //vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(geometry.m_vertices.size());
                    geometry.m_vertices.push_back(vertex);
                }

                geometry.m_indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
        , VkQueue graphicsQueue, VkCommandPool commandPool, Geometry& geometry) {

        VkDeviceSize bufferSize = sizeof(geometry.m_vertices[0]) * geometry.m_vertices.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        VmaAllocationInfo allocInfo;
        createBuffer(physicalDevice, device, vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 
            , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
            , stagingBuffer, stagingBufferAllocation, &allocInfo);

        MemCopy(device, geometry.m_vertices.data(), allocInfo, bufferSize);

        createBuffer(physicalDevice, device, vmaAllocator, bufferSize
            , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, geometry.m_vertexBuffer
            , geometry.m_vertexBufferAllocation);

        copyBuffer(device, graphicsQueue, commandPool, stagingBuffer, geometry.m_vertexBuffer, bufferSize);

        destroyBuffer(device, vmaAllocator, stagingBuffer, stagingBufferAllocation);
    }

    void createIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
        , VkQueue graphicsQueue, VkCommandPool commandPool, Geometry& geometry) {

        VkDeviceSize bufferSize = sizeof(geometry.m_indices[0]) * geometry.m_indices.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        VmaAllocationInfo allocInfo;
        createBuffer(physicalDevice, device, vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 
            , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
            , stagingBuffer, stagingBufferAllocation, &allocInfo);

        MemCopy(device, geometry.m_indices.data(), allocInfo, bufferSize);

        createBuffer(physicalDevice, device, vmaAllocator, bufferSize
            , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0
            , geometry.m_indexBuffer, geometry.m_indexBufferAllocation);

        copyBuffer(device, graphicsQueue, commandPool, stagingBuffer, geometry.m_indexBuffer, bufferSize);

        destroyBuffer(device, vmaAllocator, stagingBuffer, stagingBufferAllocation);
    }

    void createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& vmaAllocator
            , UniformBuffers &uniformBuffers) {

        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffers.m_uniformBuffersAllocation.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffers.m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VmaAllocationInfo allocInfo;
            createBuffer(physicalDevice, device, vmaAllocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 
                , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
                , uniformBuffers.m_uniformBuffers[i] 
                , uniformBuffers.m_uniformBuffersAllocation[i], &allocInfo);

            uniformBuffers.m_uniformBuffersMapped[i] = allocInfo.pMappedData;
        }
        
        std::cout << "UniformBuffers created: " << uniformBuffers.m_uniformBuffers.size() << std::endl;
        
    }

    void createDescriptorPool(VkDevice device, VkDescriptorPool& descriptorPool) {
        /*std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }*/

        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        VkDescriptorPool imguiPool;
        vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);
    }

    void createDescriptorSets(VkDevice device, Texture& texture
        , VkDescriptorSetLayout descriptorSetLayout, UniformBuffers& uniformBuffers, VkDescriptorPool descriptorPool
        , std::vector<VkDescriptorSet>& descriptorSets) {

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers.m_uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.m_textureImageView;
            imageInfo.sampler = texture.m_textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }


    void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
        , VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties
        , VmaAllocationCreateFlags vmaFlags, VkBuffer& buffer
        , VmaAllocation& allocation, VmaAllocationInfo* allocationInfo = nullptr) {
    
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = vmaFlags;
        vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, allocationInfo);
    }

    void destroyBuffer(VkDevice device, VmaAllocator vmaAllocator, VkBuffer buffer, VmaAllocation& allocation) {

        vmaDestroyBuffer(vmaAllocator, buffer, allocation);
    }
    

    VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void copyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer
        , VkBuffer dstBuffer, VkDeviceSize size) {

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
    }

    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            std::cout << "MEMORY: \n";
            std::cout << memProperties.memoryTypes[i].propertyFlags << std::endl;
        }

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createCommandBuffers(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>& commandBuffers) {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    //adjusted function for input of two different object sets and graphics pipelines (perhaps better a vector of pipelines that map to objects?)
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex
        , SwapChain& swapChain, VkRenderPass renderPass, Pipeline& graphicsPipeline, Pipeline& specialGraphicsPipeline, Pipeline& floorGraphicsPipeline
        , std::vector<Object>& standard_objects, std::vector<Object>& special_objects, std::vector<Object>& floor_objects //Geometry& geometry, std::vector<VkDescriptorSet>& descriptorSets
		, uint32_t currentFrame) {

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChain.m_swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain.m_swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.4f, 0.5f, 6.0f, 1.0f}};  //hier ersetzen?
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.m_pipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float) swapChain.m_swapChainExtent.width;
            viewport.height = (float) swapChain.m_swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = swapChain.m_swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            //first standard objects with standard graphics pipeline

			for( auto& object : standard_objects ) {
	            VkBuffer vertexBuffers[] = {object.m_geometry.m_vertexBuffer};
	            VkDeviceSize offsets[] = {0};
	            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	            //vkCmdBindIndexBuffer(commandBuffer, object.m_geometry.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.m_pipelineLayout
	                , 0, 1, &object.m_descriptorSets[currentFrame], 0, nullptr);

	            //vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.m_geometry.m_indices.size()), 1, 0, 0, 0);
                uint32_t vertexCount = static_cast<uint32_t>(object.m_geometry.m_vertices.size());
                vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
			}

            // second special objects (the sphere) with specialgraphicspipeline
            //probably better: looping through pipeline vector
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, specialGraphicsPipeline.m_pipeline);

            for (auto& object : special_objects) {
                VkBuffer vertexBuffers[] = { object.m_geometry.m_vertexBuffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

                //vkCmdBindIndexBuffer(commandBuffer, object.m_geometry.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, specialGraphicsPipeline.m_pipelineLayout
                    , 0, 1, &object.m_descriptorSets[currentFrame], 0, nullptr);

                //vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.m_geometry.m_indices.size()), 1, 0, 0, 0);
                uint32_t vertexCount = static_cast<uint32_t>(object.m_geometry.m_vertices.size());
                vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
            }

            // third floor with floorgraphicspipeline
            //probably better: looping through pipeline vector
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, floorGraphicsPipeline.m_pipeline);

            for (auto& object : floor_objects) {
                VkBuffer vertexBuffers[] = { object.m_geometry.m_vertexBuffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

                //vkCmdBindIndexBuffer(commandBuffer, object.m_geometry.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, floorGraphicsPipeline.m_pipelineLayout
                    , 0, 1, &object.m_descriptorSets[currentFrame], 0, nullptr);

                //vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.m_geometry.m_indices.size()), 1, 0, 0, 0);
                uint32_t vertexCount = static_cast<uint32_t>(object.m_geometry.m_vertices.size());
                vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
            }

            //----------------------------------------------------------------------------------
            ImGui::Render();

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
            //----------------------------------------------------------------------------------


        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void createSyncObjects(VkDevice device, SyncObjects& syncObjects) {
        syncObjects.m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        syncObjects.m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        syncObjects.m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects.m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects.m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &syncObjects.m_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }
    int space_pressed = 0;

    void checkCameraDistance(Camera& camera, Player& player, glm::vec3 player_direction, float speed) {
        if (glm::length(player.position - camera.position) > 5) {
            camera.position += normalize(glm::vec3(player_direction.x, player_direction.y, 0.0)) * speed;//glm::vec3(0.0f, 1.0f, 0.0f)* speed;
            camera.lookat += normalize(glm::vec3(player_direction.x, player_direction.y, 0.0)) * speed;
        }
    }

    void getKeyboardInput(Camera& camera, Player& player, std::vector<Object>& standard_objects, std::vector<Object>& special_objects, std::vector<Object>& floor_objects, int& space_pressed) { //SDL_Event& event, 

        glm::vec3 camera_direction = normalize(camera.lookat - camera.position);

        glm::vec3 player_direction = normalize(player.lookat - player.position);
        const float camera_speed = 0.05f;
        const float player_speed = 0.3f;
       //onst float look_speed = 0.03f;
        const float tilt_speed = 0.05f;

        glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);

        const Uint8* keystates = SDL_GetKeyboardState(NULL);

        //if (event.type == SDL_KEYDOWN) { // || event.state == SDL_PRESSED) {
           
            //switch (event.key.keysym.sym) {
                
        if (keystates[SDL_SCANCODE_W]) { //case SDLK_w:
            //camera.position += direction * speed; //glm::vec3(0.0f, 1.0f, 0.0f)* speed;
            //camera.lookat += direction * speed; //glm::vec3(0.0f, 1.0f, 0.0f)* speed;

            player.position += player_direction * player_speed; //glm::vec3(0.0f, 1.0f, 0.0f)* speed;
            player.lookat += player_direction * player_speed; //glm::vec3(0.0f, 1.0f, 0.0f)* speed;

            //checkCameraDistance(camera, player, player_direction, camera_speed);

            
            

            
        }//break;
        if (keystates[SDL_SCANCODE_S]) { //case SDLK_s:
            //camera.position -= direction * speed; //glm::vec3(0.0f, 1.0f, 0.0f)* speed;
            //camera.lookat -= direction * speed; //glm::vec3(0.0f, 1.0f, 0.0f)* speed;

            player.position -= player_direction * player_speed; //glm::vec3(0.0f, 1.0f, 0.0f)* speed;
            player.lookat -= player_direction * player_speed; //glm::vec3(0.0f, 1.0f, 0.0f)* speed;
            //break;

            //checkCameraDistance(camera, player, player_direction, camera_speed);
        }
        if (keystates[SDL_SCANCODE_A]) { //case SDLK_a:
            //player.position -= glm::cross(player_direction, up) * speed;
            //player.lookat -= glm::cross(player_direction, up) * speed;

            player.lookat -= glm::cross(player_direction, up) * tilt_speed;
            //break;

            //checkCameraDistance(camera, player, player_direction, camera_speed);
        }
        if (keystates[SDL_SCANCODE_D]) { //case SDLK_d:
            //player.position += glm::cross(player_direction, up) * speed;
            //player.lookat += glm::cross(player_direction, up) * speed;
            //break;
            player.lookat += glm::cross(player_direction, up) * tilt_speed;

            //player.position += glm::cross(direction, camera.up) * speed; camera
            //player.lookat += glm::cross(direction, camera.up) * speed;

            //checkCameraDistance(camera, player, player_direction, camera_speed);
        }

        
        if (keystates[SDL_SCANCODE_UP]) { //case SDLK_UP:
            camera.position += glm::vec3(camera_direction.x, camera_direction.y, 0.0) * camera_speed;//glm::vec3(0.0f, 1.0f, 0.0f)* speed;
            camera.lookat += glm::vec3(camera_direction.x, camera_direction.y, 0.0) * camera_speed;
            //camera.lookat += camera.up * speed;

            //camera.lookat += glm::vec3(0.0f, 0.0f, 1.0f) * tilt_speed;// *2;
            //break;
        }
        if (keystates[SDL_SCANCODE_DOWN]) { //case SDLK_DOWN:
            //camera.position -= camera.up * speed; //glm::vec3(0.0f, 1.0f, 0.0f)* speed;
            //camera.lookat -= camera.up * speed;
            //camera.lookat -= glm::vec3(0.0f, 0.0f, 1.0f) * tilt_speed;// *2;
            //break;
            camera.position -= glm::vec3(camera_direction.x, camera_direction.y, 0.0) * camera_speed;//glm::vec3(0.0f, 1.0f, 0.0f)* speed;
            camera.lookat -= glm::vec3(camera_direction.x, camera_direction.y, 0.0) * camera_speed;
        }
        
        if (keystates[SDL_SCANCODE_LEFT]) { //case SDLK_LEFT:
            camera.lookat -= glm::cross(camera_direction, camera.up) * tilt_speed;
            //player.lookat -= glm::cross(player_direction, up) * tilt_speed;
            //player.lookat = camera.lookat; // -= glm::cross(player_direction, up) * tilt_speed;
            //break;
        }
        if (keystates[SDL_SCANCODE_RIGHT]) { //case SDLK_RIGHT:
            camera.lookat += glm::cross(camera_direction, camera.up) * tilt_speed;
            //player.lookat += glm::cross(player_direction, up) * tilt_speed;
            //player.lookat = camera.lookat; // += glm::cross(player_direction, up) * tilt_speed;
            //break;
        }
        if (space_pressed > 0) space_pressed--;
        //bool space_pressed = false;
        //SDL_Event localEvent;
        //while(SDL_PollEvent(&localEvent)) {
            if (keystates[SDL_SCANCODE_SPACE] && space_pressed == 0 && !standard_objects.empty() && !special_objects.empty()) {
                Sound->play2D("sound/action2.wav", false);
                //keystates[SDL_SCANCODE_SPACE] && !space_pressed) { //case SDLK_RIGHT: localEvent.type == SDL_KEYUP &&  (localEvent.type == SDL_KEYDOWN && localEvent.key.keysym.scancode == SDL_SCANCODE_SPACE && !standard_objects.empty())
                //special_objects.push_back(standard_objects.back());
                //standard_objects.pop_back();

                //for (auto object : special_objects) {
                for (auto it = special_objects.begin(); it != special_objects.end(); it++) {
                    std::cout << "Sphere Coords: " << (*it).m_ubo.model[3][0] << ", " << (*it).m_ubo.model[3][1] << ", " << (*it).m_ubo.model[3][2] << std::endl;
                    if (abs((*it).m_ubo.model[3][0] - standard_objects[1].m_ubo.model[3][0]) < 1 && abs((*it).m_ubo.model[3][1] - standard_objects[1].m_ubo.model[3][1]) < 1) {
                        floor_objects.push_back(*it);
                        it = special_objects.erase(it);
                        spheres_restored++;
                        Sound->play2D("sound/restored.wav", false);
                        if (spheres_restored == 5) {
                            for (auto object : special_objects) {
                                standard_objects.push_back(object);
                            }
                            special_objects.clear();
                            game_won = true;
                        }
                        break;
                    }
                }
                std::cout << "Player-Position in variable: " << player.position[0] << ", " << player.position[1] << ", " << player.position[2] << std::endl;
                std::cout << "Player Position �ber ubo: " << standard_objects[1].m_ubo.model[3][0] << ", " << standard_objects[1].m_ubo.model[3][1] << ", " << standard_objects[1].m_ubo.model[3][2] << std::endl;

                space_pressed = 10;
            }
        //}

        

           
    }

    //static Camera camera;

    void updateUniformBuffer(uint32_t currentImage, SwapChain& swapChain, std::vector<Object>& objects, std::vector<Object>& standard_objects, std::vector<Object>& special_objects) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		startTime = currentTime;
        if (game_time > 0 && game_won == false) {
            game_time -= dt;
            time_interval -= dt;
        }
        //else game_time = 0;
        if (time_interval < 0 && !standard_objects.empty()) {
            special_objects.push_back(standard_objects.back());
            standard_objects.pop_back();
            time_interval = 1;
        }
        if (game_time < 0) {
            game_time = 0;
            game_over = true;
        }

        //objects[4].m_ubo.model[3][0] = camera.position.x + 1.0f;//, camera.position.y + 1.0f, camera.position.z + 1.0f, 1);

        if (game_won == true) {
            camera.lookat += glm::cross(camera.lookat - camera.position, camera.up) * 0.001f;
        }

		for( auto& object : objects ) {

            
            
            
            if (object.m_geometry.m_vertices.size() == m_objects[2].m_geometry.m_vertices.size()) {
                glm::vec3 direction = glm::normalize(player.lookat - player.position);
                //glm::vec3 player_position = camera.position + direction * 2.0f;
                float angle = atan2(direction.y, direction.x);
                glm::mat4 player_rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1));
                glm::mat4 model = glm::translate(glm::mat4(0.1f), player.position);
                model = model * player_rotation;
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                //model = glm::scale(model, glm::vec3(0.2f));
                object.m_ubo.model = model;
                
                //object.m_ubo.model = glm::translate(glm::mat4(1.0f), player_position);
                //object.m_ubo.model = glm::scale(object.m_ubo.model, glm::vec3(0.2f));
            }
            
            //if (object.m_geometry.m_vertices.size() == objects[0].m_geometry.m_vertices.size() || object.m_geometry.m_vertices.size() == objects[1].m_geometry.m_vertices.size()) object.m_ubo.model = glm::rotate(object.m_ubo.model, dt * 1.0f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            object.m_ubo.view = camera.getViewmatrix(); //glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			object.m_ubo.proj = glm::perspective(glm::radians(45.0f), swapChain.m_swapChainExtent.width / (float) swapChain.m_swapChainExtent.height, 0.1f, 100.0f);
	        object.m_ubo.proj[1][1] *= -1;
            object.m_ubo.viewInv = glm::inverse(object.m_ubo.view);
            

	        memcpy(object.m_uniformBuffers.m_uniformBuffersMapped[currentImage], &object.m_ubo, sizeof(object.m_ubo));
		}
        //std::cout << "Camera Pos: " << glm::to_string(camera.position) << "\n";
    }

    //adjusted function for inpuit of two different pipelines and object sets. Again: too convoluted and not easily extendible
    void drawFrame(SDL_Window* window, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice
        , VkDevice device, VmaAllocator vmaAllocator, VkQueue graphicsQueue, VkQueue presentQueue
        , SwapChain& swapChain, DepthImage& depthImage, VkRenderPass renderPass, Pipeline& graphicsPipeline, Pipeline& specialGraphicsPipeline, Pipeline& floorGraphicsPipeline 
		, std::vector<Object>& standard_objects, std::vector<Object>& special_objects, std::vector<Object>& floor_objects, std::vector<VkCommandBuffer>& commandBuffers
        , SyncObjects& syncObjects, uint32_t& currentFrame, bool& framebufferResized) {   

        vkWaitForFences(device, 1, &syncObjects.m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain.m_swapChain, UINT64_MAX
                            , syncObjects.m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR ) {
            recreateSwapChain(window, surface, physicalDevice, device, vmaAllocator, swapChain, depthImage, renderPass);
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        

        updateUniformBuffer(currentFrame, swapChain, standard_objects, standard_objects, special_objects); 
        updateUniformBuffer(currentFrame, swapChain, special_objects, standard_objects, special_objects);
        updateUniformBuffer(currentFrame, swapChain, floor_objects, standard_objects, special_objects);

        vkResetFences(device, 1, &syncObjects.m_inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame],  0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex, swapChain, renderPass, graphicsPipeline, specialGraphicsPipeline, floorGraphicsPipeline, standard_objects, special_objects, floor_objects, currentFrame);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {syncObjects.m_imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {syncObjects.m_renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, syncObjects.m_inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain.m_swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain(window, surface, physicalDevice, device, vmaAllocator, swapChain, depthImage, renderPass);
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) { //MAILBOX_KHR
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR; //FIFO_KHR
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            SDL_GetWindowSize(m_sdlWindow, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width
                , capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height
                , capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }
        
        
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        if (first == true) {
            std::cout << "Image Formats and Color Spaces: \n";
            for (auto f : details.formats) {
                std::cout << f.format << "\n";
            }
            std::cout << "44 means: VK_FORMAT_B8G8R8A8_UNORM. This specifies a four-component, 32-bit unsigned normalized format that has an 8-bit B component in byte 0, an 8-bit G component in byte 1, an 8-bit R component in byte 2, and an 8-bit A component in byte 3.\n";
            std::cout << "50 means: VK_FORMAT_B8G8R8A8_SRGB. This specifies a four-component, 32-bit unsigned normalized format that has an 8-bit B component stored with sRGB nonlinear encoding in byte 0, an 8-bit G component stored with sRGB nonlinear encoding in byte 1, an 8-bit R component stored with sRGB nonlinear encoding in byte 2, and an 8-bit A component in byte 3.\n\n";
            std::cout << "Present Modes: \n";
            for (auto p : details.presentModes) {
                //for (auto mode : p.pPresentModes) {
                    std::cout << p << "  ";
               // }
                
            }
            std::cout << "\n";
            std::cout << "Description: \n";
            std::cout << "1 = Mailbox Mode\n";
            std::cout << "2 = FIFO\n";
            std::cout << "3 = FIFO relaxed\n\n";
    
            
            first = false;
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions, VkSurfaceKHR surface) {
        QueueFamilyIndices indices = findQueueFamilies(device, surface);

        bool extensionsSupported = checkDeviceExtensionSupport(device, deviceExtensions);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate  && supportedFeatures.samplerAnisotropy;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t extensions_count = 0;
        std::vector<const char*> extensions;
        SDL_Vulkan_GetInstanceExtensions(m_sdlWindow, &extensions_count, nullptr);
        extensions.resize(extensions_count);
        SDL_Vulkan_GetInstanceExtensions(m_sdlWindow, &extensions_count, extensions.data());
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }

    bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                std::cout << "Instance Layer Found: " << layerProperties.layerName << "\n";
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;

                    
                    
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
        , VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
        , void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }


    //------------------------------------------------------------------------


    void setupImgui(VkInstance instance, VkPhysicalDevice physicalDevice, QueueFamilyIndices queueFamilies
        , VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorPool descriptorPool
        , VkRenderPass renderPass) {
            
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

        ImGui_ImplVulkan_LoadFunctions( VK_API_VERSION_1_0, &loadVolk );

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForVulkan(m_sdlWindow);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = instance;
        init_info.PhysicalDevice = physicalDevice;
        init_info.Device = device;
        init_info.QueueFamily = queueFamilies.graphicsFamily.value();
        init_info.Queue = graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = descriptorPool;
        init_info.PipelineInfoMain.RenderPass = renderPass;
        init_info.PipelineInfoMain.Subpass = 0;
        init_info.MinImageCount = 3;
        init_info.ImageCount = 3;
        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&init_info);
        // (this gets a bit more complicated, see example app for full reference)
        //ImGui_ImplVulkan_CreateFontsTexture(YOUR_COMMAND_BUFFER);
        // (your code submit a queue)
        //ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
};




int main() {
    HelloTriangleApplication app;

    std::vector<glm::vec4> vertices;
    vertices.push_back(glm::vec4(-1, 1, -1,1));
    vertices.push_back(glm::vec4(-1, 1, 1,1));
    vertices.push_back(glm::vec4(-1, -1, -1,1));
    vertices.push_back(glm::vec4(-1, -1, 1,1));
    vertices.push_back(glm::vec4(1, 1, -1,1));
    vertices.push_back(glm::vec4(1, 1, 1,1));
    vertices.push_back(glm::vec4(1, -1, -1,1));
    vertices.push_back(glm::vec4(1, -1, 1,1));

    glm::vec3 cube_position = glm::vec3(1, 1, -10);

    glm::vec3 camera_position = glm::vec3(1, 1, -1);
    glm::vec3 camera_direction = glm::vec3(0, 0, -1);

    float angle = 45.0f;
    float ratio = 4.0f / 3.0f;
    float near = 1.0f;
    float far = 20.0f;

    glm::mat4 world_matrix = glm::mat4(1.0f);
    world_matrix = glm::translate(world_matrix, cube_position);

    glm::vec4 new_position_1 = world_matrix * vertices[0];

    //std::cout << "new vertex 1: " << new_position_1.x << ", " << new_position_1.y << ", " << new_position_1.z << "\n";

    glm::mat4 camera_world = glm::mat4(1.0f);
    camera_world = glm::translate(camera_world, camera_position);
    glm::mat4 view_matrix = glm::inverse(camera_world);

    glm::mat4 projection = glm::perspective(glm::radians(angle), ratio, near, far);

    std::cout << "World Matrix: " << "\n" << glm::to_string(world_matrix) << "\n\n";
    std::cout << "View Matrix: " << "\n" << glm::to_string(view_matrix) << "\n\n";
    std::cout << "Projection Matrix: " << "\n" << glm::to_string(projection) << "\n\n";

    float width = 400.0f;
    float height = 300.0f;

    int count = 1;

    for (auto vertex : vertices) {
        std::cout << "VERTEX " << count << "\n" << "\n";
        std::cout << "object coordinates: " << vertex.x << ", " << vertex.y << ", " << vertex.z << "\n";
        glm::vec4 world_coord = world_matrix * vertex;
        std::cout << "world coordinates: " << world_coord.x << ", " << world_coord.y << ", " << world_coord.z << "\n";
        glm::vec4 view_coord = view_matrix * world_coord;
        std::cout << "view coordinates: " << view_coord.x << ", " << view_coord.y << ", " << view_coord.z << "\n";
        glm::vec4 proj_coord = projection * view_coord;
        std::cout << "projection coordinates: " << proj_coord.x << ", " << proj_coord.y << ", " << proj_coord.z << "\n";
        glm::vec4 divide = glm::vec4(proj_coord.x / proj_coord.w, proj_coord.y / proj_coord.w, proj_coord.z / proj_coord.w, 1.0f);
        std::cout << "After perspective divide: " << divide.x << ", " << divide.y << ", " << divide.z << "\n";
        float u = (divide.x + 1) * (width / 2);
        float v = - (divide.y - 1) * (height / 2);
        std::cout << "Screen coordinates: " << "x = " << u << ", y = " << v << "\n\n\n";
        count++;


    }

    std::cout << std::endl << "CREATE TEXTURE IMAGE QUERY" << std::endl;
    std::cout << "The Format used is VK_FORMAT_R8G8B8A8_SRGB. This is for standard RGB Images. It is used, since the source is a png file, that is itself formatted in SRGB." << std::endl;
    std::cout << "The Layouts used are VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL and VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL." << std::endl;
    std::cout << "First an undefined layout is initialized, since nothing has been done yet. Then it get's transitioned to transfer destination optimal before copying the buffer to the image. This layoutout is optimized for writing into the image (the destination)." << std::endl;
    std::cout << "Lastly it gets transitioned to Shader read only optimal, before getting transferred to the shader. Now it is optimized for being read by the shader." << std::endl << std::endl;
    

    



    app.run();


    return EXIT_SUCCESS;
}
