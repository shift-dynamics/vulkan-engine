#ifndef SHIFT_GUI_TRIANGLERENDERER_H_
#define SHIFT_GUI_TRIANGLERENDERER_H_

#include <QVulkanWindow>

namespace vulkan_engine {

class TriangleRenderer : public QVulkanWindowRenderer {
public:
  TriangleRenderer(QVulkanWindow* w, bool msaa = false);

  void initResources() override;
  void initSwapChainResources() override;
  void releaseSwapChainResources() override;
  void releaseResources() override;

  void startNextFrame() override;

protected:
  VkShaderModule createShader(const QString& name);

  QVulkanWindow* window_;
  QVulkanDeviceFunctions* funcs_;

  VkDeviceMemory buffer_memory_ = VK_NULL_HANDLE;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  VkDescriptorBufferInfo
    uniform_buffer_info_[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkDescriptorSet descriptor_set_[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

  VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;

  QMatrix4x4 projection_;
  float rotation_ = 0.0f;
};

}

#endif
