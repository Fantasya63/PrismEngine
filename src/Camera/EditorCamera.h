#pragma once
#include <glm/glm.hpp>

class EditorCamera
{
public:
	struct EditorCamCreateInfo {
		float Near = 0.01f;
		float Far = 100.0f;
		float FOV = 75.0f;
		float ViewportWidth = 1280.0f;
		float ViewportHeight = 720.0f;
	};

	EditorCamera() = default;
	EditorCamera(const EditorCamCreateInfo& createInfo);
	~EditorCamera() = default;

	void SetPosition(glm::vec3& pos);
	void SetRotation(glm::vec3& rot);
	void SetNearClip(float nearClip);
	void SetFarClip(float farClip);
	void SetFOV(float fov);

	glm::vec3 GetPosition() const;
	glm::vec3 GetRotation() const;
	float GetNearClip() const;
	float GetFarClip() const;
	float GetFOV() const;
	glm::mat4 GetProjMatrix() const;
	glm::mat4 GetViewMatrix() const;

	glm::vec3 GetUpDirection() const;
	glm::vec3 GetRightDirection() const;
	glm::vec3 GetForwardDirection() const;


	float ZoomSpeed = 10.0f;
	float MovementSpeed = 2.0f;

private:
	void CalculateView();
	void CalculateProj();

	glm::mat4 m_ViewMatrix{ 1.0f };
	glm::mat4 m_ProjMatrix{ 1.0f };

	glm::vec3 m_Position { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_Rotation{ 0.0f, 0.0f, 0.0f };
	

	float m_Near = 0.01f;
	float m_Far = 1000.0f;
	

	// Field Of View in Y axis
	float m_FOV = 45.0f;
	float m_AspectRatio{ 16.0f / 9.0f };

	float m_ViewportWidth{ 1280.0f };
	float m_ViewportHeight{ 720.0f };
};