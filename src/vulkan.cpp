#include <fstream>

#include <QVulkanFunctions>

#include <glm/glm.hpp>
#include <shaderc/shaderc.hpp>

#include <qt_window.h>

using namespace std;

static QVulkanWindow *vulkan_window;
static VkDevice vulkan_dev;
static QVulkanDeviceFunctions *dev_funct;

VkBuffer vertex_buffer;
VkDeviceMemory vertex_buffer_mem;
VkBuffer index_buffer;
VkDeviceMemory index_buffer_mem;
VkRenderPass render_pass;

struct Vertex {
    glm::vec2 pos;
    glm::vec4 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

std::vector<Vertex> BAR_VERTICES = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 1.0f, 1.0f}},
    {{0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 1.0f, 1.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}}
};
std::vector<uint16_t> BAR_INDICES = {
    0, 1, 2, 2, 3, 0
};

static uint VERTEX_BUFFER_SIZE = sizeof(Vertex) * 4;
static uint INDEX_BUFFER_SIZE = sizeof(uint32_t) * 6;
static bool BUFFERS_SIZE_FINALIZED = false;


QVulkanWindowRenderer *VulkanWindow::createRenderer() {
    VulkanRenderer *renderer = new VulkanRenderer(this);
    renderer->plot_data = this->plot_data;
    renderer->settings = this->settings;

    // Set "const" buffer sizes
    if (!BUFFERS_SIZE_FINALIZED) {
        VERTEX_BUFFER_SIZE *= settings->data.AUDIO_BUFFER_SIZE.MAX / 2;
        INDEX_BUFFER_SIZE *= settings->data.AUDIO_BUFFER_SIZE.MAX / 2;

        BUFFERS_SIZE_FINALIZED = true;
    }

    return renderer;
}

VulkanRenderer::VulkanRenderer(VulkanWindow *window) {
    vulkan_window = window;
}

