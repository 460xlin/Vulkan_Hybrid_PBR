
#pragma once

#include <glm/glm.hpp>

struct CameraBufferObject {
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

class Camera {
private:
    CameraBufferObject cameraBufferObject;
    float r, theta, phi;
    glm::vec3 pos;

public:
    Camera(float aspectRatio);
    glm::mat4 GetViewMat();
    glm::mat4 GetProjMat();
    glm::vec3 GetPos();
    ~Camera();

    void UpdateOrbit(float deltaX, float deltaY, float deltaZ);
};
