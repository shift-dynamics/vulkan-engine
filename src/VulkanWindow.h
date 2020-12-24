#ifndef SHIFT_GUI_VULKANWINDOW_H_
#define SHIFT_GUI_VULKANWINDOW_H_

#include "vulkan-engine/TriangleRenderer.h"

#include <QWidget>

namespace vulkan_engine {

class VulkanWindow;

class VulkanRenderer : public TriangleRenderer {
public:
  VulkanRenderer(VulkanWindow* w);

  void initResources() override;
  void startNextFrame() override;
};

class VulkanWindow : public QVulkanWindow {
  Q_OBJECT

public:
  QVulkanWindowRenderer* createRenderer() override;

signals:
  void vulkanInfoReceived(const QString& text);
  void frameQueued(int colorValue);
};

}

#endif
