#include "vulkan-engine/VulkanWindow.h"

#include <QApplication>
#include <QFileDialog>
#include <QLCDNumber>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QVulkanFunctions>

QVulkanWindowRenderer* vulkan_engine::VulkanWindow::createRenderer() {
  return new VulkanRenderer(this);
}

vulkan_engine::VulkanRenderer::VulkanRenderer(VulkanWindow* w)
  : TriangleRenderer(w) {}

void vulkan_engine::VulkanRenderer::initResources() {
  TriangleRenderer::initResources();

  QVulkanInstance* inst = window_->vulkanInstance();
  funcs_ = inst->deviceFunctions(window_->device());

  QString info;
  info += QString().asprintf("Number of physical devices: %d\n",
                            window_->availablePhysicalDevices().count());

  QVulkanFunctions* f = inst->functions();
  VkPhysicalDeviceProperties props;
  f->vkGetPhysicalDeviceProperties(window_->physicalDevice(), &props);
  info += QString().asprintf(
    "Active physical device name: '%s' version %d.%d.%d\nAPI version "
    "%d.%d.%d\n",
    props.deviceName, VK_VERSION_MAJOR(props.driverVersion),
    VK_VERSION_MINOR(props.driverVersion),
    VK_VERSION_PATCH(props.driverVersion), VK_VERSION_MAJOR(props.apiVersion),
    VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));

  info += QStringLiteral("Supported instance layers:\n");
  for(const QVulkanLayer& layer : inst->supportedLayers())
    info +=
      QString().asprintf("    %s v%u\n", layer.name.constData(), layer.version);
  info += QStringLiteral("Enabled instance layers:\n");
  for(const QByteArray& layer : inst->layers())
    info += QString().asprintf("    %s\n", layer.constData());

  info += QStringLiteral("Supported instance extensions:\n");
  for(const QVulkanExtension& ext : inst->supportedExtensions())
    info +=
      QString().asprintf("    %s v%u\n", ext.name.constData(), ext.version);
  info += QStringLiteral("Enabled instance extensions:\n");
  for(const QByteArray& ext : inst->extensions())
    info += QString().asprintf("    %s\n", ext.constData());

  info +=
    QString().asprintf("Color format: %u\nDepth-stencil format: %u\n",
                      window_->colorFormat(), window_->depthStencilFormat());

  info += QStringLiteral("Supported sample counts:");
  const QVector<int> sampleCounts = window_->supportedSampleCounts();
  for(int count : sampleCounts)
    info += QLatin1Char(' ') + QString::number(count);
  info += QLatin1Char('\n');

  emit static_cast<VulkanWindow*>(window_)->vulkanInfoReceived(info);
}

void vulkan_engine::VulkanRenderer::startNextFrame() {
  TriangleRenderer::startNextFrame();
  emit static_cast<VulkanWindow*>(window_)->frameQueued(int(rotation_) % 360);
}
