#include "vulkan-engine/VulkanWindow.h"
#include "vulkan-engine/Workspace.h"
#include "ui_Workspace.h"

#include <QApplication>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLoggingCategory>
#include <QSizePolicy>
#include <QVulkanInstance>
#include <QtCore/QDebug>
#include <QtWidgets/QTreeWidget>

vulkan_engine::Workspace::Workspace(vulkan_engine::VulkanWindow* vulkanWindow,
                                 QWidget* parent)
  : QMainWindow(parent)
  , ui_(new Ui::MainWindow) {

  ui_->setupUi(this);
  ui_->tabWidget->setMinimumWidth(300);
  ui_->tabWidget->sizePolicy().setHorizontalPolicy(QSizePolicy::Minimum);
  ui_->tabWidget->sizePolicy().setVerticalPolicy(QSizePolicy::Minimum);
  // ui_->tabWidget->sizePolicy().setHorizontalStretch(1);
  // ui_->splitter->setSizes(QList<int>() << 100 << 500);

  QWidget* container = QWidget::createWindowContainer(vulkanWindow);
  container->setFocusPolicy(Qt::StrongFocus);
  container->setFocus();
  container->setMinimumWidth(400);
  container->sizePolicy().setHorizontalPolicy(QSizePolicy::Expanding);
  container->sizePolicy().setVerticalPolicy(QSizePolicy::Expanding);
  // container->sizePolicy().setHorizontalStretch(3);

  QHBoxLayout* layout = findChild<QHBoxLayout*>("horizontalLayout_2");
  layout->addWidget(container);

  // setLayout(layout);
}

vulkan_engine::Workspace::~Workspace() {
  delete ui_;
}

