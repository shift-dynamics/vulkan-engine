#include "vulkan-engine/TriangleRenderer.h"

#include <QFile>
#include <QVulkanFunctions>

// Note that the vertex data and the projection matrix assume OpenGL. With
// Vulkan Y is negated in clip space and the near/far plane is at 0/1 instead
// of -1/1. These will be corrected for by an extra transformation when
// calculating the modelview-projection matrix.
static float vertex_data[] = {0.0f, 0.5f, 1.0f, 0.0f,  0.0f, -0.5f, -0.5f, 0.0f,
                              1.0f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f,  1.0f};

static const int UNIFORM_DATA_SIZE = 16 * sizeof(float);

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign) {
  return (v + byteAlign - 1) & ~(byteAlign - 1);
}

vulkan_engine::TriangleRenderer::TriangleRenderer(QVulkanWindow* w, bool msaa)
  : window_(w) {
  if(msaa) {
    const QVector<int> counts = w->supportedSampleCounts();

    // qDebug() << "Supported sample counts:" << counts;

    for(int s = 16; s >= 4; s /= 2) {
      if(counts.contains(s)) {
        // qDebug("Requesting sample count %d", s);
        window_->setSampleCount(s);
        break;
      }
    }
  }
}

VkShaderModule vulkan_engine::TriangleRenderer::createShader(const QString& name) {
  QFile file(name);
  if(!file.open(QIODevice::ReadOnly)) {
    qWarning("Failed to read shader %s", qPrintable(name));
    return VK_NULL_HANDLE;
  }
  QByteArray blob = file.readAll();
  file.close();

  VkShaderModuleCreateInfo shader_info;
  memset(&shader_info, 0, sizeof(shader_info));
  shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shader_info.codeSize = blob.size();
  shader_info.pCode = reinterpret_cast<const uint32_t*>(blob.constData());
  VkShaderModule shader_module;
  VkResult err = funcs_->vkCreateShaderModule(window_->device(), &shader_info,
                                              nullptr, &shader_module);
  if(err != VK_SUCCESS) {
    qWarning("Failed to create shader module: %d", err);
    return VK_NULL_HANDLE;
  }

  return shader_module;
}