VkShaderModule createShader(const string name, shaderc_shader_kind shader_kind) {
    string homeDir = getenv("HOME");
    vector<string> shaderFileLocations = {"shaders/",
                                    "../shaders/",
                                    homeDir + "/.config/recidia/shaders/",
                                    "/etc/recidia/shaders/"};
    // Read shader file
    ifstream file;
    for (uint i=0; i < shaderFileLocations.size(); i++) {
        file.open(shaderFileLocations[i] + name);
        if (file.is_open()) {
            break;
        }
    }
    if (!file.is_open()) {
        throw std::runtime_error("Failed to find shader file!");
    }
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

void VulkanRenderer::initResources() {
    VkResult err;

    vulkan_dev = vulkan_window->device();
    dev_funct = vulkan_window->vulkanInstance()->deviceFunctions(vulkan_dev);

    // Vertex Buffer
    createBuffer(VERTEX_BUFFER_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertex_buffer, vertex_buffer_mem);
    //Index Buffer
    createBuffer(INDEX_BUFFER_SIZE, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, index_buffer, index_buffer_mem);

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // Vertex data info
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    vertexInputInfo.pNext = nullptr;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    // Shaders
    VkShaderModule vertShaderModule = createShader("default.vert", shaderc_shader_kind::shaderc_glsl_vertex_shader);
    VkShaderModule fragShaderModule = createShader("default.frag", shaderc_shader_kind::shaderc_glsl_fragment_shader);

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
    rasterizer.cullMode = VK_CULL_MODE_NONE; // NEEDED to flip y axis
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

    err = dev_funct->vkCreatePipelineLayout(vulkan_dev, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", err);

    pipelineInfo.layout = m_pipelineLayout;

    // Render Pass template fo later
//    VkAttachmentDescription colorAttachment{};

//    colorAttachment.format = vulkan_window->colorFormat();
//    colorAttachment.samples = vulkan_window->sampleCountFlagBits();
//    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

//    VkAttachmentReference colorAttachmentRef{};
//    colorAttachmentRef.attachment = 0;
//    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

//    VkSubpassDescription subpass{};
//    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//    subpass.colorAttachmentCount = 1;
//    subpass.pColorAttachments = &colorAttachmentRef;

//    VkSubpassDependency dependency{};
//    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//    dependency.dstSubpass = 0;
//    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//    dependency.srcAccessMask = 0;
//    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

//    VkRenderPassCreateInfo renderPassInfo{};
//    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//    renderPassInfo.attachmentCount = 1;
//    renderPassInfo.pAttachments = &colorAttachment;
//    renderPassInfo.subpassCount = 1;
//    renderPassInfo.pSubpasses = &subpass;
//    renderPassInfo.dependencyCount = 1;
//    renderPassInfo.pDependencies = &dependency;


//    if (dev_funct->vkCreateRenderPass(vulkan_dev, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create render pass!");
//    }
//    pipelineInfo.renderPass = render_pass;

    pipelineInfo.renderPass = vulkan_window->defaultRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    // Finish up Pipline
    err = dev_funct->vkCreateGraphicsPipelines(vulkan_dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (err != VK_SUCCESS)
        printf("Failed to create graphics pipeline: %d", err);

    // Cleanup
    if (vertShaderModule)
        dev_funct->vkDestroyShaderModule(vulkan_dev, vertShaderModule, nullptr);
    if (fragShaderModule)
        dev_funct->vkDestroyShaderModule(vulkan_dev, fragShaderModule, nullptr);
}

void VulkanRenderer::initSwapChainResources() {

}

void VulkanRenderer::releaseSwapChainResources() {

}

void VulkanRenderer::releaseResources() {
    dev_funct->vkDestroyRenderPass(vulkan_dev, render_pass, nullptr);

    if (vertex_buffer) {
        dev_funct->vkDestroyBuffer(vulkan_dev, vertex_buffer, nullptr);
        dev_funct->vkFreeMemory(vulkan_dev, vertex_buffer_mem, nullptr);
    }

    if (m_pipeline) {
        dev_funct->vkDestroyPipeline(vulkan_dev, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout) {
        dev_funct->vkDestroyPipelineLayout(vulkan_dev, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_pipelineCache) {
        dev_funct->vkDestroyPipelineCache(vulkan_dev, m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
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

void VulkanRenderer::startNextFrame() {
    VkCommandBuffer commandBuffer = vulkan_window->currentCommandBuffer();

    plot_data->width = vulkan_window->width();
    plot_data->height = vulkan_window->height();
    uint plotsCount = vulkan_window->width() / (settings->design.plot_width + settings->design.gap_width);

    // Background Color
    float red = settings->design.back_color.red;
    float green = settings->design.back_color.green;
    float blue = settings->design.back_color.blue;
    float alpha = settings->design.back_color.alpha;

    VkClearColorValue clearColor = {{red, green, blue, alpha}};
    VkClearDepthStencilValue clearDS = { 1, 0 };
    VkClearValue clearValues[3]{};

    clearValues[0].color = clearValues[2].color = clearColor;
    clearValues[1].depthStencil = clearDS;

    // Create bars
    red = settings->design.main_color.red;
    green = settings->design.main_color.green;
    blue = settings->design.main_color.blue;
    alpha = settings->design.main_color.alpha;

    // Finalize plots height
    float finalPlots[plotsCount];

    float relHeight = 2.0;
    for (uint i=0; i < plotsCount; i++ ) {

        // Scale plots
        finalPlots[i] = (plot_data->plots[i] / settings->data.height_cap) * relHeight;
        if (finalPlots[i] > settings->design.HEIGHT.MAX * relHeight) {
            finalPlots[i] = settings->design.HEIGHT.MAX * relHeight;
        }
        else if (finalPlots[i] < settings->design.HEIGHT.MIN * relHeight)
            finalPlots[i] = settings->design.HEIGHT.MIN * relHeight;
    }

    // Create Bars
    vector<Vertex> plotsVertexes;
    vector<uint32_t> plotsIndexes;

    float relSize = relHeight / (float) vulkan_window->width(); // Pixel to relative

    float plotSize = relSize * (float) settings->design.plot_width;
    float stepSize = relSize * (float) (settings->design.plot_width + settings->design.gap_width);

    float xPlace = -1.0;
    float xPos, yPos;
    for(uint i=0; i < plotsCount; i++) {

        for(uint j=0; j < BAR_VERTICES.size(); j++) {
            auto vertex = BAR_VERTICES[j];

            xPos = xPlace;
            yPos = -1.0;

            if (j == 1 || j == 2) {
                xPos += plotSize;
            }
            if (j > 1) {
                yPos += finalPlots[i];
            }

            vertex.pos = {xPos, -yPos};
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
    dev_funct->vkMapMemory(vulkan_dev, vertex_buffer_mem, 0, VERTEX_BUFFER_SIZE, 0, &data);
        memcpy(data, plotsVertexes.data(), plotsVertexes.size() * sizeof(plotsVertexes.data()[0]));
    dev_funct->vkUnmapMemory(vulkan_dev, vertex_buffer_mem);
    // Index
    dev_funct->vkMapMemory(vulkan_dev, index_buffer_mem, 0, INDEX_BUFFER_SIZE, 0, &data);
        memcpy(data, plotsIndexes.data(), plotsIndexes.size() * sizeof(plotsIndexes.data()[0]));
    dev_funct->vkUnmapMemory(vulkan_dev, index_buffer_mem);

    // Bind buffers
    VkBuffer vertexBuffers[] = {vertex_buffer};
    VkDeviceSize vbOffset[] = {0};
    dev_funct->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, vbOffset);
    dev_funct->vkCmdBindIndexBuffer(commandBuffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

    dev_funct->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

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
    VkCommandBuffer cmdBuf = vulkan_window->currentCommandBuffer();
    dev_funct->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // DRAW FINALLY
    dev_funct->vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(plotsIndexes.size()), 1, 0, 0, 0);

    dev_funct->vkCmdEndRenderPass(cmdBuf);

    vulkan_window->frameReady();
    vulkan_window->requestUpdate(); // render continuously, throttled by the presentation rate
}
