#include "vulkan-engine/OrbitalCamera.h"
#include "vulkan-engine/VulkanEngine.h"
#include "vulkan-engine/VulkanWindow.h"


QMatrix4x4 getRotation(QMatrix4x4& mat) {
  QMatrix4x4 rotation = mat;
  rotation(0, 3) = 0.f;
  rotation(1, 3) = 0.f;
  rotation(2, 3) = 0.f;
  return rotation;
}

void vulkan_engine::OrbitalCamera::wheelEvent(QWheelEvent* ev) {
  float delta = ev->angleDelta().y() / 1000.f;
  QVector3D temp = view_.transposed().column(2).toVector3D() * delta;
  camera_distance_ += delta;
  view_.translate(temp);
  update();
}

void vulkan_engine::OrbitalCamera::setLookAt(const QVector3D& pos, const QVector3D& to,
                                     const QVector3D& up) {
  view_.setToIdentity();
  view_.lookAt(pos, to, up);
  update();
}

void vulkan_engine::OrbitalCamera::mousePressEvent(QMouseEvent* ev) {
  setOldPoint(ev->pos());
  setNewPoint(ev->pos());
  switch(ev->button()) {
    case Qt::LeftButton: {
      setMode(OrbitalCameraMode::Rotate);
      break;
    }
    case Qt::RightButton: {
      setMode(OrbitalCameraMode::Translate);
      break;
    }
    case Qt::MiddleButton: {
      setMode(OrbitalCameraMode::Scale);
      break;
    }
    default:
      break;
  }
}

void vulkan_engine::OrbitalCamera::update() {
  switch(mode_) {
    case OrbitalCameraMode::Translate: {
      updateTranslate();
      break;
    }
    case OrbitalCameraMode::Rotate: {
      updateRotate();
      break;
    }
    case OrbitalCameraMode::Scale: {
      updateScale();
      break;
    }
    default:
      break;
  }
}

void vulkan_engine::OrbitalCamera::mouseMoveEvent(QMouseEvent* ev) {
  setNewPoint(ev->pos());
  update();
  setOldPoint(ev->pos());
}

void vulkan_engine::OrbitalCamera::mouseReleaseEvent(QMouseEvent*) {
  setMode(OrbitalCameraMode::None);
}

void vulkan_engine::OrbitalCamera::updateScale() {
  const double dy =
    20.0 * (new_point_.y() - old_point_.y()) / engine_->height();
  scroll_ += dy;
}

void vulkan_engine::OrbitalCamera::updateTranslate() {
  QMatrix4x4 camera2objMat = getRotation(view_).transposed();
  const double dx =
    10.0 * (new_point_.x() - old_point_.x()) / engine_->width();
  const double dy =
    -10.0 * (new_point_.y() - old_point_.y()) / engine_->height();
  view_.translate(camera2objMat * QVector3D(dx, dy, 0.0));
}

void vulkan_engine::OrbitalCamera::updateRotate() {
  const QVector3D p0 = getVector(old_point_.x(), old_point_.y());
  const QVector3D p1 = getVector(new_point_.x(), new_point_.y());

  const double radians =
    std::acos(std::min(1.0f, QVector3D::dotProduct(p0, p1)));
  const QVector3D rotation_axis = QVector3D::crossProduct(p0, p1);
  QMatrix4x4 temp;
  const double degrees = radians * 180.0 / M_PI;
  temp.rotate(4.0 * degrees, rotation_axis);

  /*! @TODO if user has clicked on a vertex in model, find rotation center to
  apply rotation about user-defined position, otherwise, rotate camera about a
  point 1 m in front of camera */
  QMatrix4x4 view_to_rotation_center;
  view_to_rotation_center.translate(0, 0, 1);
  QMatrix4x4 rotation_center = view_to_rotation_center * view_;

  QMatrix4x4 rotation_center_to_view;
  rotation_center_to_view.translate(0, 0, -1);
  view_ = rotation_center_to_view * temp * rotation_center;
}

QVector3D vulkan_engine::OrbitalCamera::getVector(int x, int y) {
  QVector3D pt(2.0 * x / engine_->width() - 1.0,
               -2.0 * y / engine_->height() + 1.0, 0.0);

  const double r = pt.x() * pt.x() + pt.y() * pt.y();
  if(r < 1.0) {
    pt.setZ(std::sqrt(1.0 - r));
  } else {
    pt.normalize();
  }
  return pt;
}
