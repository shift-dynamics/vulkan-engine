#ifndef SHIFT_GUI_ORBITALCAMERA_H_
#define SHIFT_GUI_ORBITALCAMERA_H_

#include <cmath>

#include <QtGui/qevent.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qvector3d.h>
#include <QtWidgets/qwidget.h>

namespace vulkan_engine {

class VulkanEngine;

enum class OrbitalCameraMode : int {
  None = 0x00,
  Translate = 0x01,
  Rotate = 0x02,
  Scale = 0x04
};

class OrbitalCamera {

public:
  OrbitalCamera(VulkanEngine* engine) {
    engine_ = engine;
    view_.setToIdentity();
    setLookAt(QVector3D(-2.0, -2.0, -2.0), QVector3D(0.0, 0.0, 0.0),
              QVector3D(0.0, 0.0, -1.0));
  }

  void setLookAt(const QVector3D& pos, const QVector3D& to,
                 const QVector3D& up);

  inline float cameraDistance() {
    return camera_distance_;
  }

  void update();

  inline QMatrix4x4 getViewMatrix() const {
    return view_;
  }

  inline double scroll() const {
    return scroll_;
  }

  inline void setMode(OrbitalCameraMode mode) {
    mode_ = mode;
  }

  inline void setOldPoint(const QPoint& pos) {
    old_point_ = pos;
  }

  inline void setNewPoint(const QPoint& pos) {
    new_point_ = pos;
  }

  inline void setScroll(double scroll) {
    scroll_ = scroll;
  }

  void mousePressEvent(QMouseEvent* ev);

  void mouseMoveEvent(QMouseEvent* ev);

  void mouseReleaseEvent(QMouseEvent* ev);

  void wheelEvent(QWheelEvent* ev);

private:
  QVector3D getVector(int x, int y);
  void updateTranslate();
  void updateRotate();
  void updateScale();

  VulkanEngine* engine_ = nullptr;
  QMatrix4x4 view_;
  double scroll_ = 0.0;
  QPoint old_point_ = QPoint(0, 0);
  QPoint new_point_ = QPoint(0, 0);

  OrbitalCameraMode mode_ = OrbitalCameraMode::None;

  float camera_distance_ = 3.0;
};

} // namespace vulkan_engine

#endif
