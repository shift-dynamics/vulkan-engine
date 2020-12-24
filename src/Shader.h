#ifndef SHIFT_GUI_SHADER_H_
#define SHIFT_GUI_SHADER_H_

#include <QFuture>
#include <QVulkanInstance>

namespace vulkan_engine {

struct ShaderData {
  bool isValid() const {
    return shader_module != VK_NULL_HANDLE;
  }
  VkShaderModule shader_module = VK_NULL_HANDLE;
};

class Shader {
public:
  void load(QVulkanInstance* inst, VkDevice dev, const QString& fn);
  ShaderData* data();
  bool isValid() {
    return data()->isValid();
  }
  void reset();

private:
  bool maybe_running_ = false;
  QFuture<ShaderData> future_;
  ShaderData data_;
};

}

#endif
