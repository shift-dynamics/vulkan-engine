#include "vulkan-engine/VulkanWindow.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QPlainTextEdit>
#include <QPointer>
#include <QVulkanInstance>
#include <QMainWindow>
#include <QHBoxLayout>
Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

int main(int argc, char *argv[]) {

  QApplication app(argc, argv);

  Q_INIT_RESOURCE(shaders);

  QVulkanInstance inst;
#ifndef Q_OS_ANDROID
  inst.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
#else
  inst.setLayers(QByteArrayList() << "VK_LAYER_GOOGLE_threading"
                                  << "VK_LAYER_LUNARG_parameter_validation"
                                  << "VK_LAYER_LUNARG_object_tracker"
                                  << "VK_LAYER_LUNARG_core_validation"
                                  << "VK_LAYER_LUNARG_image"
                                  << "VK_LAYER_LUNARG_swapchain"
                                  << "VK_LAYER_GOOGLE_unique_objects");
#endif
  if (!inst.create()) {
    qFatal("Failed to create Vulkan instance: %d", inst.errorCode());
  }

  vulkan_engine::VulkanWindow *window = new vulkan_engine::VulkanWindow;
  window->setVulkanInstance(&inst);

  QWidget* container = QWidget::createWindowContainer(window);
  container->setFocusPolicy(Qt::StrongFocus);
  container->setFocus();
  container->setMinimumWidth(400);
  container->sizePolicy().setHorizontalPolicy(QSizePolicy::Expanding);
  container->sizePolicy().setVerticalPolicy(QSizePolicy::Expanding);
 
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(container);

  QMainWindow* main_window = new QMainWindow;
  main_window->setLayout(layout);  
  main_window->setMouseTracking(true);
  main_window->showMaximized();

  return app.exec();
}


