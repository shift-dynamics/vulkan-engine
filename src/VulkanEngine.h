#ifndef SHIFT_GUI_VULKANRENDERER_H_
#define SHIFT_GUI_VULKANRENDERER_H_

#include <QVulkanWindow>

#include "vulkan-engine/MeshData.h"

namespace vulkan_engine {

class VulkanEngine : public QVulkanWindowRenderer {
public:
  VulkanEngine(QVulkanWindow* w, bool msaa = false);

  void initResources() override;
  void initSwapChainResources() override;
  void releaseSwapChainResources() override;
  void releaseResources() override;
  void startNextFrame() override;
  float height();
  float width();

protected:

  VkShaderModule createShader(const QString& name);

  struct Material {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
  };

  struct RenderObject {
    Material* material = nullptr;
    vulkan_engine::MeshData* mesh_data = nullptr;
    vulkan_engine::MaterialData* material_data = nullptr;
    QMatrix4x4 transform = QMatrix4x4();
  };

  std::vector<RenderObject> renderables_;




  QVulkanWindow* window_ = nullptr;
  QVulkanDeviceFunctions* funcs_ = nullptr;

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

  QMatrix4x4 view_ = QMatrix4x4();
  QMatrix4x4 projection_ = QMatrix4x4();

  float rotation_ = 0.0f;
};

}

#endif
