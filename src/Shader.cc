#include "vulkan-engine/Shader.h"

#include <QFile>
#include <QVulkanDeviceFunctions>
#include <QtConcurrent/QtConcurrent>

void vulkan_engine::Shader::load(QVulkanInstance* inst, VkDevice dev, const QString& fn) {
  reset();
  maybe_running_ = true;
  future_ = QtConcurrent::run([inst, dev, fn]() {
    ShaderData sd;
    QFile f(fn);
    if(!f.open(QIODevice::ReadOnly)) {
      qWarning("Failed to open %s", qPrintable(fn));
      return sd;
    }
    QByteArray blob = f.readAll();
    VkShaderModuleCreateInfo shader_info;
    memset(&shader_info, 0, sizeof(shader_info));
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = blob.size();
    shader_info.pCode = reinterpret_cast<const uint32_t*>(blob.constData());
    VkResult err = inst->deviceFunctions(dev)->vkCreateShaderModule(
      dev, &shader_info, nullptr, &sd.shader_module);
    if(err != VK_SUCCESS) {
      qWarning("Failed to create shader module: %d", err);
      return sd;
    }
    return sd;
  });
}

vulkan_engine::ShaderData* vulkan_engine::Shader::data() {
  if(maybe_running_ && !data_.isValid())
    data_ = future_.result();
  return &data_;
}

void vulkan_engine::Shader::reset() {
  *data() = ShaderData();
  maybe_running_ = false;
}
