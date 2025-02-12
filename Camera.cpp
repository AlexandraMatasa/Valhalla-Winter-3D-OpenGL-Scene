#include "Camera.hpp"

namespace gps {

    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = glm::normalize(cameraUp);
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }


    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }


    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == MOVE_FORWARD) {
            cameraPosition += speed * cameraFrontDirection;
        }
        if (direction == MOVE_BACKWARD) {
            cameraPosition -= speed * cameraFrontDirection;
        }
        if (direction == MOVE_RIGHT) {
            cameraPosition += speed * cameraRightDirection;
        }
        if (direction == MOVE_LEFT) {
            cameraPosition -= speed * cameraRightDirection;
        }
        if (direction == MOVE_UP) {
            cameraPosition.y += speed;
        }
        if (direction == MOVE_DOWN) {
            cameraPosition.y -= speed;
        }
        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 front;
        front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
        front.y = sin(glm::radians(pitch));
        front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));

        cameraFrontDirection = glm::normalize(front);

        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, worldUp));

        cameraTarget = cameraPosition + cameraFrontDirection;
    }
}