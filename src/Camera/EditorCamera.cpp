#include "EditorCamera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

EditorCamera::EditorCamera(const EditorCamCreateInfo& createInfo)
	: m_Position( 0.0f ),
	m_Rotation( 0.0f),
	m_ViewMatrix( 1.0f ),
	m_ProjMatrix( 1.0f),
	m_Near(createInfo.Near),
	m_Far(createInfo.Far),
	m_FOV(createInfo.FOV),
	m_AspectRatio(createInfo.ViewportWidth / createInfo.ViewportHeight),
	m_ViewportWidth(createInfo.ViewportWidth),
	m_ViewportHeight(createInfo.ViewportHeight)
{
	CalculateView();
	CalculateProj();
}

void EditorCamera::SetPosition(glm::vec3& pos)
{
	m_Position = pos;
	CalculateView();
}

void EditorCamera::SetRotation(glm::vec3& rot)
{
	m_Rotation = rot;
	CalculateView();
}

void EditorCamera::SetNearClip(float nearClip)
{
	m_Near = nearClip;
	CalculateProj();
}

void EditorCamera::SetFarClip(float farClip)
{
	m_Far = farClip;
	CalculateProj();
}

void EditorCamera::SetFOV(float fov)
{
	glm::clamp(fov, 0.0f, 160.0f);
	m_FOV = fov;
	CalculateProj();
}



glm::vec3 EditorCamera::GetPosition() const
{
	return m_Position;
}

glm::vec3 EditorCamera::GetRotation() const
{
	return m_Rotation;
}

float EditorCamera::GetNearClip() const
{
	return m_Near;
}

float EditorCamera::GetFarClip() const
{
	return m_Far;
}

float EditorCamera::GetFOV() const
{
	return m_FOV;
}

glm::mat4 EditorCamera::GetProjMatrix() const
{
	return m_ProjMatrix;
}

glm::mat4 EditorCamera::GetViewMatrix() const
{
	return m_ViewMatrix;
}

glm::vec3 EditorCamera::GetUpDirection() const
{
	glm::quat rot(m_Rotation);
	return rot * glm::vec3( 0.0f, 1.0f, 0.0f);
}

glm::vec3 EditorCamera::GetRightDirection() const
{
	glm::quat rot(m_Rotation);
	return rot * glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 EditorCamera::GetForwardDirection() const
{
	glm::quat rot(m_Rotation);
	return rot * glm::vec3(0.0f, 0.0f, -1.0f);
}


void EditorCamera::CalculateView()
{
	glm::quat rot(m_Rotation);
	m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(rot);
	m_ViewMatrix = glm::inverse(m_ViewMatrix);
}

void EditorCamera::CalculateProj()
{
	m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
	m_ProjMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_Near, m_Far);
}
