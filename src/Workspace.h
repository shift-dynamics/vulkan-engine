#ifndef SHIFT_GUI_WORKSPACE_H_
#define SHIFT_GUI_WORKSPACE_H_

#include <QtWidgets/QMainWindow>

namespace Ui {
class MainWindow;
}

namespace vulkan_engine {

class VulkanWindow;

class Workspace : public QMainWindow {
  Q_OBJECT

public:
  explicit Workspace(VulkanWindow* vulkanWindow, QWidget* parent = 0);
  ~Workspace();

private:
  Ui::MainWindow* ui_;

private slots:
};

}

#endif
