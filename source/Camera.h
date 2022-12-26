#pragma once

#include "Timer.h"

using namespace dae;

class Camera
{
public:
	Camera() = default;
	Camera(const Vector3& origin, const float fovAngle, const float aspectRatio);

	~Camera() = default;
	Camera(const Camera& other) = delete;
	Camera& operator=(const Camera& rhs) = delete;
	Camera(Camera&& other) = delete;
	Camera& operator=(Camera&& rhs) = delete;

	void Initialize(float fovAngle = 90.f, Vector3 origin = { 0.f,0.f,0.f }, float aspectRatio = 1.f);

	void Update(const Timer* pTimer);

	Matrix GetViewMatrix() const { return m_ViewMatrix; }
	Matrix GetInvViewMatrix() const { return m_InvViewMatrix; }
	Matrix GetProjectionMatrix() const { return m_ProjectionMatrix; }

private:
	Vector3 m_Origin{};
	float m_FovAngle{};
	float m_Fov{};
	float m_AspectRatio;

	Vector3 m_Forward{ Vector3::UnitZ };
	Vector3 m_Up{ Vector3::UnitY };
	Vector3 m_Right{ Vector3::UnitX };

	float m_TotalPitch{};
	float m_TotalYaw{};

	Matrix m_InvViewMatrix{};
	Matrix m_ViewMatrix{};
	
	Matrix m_ProjectionMatrix{};

	void CalculateViewMatrix();
	void CalculateProjectionMatrix();

	void HandleKeyboardInput(float deltaTime, float moveSpeed);
	void HandleMouseInput(float deltaTime, float moveSpeed, float rotationSpeed);
	void MoveForward(float deltaTime, float moveSpeed);
	void MoveBackward(float deltaTime, float moveSpeed);
	void MoveUp(float deltaTime, float moveSpeed);
	void MoveDown(float deltaTime, float moveSpeed);
};

