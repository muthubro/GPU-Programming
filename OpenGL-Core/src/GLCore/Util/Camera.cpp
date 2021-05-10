#include "glpch.h"
#include "Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "GLCore/Core/Input.h"
#include "GLCore/Core/KeyCodes.h"
#include "GLCore/Core/MouseButtonCodes.h"

#define PI 3.14159265f

namespace GLCore::Utils
{

Camera::Camera(const glm::mat4& projectionMatrix)
    : m_ProjectionMatrix(projectionMatrix)
{
    m_FocalPoint = { 0.0f, 0.0f, 0.0f };
    m_Pitch = -PI / 4.0f;
    m_Yaw = -3.0f * PI / 4.0f;
    m_Distance = 9.0f;

    UpdateCamera();
}

void Camera::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseScrolledEvent>(GLCORE_BIND_EVENT_FN(Camera::OnMouseScroll));
}

void Camera::OnUpdate(Timestep ts)
{
    const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
    auto delta = mouse - m_MousePosition;
    m_MousePosition = mouse;

    delta *= ts;

    if (Input::IsKeyPressed(HZ_KEY_LEFT_SHIFT))
    {
        if (Input::IsMouseButtonPressed(HZ_MOUSE_BUTTON_LEFT))
            MousePan(delta);
    }

    if (Input::IsMouseButtonPressed(HZ_MOUSE_BUTTON_MIDDLE))
        MouseRotate(delta);

    UpdateCamera();
}

void Camera::CalculateOrientation()
{
    m_Orientation = glm::quat(glm::vec3(m_Pitch, m_Yaw, 0.0f));
}

void Camera::CalculatePosition()
{
    m_Position = m_FocalPoint - m_ForwardDirection * m_Distance;
}

void Camera::CalculateViewMatrix()
{
    m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(m_Orientation);
    m_ViewMatrix = glm::inverse(m_ViewMatrix);
    m_ViewProjection = m_ProjectionMatrix * m_ViewMatrix;
}

void Camera::SetDirections()
{
    m_UpDirection = glm::rotate(m_Orientation, glm::vec3(0.0f, 1.0f, 0.0f));
    m_RightDirection = glm::rotate(m_Orientation, glm::vec3(1.0f, 0.0f, 0.0f));
    m_ForwardDirection = glm::rotate(m_Orientation, glm::vec3(0.0f, 0.0f, -1.0f));
}

void Camera::UpdateCamera()
{
    CalculateOrientation();
    SetDirections();
    CalculatePosition();
    CalculateViewMatrix();
}

bool Camera::OnMouseScroll(MouseScrolledEvent& e)
{
    float delta = e.GetYOffset();
    MouseZoom(delta);
    return false;
}

void Camera::MousePan(const glm::vec2& delta)
{
    auto [xSpeed, ySpeed] = PanSpeed();
    m_FocalPoint += -m_RightDirection * delta.x * xSpeed * m_Distance;
    m_FocalPoint += m_UpDirection * delta.y * ySpeed * m_Distance;
}

void Camera::MouseRotate(const glm::vec2& delta)
{
    float speed = RotationSpeed();
    float yawSign = m_UpDirection.y < 0.0f ? -1.0f : 1.0f;
    m_Pitch -= delta.y * speed;
    m_Yaw -= yawSign * delta.x * speed;
}

void Camera::MouseZoom(float delta)
{
    m_Distance -= delta * ZoomSpeed();
    if (m_Distance < 1.0f)
    {
        m_FocalPoint += m_ForwardDirection;
        m_Distance = 1.0f;
    }
}

std::pair<float, float> Camera::PanSpeed() const
{
    float x = std::min(m_ViewportWidth / 1000.0f, 2.4f);
    float xFactor = 0.0732f * x * x - 0.3556f * x + 0.6042f;

    float y = std::min(m_ViewportHeight / 1000.0f, 2.4f);
    float yFactor = 0.0732f * y * y - 0.3556f * y + 0.6042f;

    return { xFactor, yFactor };
}

float Camera::RotationSpeed() const
{
    return 1.0f;
}

float Camera::ZoomSpeed() const
{
    float distance = m_Distance * 0.2f;
    distance = std::max(distance, 0.0f);
    float speed = distance;
    speed = std::min(speed, 100.0f);
    return speed;
}

}