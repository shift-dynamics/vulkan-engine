#ifndef SHIFT_GUI_CAMERA_H_
#define SHIFT_GUI_CAMERA_H_

#include <QMatrix4x4>
#include <QVector3D>

namespace vulkan_engine {

class Camera {
public:
  Camera(const QVector3D& pos);

  void yaw(float degrees);
  void pitch(float degrees);
  void walk(float amount);
  void strafe(float amount);

  QMatrix4x4 viewMatrix() const;

private:
  QVector3D forward_;
  QVector3D right_;
  QVector3D up_;
  QVector3D pos_;
  float yaw_;
  float pitch_;
  QMatrix4x4 yaw_matrix_;
  QMatrix4x4 pitch_matrix_;
};

}

#endif
