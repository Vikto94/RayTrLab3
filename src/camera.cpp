#include "camera.h"


Camera::Camera() {
  position = glm::vec3 ( 0.0f,0.0f,1.0f );
  up = glm::vec3 ( 0.0, 1.0, 0.0 );
  lookat = glm::vec3 ( 0.0, 0.0, 0.0 );

  calculateViewingCoordinates();
}

glm::vec3 Camera::getPosition() {
  return position;
}

void Camera::setPosition ( glm::vec3 pos ) {
  position = pos;
}

void Camera::setPosition ( float x,float y,float z ) {
  position = glm::vec3 ( x,y,z );
}

void Camera::calculateViewingCoordinates() {
  w = glm::normalize ( ( lookat - position ) );
  u = glm::normalize ( glm::cross ( up, w ) );
  v = glm::cross ( w, u );
}