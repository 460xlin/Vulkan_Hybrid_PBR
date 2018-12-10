#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Camera::Camera() :
    Camera(400, 400)
{
    look = glm::vec3(0, 0, -1);
    up = glm::vec3(0, 1, 0);
    right = glm::vec3(1, 0, 0);
}

Camera::Camera(unsigned int w, unsigned int h) :
    Camera(w, h, glm::vec3(0, 0, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0))
{}

Camera::Camera(unsigned int w, unsigned int h, const glm::vec3 &e, const glm::vec3 &r, const glm::vec3 &worldUp) :
    fovy(45),
    width(w),
    height(h),
    near_clip(0.1f),
    far_clip(1000.f),
    eye(e),
    ref(r),
    world_up(worldUp)
{
    RecomputeAttributes();
}

Camera::Camera(const Camera &c) :
    fovy(c.fovy),
    width(c.width),
    height(c.height),
    near_clip(c.near_clip),
    far_clip(c.far_clip),
    aspect(c.aspect),
    eye(c.eye),
    ref(c.ref),
    look(c.look),
    up(c.up),
    right(c.right),
    world_up(c.world_up),
    V(c.V),
    H(c.H)
{}

void Camera::UpdateEyeAndRef(const glm::vec3& eye_in,
    const glm::vec3& ref_in) {
    eye = eye_in;
    ref = ref_in;
    RecomputeAttributes();
}


void Camera::RecomputeAttributes()
{
    look = glm::normalize(ref - eye);
    right = glm::normalize(glm::cross(look, world_up));
    up = glm::cross(right, look);

    float tan_fovy = tan(glm::radians(fovy / 2));
    float len = glm::length(ref - eye);
    aspect = (float)width / (float)height;
    V = up * len*tan_fovy;
    H = right * len*aspect*tan_fovy;
}

glm::mat4 Camera::GetViewProjMat()
{
    return glm::perspective(glm::radians(fovy),
        (float)width / (float)height,
        near_clip, far_clip) * glm::lookAt(eye, ref, up);
}

glm::mat4 Camera::GetView()
{
    glm::mat4 View = glm::lookAt(
        eye,
        ref,
        up
    );

    return View;
}

glm::mat4 Camera::GetProj()
{
    glm::mat4 projectionMatrix = glm::perspective(
        glm::radians(fovy), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90?(extra wide) and 30?(quite zoomed in)
        aspect,             // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
        near_clip,               // Near clipping plane. Keep as big as possible, or you'll get precision issues.
        far_clip             // Far clipping plane. Keep as little as possible.
    );

    return projectionMatrix;
}

glm::vec3 Camera::GetForward()
{
    return look;
}

glm::vec3 Camera::GetPos()
{
    return eye;
}

void Camera::RotateAboutUp(float deg)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(deg), up);
    ref = ref - eye;
    ref = glm::vec3(rotation * glm::vec4(ref, 1));
    ref = ref + eye;
    RecomputeAttributes();
}

void Camera::RotateAboutRight(float deg)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(deg), right);
    ref = ref - eye;
    ref = glm::vec3(rotation * glm::vec4(ref, 1));
    ref = ref + eye;
    RecomputeAttributes();
}

void Camera::TranslateAlongLook(float amt)
{
    glm::vec3 translation = look * amt;
    eye += translation;
    ref += translation;
}

void Camera::TranslateAlongRight(float amt)
{
    glm::vec3 translation = right * amt;
    eye += translation;
    ref += translation;
}
void Camera::TranslateAlongUp(float amt)
{
    glm::vec3 translation = world_up * amt;
    eye += translation;
    ref += translation;
}