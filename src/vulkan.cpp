#include <fstream>
#include <unistd.h>

#include <QVulkanFunctions>
#include <QApplication>

#include <glm/glm.hpp>
#include <shaderc/shaderc.hpp>

#include <qt_window.hpp>
#include <recidia.h>

using namespace std;

static VulkanWindow *vulkan_window;
static VkDevice vulkan_dev;
static QVulkanDeviceFunctions *dev_funct;

VkBuffer back_vertex_buffer;
VkDeviceMemory back_vertex_buffer_mem;
VkBuffer main_vertex_buffer;
VkDeviceMemory main_vertex_buffer_mem;
VkBuffer main_index_buffer;
VkDeviceMemory main_index_buffer_mem;

struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;

    static VkVertexInputBindingDescription getBindingDescription(int binding) {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = binding;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions(int binding) {
        array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = binding;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = binding;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

struct PushConstants {
    glm::float32 time;
    glm::float32 power;
};

std::vector<Vertex> BAR_VERTICES = {
    {{-0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
};
std::vector<uint16_t> BAR_INDICES = {
    0, 1, 2, 2, 3, 0
};

static uint VERTEX_BUFFER_SIZE = sizeof(Vertex) * 4;
static uint INDEX_BUFFER_SIZE = sizeof(uint32_t) * 6;
static bool BUFFERS_SIZE_FINALIZED = false;


QVulkanWindowRenderer *VulkanWindow::createRenderer() {
    VulkanRenderer *renderer = new VulkanRenderer(this);

    // Set "const" buffer sizes
    if (!BUFFERS_SIZE_FINALIZED) {
        VERTEX_BUFFER_SIZE *= recidia_settings.data.AUDIO_BUFFER_SIZE.MAX / 2;
        INDEX_BUFFER_SIZE *= recidia_settings.data.AUDIO_BUFFER_SIZE.MAX / 2;

        BUFFERS_SIZE_FINALIZED = true;
    }

    return renderer;
}

VulkanRenderer::VulkanRenderer(VulkanWindow *window) {
    vulkan_window = window;
}

VkShaderModule createShader(const string name, shaderc_shader_kind shader_kind) {
    string homeDir = getenv("HOME");
    string shaderFileLocations[] = {"shaders/",
                                    "../shaders/",
                                    homeDir + "/.config/recidia/shaders/",
                                    "/etc/recidia/shaders/"};
    // Read shader file
    ifstream file;
    for (uint i=0; i < 4; i++) {
        file.open(shaderFileLocations[i] + name);
        if (file.is_open())
            break;
    }
    if (!file.is_open())
        throw std::runtime_error("Failed to find shader file!");

    std::string shaderText( (std::istreambuf_iterator<char>(file) ),
                          (std::istreambuf_iterator<char>()    ) );
    file.close();

    shaderc::Compiler compiler;
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shaderText, shader_kind, "");
    vector<uint32_t> spvCode;
    spvCode.assign(result.cbegin(), result.cend());

    VkShaderModuleCreateInfo shaderInfo;
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = sizeof(uint32_t) * spvCode.size();
    shaderInfo.pCode = (const uint32_t*) spvCode.data();
    shaderInfo.pNext = nullptr; // WILL SEGV WITHOUT
    shaderInfo.flags = 0;

    VkShaderModule shaderModule;
    VkResult err = dev_funct->vkCreateShaderModule(vulkan_dev, &shaderInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        printf("Failed to create shader module: %d\n", err);
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (dev_funct->vkCreateBuffer(vulkan_dev, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        dev_funct->vkGetBufferMemoryRequirements(vulkan_dev, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vulkan_window->hostVisibleMemoryIndex();

        if (dev_funct->vkAllocateMemory(vulkan_dev, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        dev_funct->vkBindBufferMemory(vulkan_dev, buffer, bufferMemory, 0);
    }

static void createPipline(shader_setting shader, VkPipelineLayout &pipelineLayout, VkPipeline &pipeline, int binding) {
    VkResult err;

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // Vertex data info
    auto bindingDescription = Vertex::getBindingDescription(binding);
    auto attributeDescriptions = Vertex::getAttributeDescriptions(binding);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    // Default shaders
    if (!shader.vertex) {
        string shaderName = "default.vert";
        shader.vertex = new char[shaderName.length()+1];
        strcpy(shader.vertex, shaderName.c_str());
    }
    if (!shader.frag) {
        string shaderName = "default.frag";
        shader.frag = new char[shaderName.length()+1];
        strcpy(shader.frag, shaderName.c_str());
    }
    // Shaders
    VkShaderModule vertShaderModule = createShader(shader.vertex, shaderc_shader_kind::shaderc_glsl_vertex_shader);
    VkShaderModule fragShaderModule = createShader(shader.frag, shaderc_shader_kind::shaderc_glsl_fragment_shader);

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

    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    pipelineInfo.pInputAssemblyState = &inputAssembly;

    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    pipelineInfo.pViewportState = &viewportState;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // NEEDED to flip y axis
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    pipelineInfo.pRasterizationState = &rasterizer;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = vulkan_window->sampleCountFlagBits();
    pipelineInfo.pMultisampleState = &multisampling;

    // Depth Stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineInfo.pDepthStencilState = &depthStencil;

    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAtt{};
    colorBlendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAtt.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAtt.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAtt.colorWriteMask = 0xF; // All Colors

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAtt;

    pipelineInfo.pColorBlendState = &colorBlending;

    // Dynamnic States (changes without recreating pipeline)
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
    dynamicState.pDynamicStates = dynamicStates;
    pipelineInfo.pDynamicState = &dynamicState;

    // Pipeline layout (Allows for uniform values or GPU global variables)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    
	// Setup push constants to pass data to shaders
	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(PushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	pipelineLayoutInfo.pushConstantRangeCount = 1;


    err = dev_funct->vkCreatePipelineLayout(vulkan_dev, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    if (err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", err);

    pipelineInfo.layout = pipelineLayout;

    pipelineInfo.renderPass = vulkan_window->defaultRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    // Finish up Pipline
    err = dev_funct->vkCreateGraphicsPipelines(vulkan_dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
    if (err != VK_SUCCESS)
        printf("Failed to create graphics pipeline: %d", err);

    // Cleanup
    if (vertShaderModule)
        dev_funct->vkDestroyShaderModule(vulkan_dev, vertShaderModule, nullptr);
    if (fragShaderModule)
        dev_funct->vkDestroyShaderModule(vulkan_dev, fragShaderModule, nullptr);
}

void VulkanRenderer::initResources() {
    vulkan_dev = vulkan_window->device();
    dev_funct = vulkan_window->vulkanInstance()->deviceFunctions(vulkan_dev);

    createBuffer(VERTEX_BUFFER_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, back_vertex_buffer, back_vertex_buffer_mem);
    createBuffer(VERTEX_BUFFER_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, main_vertex_buffer, main_vertex_buffer_mem);
    createBuffer(INDEX_BUFFER_SIZE, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, main_index_buffer, main_index_buffer_mem);

    createPipline(recidia_settings.misc.back_shader, back_pipelineLayout, back_pipeline, 0);
    createPipline(recidia_settings.misc.main_shader, main_pipelineLayout, main_pipeline, 1);
}

void VulkanRenderer::initSwapChainResources() {

}

void VulkanRenderer::releaseSwapChainResources() {

}

void VulkanRenderer::releaseResources() {
    if (main_vertex_buffer) {
        dev_funct->vkDestroyBuffer(vulkan_dev, main_vertex_buffer, nullptr);
        dev_funct->vkFreeMemory(vulkan_dev, main_vertex_buffer_mem, nullptr);
    }
    if (back_vertex_buffer) {
        dev_funct->vkDestroyBuffer(vulkan_dev, back_vertex_buffer, nullptr);
        dev_funct->vkFreeMemory(vulkan_dev, back_vertex_buffer_mem, nullptr);
    }
    if (main_index_buffer) {
        dev_funct->vkDestroyBuffer(vulkan_dev, main_index_buffer, nullptr);
        dev_funct->vkFreeMemory(vulkan_dev, main_index_buffer_mem, nullptr);
    }

    if (main_pipeline) {
        dev_funct->vkDestroyPipeline(vulkan_dev, main_pipeline, nullptr);
        main_pipeline = VK_NULL_HANDLE;
    }

    if (main_pipelineLayout) {
        dev_funct->vkDestroyPipelineLayout(vulkan_dev, main_pipelineLayout, nullptr);
        main_pipelineLayout = VK_NULL_HANDLE;
    }

    if (back_pipeline) {
        dev_funct->vkDestroyPipeline(vulkan_dev, back_pipeline, nullptr);
        back_pipeline = VK_NULL_HANDLE;
    }

    if (back_pipelineLayout) {
        dev_funct->vkDestroyPipelineLayout(vulkan_dev, back_pipelineLayout, nullptr);
        back_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_descSetLayout) {
        dev_funct->vkDestroyDescriptorSetLayout(vulkan_dev, m_descSetLayout, nullptr);
        m_descSetLayout = VK_NULL_HANDLE;
    }

    if (m_descPool) {
        dev_funct->vkDestroyDescriptorPool(vulkan_dev, m_descPool, nullptr);
        m_descPool = VK_NULL_HANDLE;
    }

    if (m_buf) {
        dev_funct->vkDestroyBuffer(vulkan_dev, m_buf, nullptr);
        m_buf = VK_NULL_HANDLE;
    }

    if (m_bufMem) {
        dev_funct->vkFreeMemory(vulkan_dev, m_bufMem, nullptr);
        m_bufMem = VK_NULL_HANDLE;
    }
}

float get_linear_color(uint srgb) {

    float srgbF = (float) srgb / 255;

    if (srgbF <= 0.04045)
        return srgbF/12.92;
    else
        return pow((srgbF+0.055) / 1.055, 2.4);
}

void create_plots() {
    // Create bars
    float alpha = (float) recidia_settings.design.main_color.alpha / 255;
    float red = get_linear_color(recidia_settings.design.main_color.red) * alpha;
    float green = get_linear_color(recidia_settings.design.main_color.green) * alpha;
    float blue = get_linear_color(recidia_settings.design.main_color.blue) * alpha;

    // Finalize plots height
    float finalPlots[recidia_data.plots_count];

    float relHeight = 2.0;
    for (uint i=0; i < recidia_data.plots_count; i++ ) {

        // Scale plots
        finalPlots[i] = (recidia_data.plots[i] / recidia_settings.data.height_cap) * relHeight;
        if (finalPlots[i] > recidia_settings.design.draw_height * relHeight) {
            finalPlots[i] = recidia_settings.design.draw_height * relHeight;
        }
        else if (finalPlots[i] < recidia_settings.design.min_plot_height * relHeight)
            finalPlots[i] = recidia_settings.design.min_plot_height * relHeight;
    }

    vector<Vertex> plotsVertexes;
    vector<uint32_t> plotsIndexes;

    float relSize = relHeight / (float) vulkan_window->width(); // Pixel to relative

    float plotSize = relSize * (float) recidia_settings.design.plot_width;
    float stepSize = relSize * (float) (recidia_settings.design.plot_width + recidia_settings.design.gap_width);

    float xPlace = recidia_settings.design.draw_x;
    float xPos, yPos;
    for(uint i=0; i < recidia_data.plots_count; i++) {

        for(uint j=0; j < BAR_VERTICES.size(); j++) {
            auto vertex = BAR_VERTICES[j];

            xPos = xPlace;
            yPos = recidia_settings.design.draw_y;

            if (j == 1 || j == 2) {
                xPos += plotSize;
            }
            if (j > 1) {
                if (j == 2 && i < recidia_data.plots_count-1 && recidia_settings.design.draw_mode == 1)
                    yPos += finalPlots[i+1];
                else
                    yPos += finalPlots[i];
            }

            vertex.pos = {xPos, -yPos, vertex.pos[2]};
            vertex.color = {red, green, blue, alpha};
            plotsVertexes.push_back(vertex);
        }
        xPlace += stepSize;

        for(uint j=0; j < BAR_INDICES.size(); j++) {
            auto index = BAR_INDICES[j];
            index += i*4;

            plotsIndexes.push_back(index);
        }
    }

    // Copy vertices data to mem
    void* data;
    dev_funct->vkMapMemory(vulkan_dev, main_vertex_buffer_mem, 0, VERTEX_BUFFER_SIZE, 0, &data);
        memcpy(data, plotsVertexes.data(), plotsVertexes.size() * sizeof(plotsVertexes.data()[0]));
    dev_funct->vkUnmapMemory(vulkan_dev, main_vertex_buffer_mem);
    // Index 
    dev_funct->vkMapMemory(vulkan_dev, main_index_buffer_mem, 0, INDEX_BUFFER_SIZE, 0, &data);
        memcpy(data, plotsIndexes.data(), plotsIndexes.size() * sizeof(plotsIndexes.data()[0]));
    dev_funct->vkUnmapMemory(vulkan_dev, main_index_buffer_mem);
}

static PushConstants get_push_constants(shader_setting shader) {
    PushConstants constants;
    
    constants.time = (float) (utime_now() % (1000000 * shader.loop_time)) / 1000000;
    
    constants.power = 0.0;
    // Rounded up plots count
    uint plotsPowerCount = 0.5 + (recidia_data.plots_count * (shader.power_mod_range[1] - shader.power_mod_range[0]));
    uint plotPowerStart = (recidia_data.plots_count - 1) * shader.power_mod_range[0];
    for(uint i=plotPowerStart; i < (plotsPowerCount + plotPowerStart) && i < recidia_data.plots_count; i++) {

        float powerPush = recidia_data.plots[i] / recidia_settings.data.height_cap;
        if (powerPush < 1.0) {
            constants.power += powerPush / plotsPowerCount;
        }
        else {
            constants.power += 1.0 / plotsPowerCount;
        }
    }
    constants.power *= shader.power;

    return constants;
}

static void draw_background(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, VkPipeline &pipeline) {

    // Background Color
    float alpha = (float) recidia_settings.design.back_color.alpha / 255;
    float red = get_linear_color(recidia_settings.design.back_color.red) * alpha;
    float green = get_linear_color(recidia_settings.design.back_color.green) * alpha;
    float blue = get_linear_color(recidia_settings.design.back_color.blue) * alpha;

    const int verticesCount = 6;
    Vertex backVerticies[verticesCount] = {
        {{-1.0f, -1.0f, 0.0f}, {red, green, blue, alpha}},
        {{-1.0f, 1.0f, 0.0f}, {red, green, blue, alpha}},
        {{1.0f, -1.0f, 0.0f}, {red, green, blue, alpha}},

        {{-1.0f, 1.0f, 0.0f}, {red, green, blue, alpha}},
        {{1.0f, 1.0f, 0.0f}, {red, green, blue, alpha}},
        {{1.0f, -1.0f, 0.0f}, {red, green, blue, alpha}},
    };

    void* data;
    dev_funct->vkMapMemory(vulkan_dev, back_vertex_buffer_mem, 0, VERTEX_BUFFER_SIZE, 0, &data);
        memcpy(data, backVerticies, sizeof(backVerticies[0]) * verticesCount);
    dev_funct->vkUnmapMemory(vulkan_dev, back_vertex_buffer_mem);

    dev_funct->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    PushConstants constants = get_push_constants(recidia_settings.misc.back_shader);
    dev_funct->vkCmdPushConstants(commandBuffer, pipelineLayout, 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);

    dev_funct->vkCmdDraw(commandBuffer, verticesCount, 1, 0, 0);
}

static void draw_plots(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, VkPipeline &pipeline) {
    create_plots();

    dev_funct->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    PushConstants constants = get_push_constants(recidia_settings.misc.main_shader);
    dev_funct->vkCmdPushConstants(commandBuffer, pipelineLayout, 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);

    uint32_t indices_count = recidia_data.plots_count * BAR_INDICES.size();
    dev_funct->vkCmdDrawIndexed(commandBuffer, indices_count, 1, 0, 0, 0);
}

void VulkanRenderer::startNextFrame() {
    VkCommandBuffer commandBuffer = vulkan_window->currentCommandBuffer();

    if (vulkan_window->shader_setting_change) {
        this->recreatePipline();
        vulkan_window->shader_setting_change = 0;
    }

    recidia_data.width = vulkan_window->width() * recidia_settings.design.draw_width;
    recidia_data.height = vulkan_window->height() * recidia_settings.design.draw_height;
    recidia_data.plots_count = (recidia_data.width / (recidia_settings.design.plot_width + recidia_settings.design.gap_width)) + 1;

    VkClearColorValue clearColor = {{0, 0, 0, 0}};
    VkClearDepthStencilValue clearDS = { 1, 0 };
    VkClearValue clearValues[3]{};

    clearValues[0].color = clearValues[2].color = clearColor;
    clearValues[1].depthStencil = clearDS;

    // Graphics view
    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = vulkan_window->width();
    viewport.height = vulkan_window->height();
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    dev_funct->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    dev_funct->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Render Pass
    VkRenderPassBeginInfo rpBeginInfo{};
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = vulkan_window->defaultRenderPass();
    rpBeginInfo.framebuffer = vulkan_window->currentFramebuffer();
    rpBeginInfo.renderArea.extent.width = vulkan_window->width();
    rpBeginInfo.renderArea.extent.height = vulkan_window->height();
    rpBeginInfo.clearValueCount = vulkan_window->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
    rpBeginInfo.pClearValues = clearValues;

    // Bind buffers
    VkDeviceSize vbOffset[] = {0};
    dev_funct->vkCmdBindVertexBuffers(commandBuffer, 0, 1, &back_vertex_buffer, vbOffset);
    dev_funct->vkCmdBindVertexBuffers(commandBuffer, 1, 1, &main_vertex_buffer, vbOffset);
    dev_funct->vkCmdBindIndexBuffer(commandBuffer, main_index_buffer, 0, VK_INDEX_TYPE_UINT32);

    // DRAW FINALLY
    dev_funct->vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    draw_background(commandBuffer, back_pipelineLayout, back_pipeline);
    draw_plots(commandBuffer, main_pipelineLayout, main_pipeline);
    dev_funct->vkCmdEndRenderPass(commandBuffer);

    vulkan_window->frameReady();
    recidia_data.latency = (float) (utime_now() - recidia_data.start_time) / 1000;

    // Sleep for fps cap
    double frameTime = 0;
    double sleepTime = (1000 / (double) recidia_settings.design.fps_cap) * 1000;
    while (sleepTime > frameTime) {
        QApplication::processEvents(QEventLoop::AllEvents, 1);
        usleep(100);

        frameTime = utime_now() - last_frame_time;
    }
    recidia_data.frame_time = utime_now() - last_frame_time;
    last_frame_time = utime_now();

    vulkan_window->requestUpdate(); // render continuously, throttled by the presentation rate
}

void VulkanRenderer::recreatePipline() {
    dev_funct->vkDeviceWaitIdle(vulkan_dev);
    this->releaseSwapChainResources();

    if (vulkan_window->shader_setting_change == 1) {
        dev_funct->vkDestroyPipeline(vulkan_dev, main_pipeline, nullptr);
        main_pipeline = VK_NULL_HANDLE;
    
        dev_funct->vkDestroyPipelineLayout(vulkan_dev, main_pipelineLayout, nullptr);
        main_pipelineLayout = VK_NULL_HANDLE;

        createPipline(recidia_settings.misc.main_shader, main_pipelineLayout, main_pipeline, 1);
    }
    else if (vulkan_window->shader_setting_change == 2) {
        dev_funct->vkDestroyPipeline(vulkan_dev, back_pipeline, nullptr);
        back_pipeline = VK_NULL_HANDLE;

        dev_funct->vkDestroyPipelineLayout(vulkan_dev, back_pipelineLayout, nullptr);
        back_pipelineLayout = VK_NULL_HANDLE;
    
        createPipline(recidia_settings.misc.back_shader, back_pipelineLayout, back_pipeline, 0);
    }
    this->initSwapChainResources();
}