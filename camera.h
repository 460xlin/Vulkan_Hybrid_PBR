
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
    glm::vec3 pos, forward;
    glm::vec3 up = glm::vec3(0, 1, 0);
    float tanslationSpeed = 10.f;
    float turnSpeed = 10.f;
    void UpdateViewMatrix();

public:
    Camera(float aspectRatio);
    glm::mat4 GetViewMat();
    glm::mat4 GetProjMat();
    glm::vec3 GetPos();
    glm::vec3 GetForward();
    void MoveForward(float deltaTime);
    void MoveRight(float deltaTime);
    void MoveUp(float deltaTime);

    ~Camera();

    void UpdateOrbit(float deltaX, float deltaY, float deltaZ);
};