void vulkan_engine::TriangleRenderer::initResources() {
  // qDebug("initResources");

  VkDevice device = window_->device();
  funcs_ = window_->vulkanInstance()->deviceFunctions(device);

  // Prepare the vertex and uniform data. The vertex data will never
  // change so one buffer is sufficient regardless of the value of
  // QVulkanWindow::CONCURRENT_FRAME_COUNT. Uniform data is changing per
  // frame however so active frames have to have a dedicated copy.

  // Use just one memory allocation and one buffer. We will then specify the
  // appropriate offsets for uniform buffers in the VkDescriptorBufferInfo.
  // Have to watch out for
  // VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment, though.

  // The uniform buffer is not strictly required in this example, we could
  // have used push constants as well since our single matrix (64 bytes) fits
  // into the spec mandated minimum limit of 128 bytes. However, once that
  // limit is not sufficient, the per-frame buffers, as shown below, will
  // become necessary.

  const int concurrent_frame_count = window_->concurrentFrameCount();
  const VkPhysicalDeviceLimits* device_limits =
    &window_->physicalDeviceProperties()->limits;
  const VkDeviceSize uniform_align =
    device_limits->minUniformBufferOffsetAlignment;

  // qDebug("uniform buffer offset alignment is %u", (uint)uniform_align);
  VkBufferCreateInfo buffer_info;
  memset(&buffer_info, 0, sizeof(buffer_info));
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

  // Our internal layout is vertex, uniform, uniform, ... with each uniform
  // buffer start offset aligned to unform_align.
  const VkDeviceSize vertex_alloc_size =
    aligned(sizeof(vertex_data), uniform_align);
  const VkDeviceSize uniform_alloc_size =
    aligned(UNIFORM_DATA_SIZE, uniform_align);
  buffer_info.size =
    vertex_alloc_size + concurrent_frame_count * uniform_alloc_size;
  buffer_info.usage =
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  VkResult err =
    funcs_->vkCreateBuffer(device, &buffer_info, nullptr, &buffer_);
  if(err != VK_SUCCESS) {
    qFatal("Failed to create buffer: %d", err);
  }

  VkMemoryRequirements memory_requirements;
  funcs_->vkGetBufferMemoryRequirements(device, buffer_, &memory_requirements);

  VkMemoryAllocateInfo memory_alloc_info = {
    VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memory_requirements.size,
    window_->hostVisibleMemoryIndex()};

  err = funcs_->vkAllocateMemory(device, &memory_alloc_info, nullptr,
                                 &buffer_memory_);
  if(err != VK_SUCCESS) {
    qFatal("Failed to allocate memory: %d", err);
  }

  err = funcs_->vkBindBufferMemory(device, buffer_, buffer_memory_, 0);
  if(err != VK_SUCCESS) {
    qFatal("Failed to bind buffer memory: %d", err);
  }

  quint8* p;
  err = funcs_->vkMapMemory(device, buffer_memory_, 0, memory_requirements.size,
                            0, reinterpret_cast<void**>(&p));
  if(err != VK_SUCCESS) {
    qFatal("Failed to map memory: %d", err);
  }
  memcpy(p, vertex_data, sizeof(vertex_data));
  QMatrix4x4 ident;
  memset(uniform_buffer_info_, 0, sizeof(uniform_buffer_info_));
  for(int i = 0; i < concurrent_frame_count; ++i) {
    const VkDeviceSize offset = vertex_alloc_size + i * uniform_alloc_size;
    memcpy(p + offset, ident.constData(), 16 * sizeof(float));
    uniform_buffer_info_[i].buffer = buffer_;
    uniform_buffer_info_[i].offset = offset;
    uniform_buffer_info_[i].range = uniform_alloc_size;
  }
  funcs_->vkUnmapMemory(device, buffer_memory_);

  VkVertexInputBindingDescription vertex_binding_description = {
    .binding = 0, // binding
    .stride = 5 * sizeof(float),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
  VkVertexInputAttributeDescription vertex_attribute_description[] = {
    {               // position
     .location = 0, // shader binding location
     .binding = 0,  // binding number for the attribute
     .format = VK_FORMAT_R32G32_SFLOAT,
     .offset = 0},
    {// color
     .location = 1,
     .binding = 0,
     .format = VK_FORMAT_R32G32B32_SFLOAT,
     .offset = 2 * sizeof(float)}};

  VkPipelineVertexInputStateCreateInfo vertex_input_info;
  vertex_input_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.pNext = nullptr;
  vertex_input_info.flags = 0;
  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.pVertexBindingDescriptions = &vertex_binding_description;
  vertex_input_info.vertexAttributeDescriptionCount = 2;
  vertex_input_info.pVertexAttributeDescriptions = vertex_attribute_description;

  // Set up descriptor set and its layout.
  VkDescriptorPoolSize descPoolSizes = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                        uint32_t(concurrent_frame_count)};
  VkDescriptorPoolCreateInfo descriptor_pool_info;
  memset(&descriptor_pool_info, 0, sizeof(descriptor_pool_info));
  descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_info.maxSets = concurrent_frame_count;
  descriptor_pool_info.poolSizeCount = 1;
  descriptor_pool_info.pPoolSizes = &descPoolSizes;
  err = funcs_->vkCreateDescriptorPool(device, &descriptor_pool_info, nullptr,
                                       &descriptor_pool_);
  if(err != VK_SUCCESS) {
    qFatal("Failed to create descriptor pool: %d", err);
  }

  VkDescriptorSetLayoutBinding layoutBinding = {
    0, // binding
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
  VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 1,
    &layoutBinding};
  err = funcs_->vkCreateDescriptorSetLayout(device, &descriptor_layout_info,
                                            nullptr, &descriptor_set_layout_);
  if(err != VK_SUCCESS)
    qFatal("Failed to create descriptor set layout: %d", err);

  for(int i = 0; i < concurrent_frame_count; ++i) {
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, descriptor_pool_,
      1, &descriptor_set_layout_};
    err = funcs_->vkAllocateDescriptorSets(device, &descriptor_set_alloc_info,
                                           &descriptor_set_[i]);
    if(err != VK_SUCCESS) {
      qFatal("Failed to allocate descriptor set: %d", err);
    }

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = descriptor_set_[i];
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.pBufferInfo = &uniform_buffer_info_[i];
    funcs_->vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
  }

  // Pipeline cache
  VkPipelineCacheCreateInfo pipeline_cache_info;
  memset(&pipeline_cache_info, 0, sizeof(pipeline_cache_info));
  pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  err = funcs_->vkCreatePipelineCache(device, &pipeline_cache_info, nullptr,
                                      &pipeline_cache_);
  if(err != VK_SUCCESS) {
    qFatal("Failed to create pipeline cache: %d", err);
  }

  // Pipeline layout
  VkPipelineLayoutCreateInfo pipeline_layout_info;
  memset(&pipeline_layout_info, 0, sizeof(pipeline_layout_info));
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;
  err = funcs_->vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr,
                                       &pipeline_layout_);
  if(err != VK_SUCCESS) {
    qFatal("Failed to create pipeline layout: %d", err);
  }

  // Shaders
  VkShaderModule vertShaderModule =
    createShader(QStringLiteral(":/shaders/color.vert.spv"));
  VkShaderModule fragShaderModule =
    createShader(QStringLiteral(":/shaders/color.frag.spv"));

  // Graphics pipeline
  VkGraphicsPipelineCreateInfo pipeline_info;
  memset(&pipeline_info, 0, sizeof(pipeline_info));
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  VkPipelineShaderStageCreateInfo shader_stages[2] = {
    {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
     VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main", nullptr},
    {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
     VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main", nullptr}};
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;

  pipeline_info.pVertexInputState = &vertex_input_info;

  VkPipelineInputAssemblyStateCreateInfo ia;
  memset(&ia, 0, sizeof(ia));
  ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  pipeline_info.pInputAssemblyState = &ia;

  // The viewport and scissor will be set dynamically via
  // vkCmdSetViewport/Scissor. This way the pipeline does not need to be touched
  // when resizing the window.
  VkPipelineViewportStateCreateInfo vp;
  memset(&vp, 0, sizeof(vp));
  vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vp.viewportCount = 1;
  vp.scissorCount = 1;
  pipeline_info.pViewportState = &vp;

  VkPipelineRasterizationStateCreateInfo rs;
  memset(&rs, 0, sizeof(rs));
  rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rs.polygonMode = VK_POLYGON_MODE_LINE; // FILL
  rs.cullMode = VK_CULL_MODE_NONE;       // we want the back face as well
  rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rs.lineWidth = 5.0f;
  pipeline_info.pRasterizationState = &rs;

  VkPipelineMultisampleStateCreateInfo ms;
  memset(&ms, 0, sizeof(ms));
  ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

  // Enable multisampling.
  ms.rasterizationSamples = window_->sampleCountFlagBits();
  pipeline_info.pMultisampleState = &ms;

  VkPipelineDepthStencilStateCreateInfo ds;
  memset(&ds, 0, sizeof(ds));
  ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  ds.depthTestEnable = VK_TRUE;
  ds.depthWriteEnable = VK_TRUE;
  ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  pipeline_info.pDepthStencilState = &ds;

  VkPipelineColorBlendStateCreateInfo cb;
  memset(&cb, 0, sizeof(cb));
  cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

  // no blend, write out all of rgba
  VkPipelineColorBlendAttachmentState att;
  memset(&att, 0, sizeof(att));
  att.colorWriteMask = 0xF;
  cb.attachmentCount = 1;
  cb.pAttachments = &att;
  pipeline_info.pColorBlendState = &cb;

  VkDynamicState dynamic_state[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dyn;
  memset(&dyn, 0, sizeof(dyn));
  dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dyn.dynamicStateCount = sizeof(dynamic_state) / sizeof(VkDynamicState);
  dyn.pDynamicStates = dynamic_state;
  pipeline_info.pDynamicState = &dyn;

  pipeline_info.layout = pipeline_layout_;
  pipeline_info.renderPass = window_->defaultRenderPass();

  err = funcs_->vkCreateGraphicsPipelines(device, pipeline_cache_, 1,
                                          &pipeline_info, nullptr, &pipeline_);
  if(err != VK_SUCCESS)
    qFatal("Failed to create graphics pipeline: %d", err);

  if(vertShaderModule) {
    funcs_->vkDestroyShaderModule(device, vertShaderModule, nullptr);
  }
  if(fragShaderModule) {
    funcs_->vkDestroyShaderModule(device, fragShaderModule, nullptr);
  }
}

void vulkan_engine::TriangleRenderer::initSwapChainResources() {
  // Projection matrix
  projection_ = window_->clipCorrectionMatrix(); // adjust for Vulkan-OpenGL
                                                 // clip space differences
  const QSize sz = window_->swapChainImageSize();
  projection_.perspective(45.0f, sz.width() / (float)sz.height(), 0.01f,
                          100.0f);
  projection_.translate(0, 0, -4);
}

void vulkan_engine::TriangleRenderer::releaseSwapChainResources() {
  // qDebug("releaseSwapChainResources");
}

void vulkan_engine::TriangleRenderer::releaseResources() {
  // qDebug("releaseResources");

  VkDevice device = window_->device();

  if(pipeline_) {
    funcs_->vkDestroyPipeline(device, pipeline_, nullptr);
    pipeline_ = VK_NULL_HANDLE;
  }

  if(pipeline_layout_) {
    funcs_->vkDestroyPipelineLayout(device, pipeline_layout_, nullptr);
    pipeline_layout_ = VK_NULL_HANDLE;
  }

  if(pipeline_cache_) {
    funcs_->vkDestroyPipelineCache(device, pipeline_cache_, nullptr);
    pipeline_cache_ = VK_NULL_HANDLE;
  }

  if(descriptor_set_layout_) {
    funcs_->vkDestroyDescriptorSetLayout(device, descriptor_set_layout_,
                                         nullptr);
    descriptor_set_layout_ = VK_NULL_HANDLE;
  }

  if(descriptor_pool_) {
    funcs_->vkDestroyDescriptorPool(device, descriptor_pool_, nullptr);
    descriptor_pool_ = VK_NULL_HANDLE;
  }

  if(buffer_) {
    funcs_->vkDestroyBuffer(device, buffer_, nullptr);
    buffer_ = VK_NULL_HANDLE;
  }

  if(buffer_memory_) {
    funcs_->vkFreeMemory(device, buffer_memory_, nullptr);
    buffer_memory_ = VK_NULL_HANDLE;
  }
}

void vulkan_engine::TriangleRenderer::startNextFrame() {

  VkDevice device = window_->device();
  VkCommandBuffer cb = window_->currentCommandBuffer();
  const QSize sz = window_->swapChainImageSize();

  VkClearColorValue clear_color = {.uint32 = {0, 0, 0, 1}};
  VkClearDepthStencilValue clear_ds = {.depth = 1, .stencil = 0};

  VkClearValue clear_values[] = {{
                                   .color = clear_color,
                                 },
                                 {
                                   .depthStencil = clear_ds,
                                 },
                                 {
                                   .color = clear_color,
                                 }};

  VkRenderPassBeginInfo render_pass_begin_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .pNext = 0,
    .renderPass = window_->defaultRenderPass(),
    .framebuffer = window_->currentFramebuffer(),
    .renderArea =
      {
        .offset = {.x = 0, .y = 0},
        .extent =
          {
            .width = static_cast<unsigned int>(sz.width()),
            .height = static_cast<unsigned int>(sz.height()),
          },
      },
    .clearValueCount = static_cast<unsigned int>(
      window_->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2),
    .pClearValues = clear_values,
  };
  VkCommandBuffer command_buffer = window_->currentCommandBuffer();
  funcs_->vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info,
                               VK_SUBPASS_CONTENTS_INLINE);

  quint8* p;
  VkResult err =
    funcs_->vkMapMemory(device, buffer_memory_,
                        uniform_buffer_info_[window_->currentFrame()].offset,
                        UNIFORM_DATA_SIZE, 0, reinterpret_cast<void**>(&p));
  if(err != VK_SUCCESS) {
    qFatal("Failed to map memory: %d", err);
  }
  QMatrix4x4 m = projection_;
  m.rotate(rotation_, 0, 1, 0);
  memcpy(p, m.constData(), 16 * sizeof(float));
  funcs_->vkUnmapMemory(device, buffer_memory_);

  // Not exactly a real animation system, just advance on every frame for now.
  rotation_ += 1.0f;

  funcs_->vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
  funcs_->vkCmdBindDescriptorSets(
    cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1,
    &descriptor_set_[window_->currentFrame()], 0, nullptr);
  VkDeviceSize vb_offset = 0;
  funcs_->vkCmdBindVertexBuffers(cb, 0, 1, &buffer_, &vb_offset);

  VkViewport viewport = {
    .x = 0,
    .y = 0,
    .width = static_cast<float>(sz.width()),
    .height = static_cast<float>(sz.height()),
    .minDepth = 0,
    .maxDepth = 1,
  };
  funcs_->vkCmdSetViewport(cb, 0, 1, &viewport);

  VkRect2D scissor = {
    .offset = {.x = 0, .y = 0},
    .extent =
      {
        .width = static_cast<unsigned int>(sz.width()),
        .height = static_cast<unsigned int>(sz.height()),
      },
  };

  funcs_->vkCmdSetScissor(cb, 0, 1, &scissor);
  funcs_->vkCmdDraw(cb, /* vertex count */ 3, /* instance count */ 1,
                    /* first vertex */ 0, /* first instance */ 0);
  funcs_->vkCmdEndRenderPass(command_buffer);

  window_->frameReady();
  window_->requestUpdate(); // render continuously, throttled by the
                            // presentation rate
}
