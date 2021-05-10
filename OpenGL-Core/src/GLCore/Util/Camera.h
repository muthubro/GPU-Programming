#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "GLCore/Core/Timestep.h"

#include "GLCore/Events/Event.h"
#include "GLCore/Events/MouseEvent.h"

namespace GLCore::Utils
{

class Camera
{
public:
    Camera(const glm::mat4& projectionMatrix);

    void OnEvent(Event& e);
    void OnUpdate(Timestep ts);

    void UpdateCamera();

    const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    const glm::mat4& GetViewProjection() const { return m_ViewProjection; }

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::quat& GetOrientation() const { return m_Orientation; }
    const glm::vec3& GetFocalPoint() const { return m_FocalPoint; }

    float GetDistance() const { return m_Distance; }
    float GetPitch() const { return m_Pitch; }
    float GetYaw() const { return m_Yaw; }

    const glm::vec3& GetUpDirection() const { return m_UpDirection; }
    const glm::vec3& GetRightDirection() const { return m_RightDirection; }
    const glm::vec3& GetForwardDirection() const { return m_ForwardDirection; }

    void SetProjectionMatrix(const glm::mat4& projectionMatrix) { m_ProjectionMatrix = projectionMatrix; }
    void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; }

    void SetFocalPoint(const glm::vec3& focalPoint) { m_FocalPoint = focalPoint; }

    void SetDistance(float distance) { m_Distance = distance; }
    void SetPitch(float pitch) { m_Pitch = pitch; }
    void SetYaw(float yaw) { m_Yaw = yaw; }

private:
    glm::mat4 m_ViewMatrix, m_ProjectionMatrix, m_ViewProjection;
    glm::vec3 m_Position, m_FocalPoint;
    glm::quat m_Orientation;
    glm::vec3 m_UpDirection, m_RightDirection, m_ForwardDirection;

    float m_Distance;
    float m_Yaw, m_Pitch;

    uint32_t m_ViewportWidth, m_ViewportHeight;

    glm::vec2 m_MousePosition = glm::vec2(0.0f);

    void CalculateOrientation();
    void CalculatePosition();
    void CalculateViewMatrix();
    void SetDirections();

    bool OnMouseScroll(MouseScrolledEvent& e);

    void MousePan(const glm::vec2& delta);
    void MouseRotate(const glm::vec2& delta);
    void MouseZoom(float delta);

    std::pair<float, float> PanSpeed() const;
    float RotationSpeed() const;
    float ZoomSpeed() const;
};

}
