#include "pch.h"
#include "Camera.h"

Camera::Camera(const Vector3& origin, const float fovAngle, const float aspectRatio)
	: m_Origin(origin),
	m_FovAngle(fovAngle),
	m_AspectRatio(aspectRatio)
{
	m_Fov = tanf(m_FovAngle * TO_RADIANS / 2.f);

	CalculateProjectionMatrix();
}

void Camera::Initialize(float fovAngle, Vector3 origin, float aspectRatio)
{
	m_FovAngle = fovAngle;
	m_Fov = tanf(fovAngle * TO_RADIANS / 2.f);

	m_Origin = origin;

	m_AspectRatio = aspectRatio;

	CalculateProjectionMatrix();
}

void Camera::Update(const Timer* pTimer)
{
	const float deltaTime = pTimer->GetElapsed();
	constexpr float moveSpeed = 10.f;
	constexpr float rotationSpeed = PI_2;

	// Keyboard Input
	HandleKeyboardInput(deltaTime, moveSpeed);

	// Mouse Input 
	HandleMouseInput(deltaTime, moveSpeed, rotationSpeed);

	const Matrix finalRotation = Matrix::CreateRotation(m_TotalPitch, m_TotalYaw, 0);

	m_Forward = finalRotation.TransformVector(Vector3::UnitZ);

	//Update Matrices
	CalculateViewMatrix();
//	CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
}

void Camera::CalculateViewMatrix()
{
	m_Right = Vector3::Cross(Vector3::UnitY, m_Forward).Normalized();
	m_Up = Vector3::Cross(m_Forward, m_Right);

	m_InvViewMatrix = Matrix
	{
		m_Right,
		m_Up,
		m_Forward,
		m_Origin
	};

	m_ViewMatrix = Matrix::Inverse(m_InvViewMatrix);
}

void Camera::CalculateProjectionMatrix()
{
	constexpr float nearPlane{ 0.1f };
	constexpr float farPlane{ 100.f };

	constexpr float a{ farPlane / (farPlane - nearPlane) };
	constexpr float b{ -(farPlane * nearPlane) / (farPlane - nearPlane) };

	m_ProjectionMatrix =
	{
		{1 / (m_Fov * m_AspectRatio),0,0,0},
		{0,1 / m_Fov,0,0},
		{0,0,a,1},
		{0,0,b,0}
	};
}

void Camera::HandleKeyboardInput(float deltaTime, float moveSpeed)
{
	const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
	if (pKeyboardState[SDL_SCANCODE_W])
	{
		MoveForward(deltaTime, moveSpeed);
	}
	if (pKeyboardState[SDL_SCANCODE_S])
	{
		MoveBackward(deltaTime, moveSpeed);
	}
	if (pKeyboardState[SDL_SCANCODE_D])
	{
		m_Origin.x += moveSpeed * deltaTime;
	}
	if (pKeyboardState[SDL_SCANCODE_A])
	{
		m_Origin.x -= moveSpeed * deltaTime;
	}
	if (pKeyboardState[SDL_SCANCODE_Q])
	{
		MoveUp(deltaTime, moveSpeed);
	}
	if (pKeyboardState[SDL_SCANCODE_E])
	{
		MoveDown(deltaTime, moveSpeed);
	}
}
void Camera::HandleMouseInput(float deltaTime, float moveSpeed, float rotationSpeed)
{
	int mouseX{}, mouseY{};
	const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);
	if (mouseState & SDL_BUTTON_LMASK && mouseState & SDL_BUTTON_RMASK)
	{
		if (mouseY < 0)
		{
			MoveUp(deltaTime, moveSpeed);
		}
		else if (mouseY > 0)
		{
			MoveDown(deltaTime, moveSpeed);
		}
	}
	else if (mouseState & SDL_BUTTON_RMASK)
	{
		if (mouseX != 0)
		{
			m_TotalYaw += mouseX * rotationSpeed * deltaTime;
		}
		if (mouseY != 0)
		{
			m_TotalPitch -= mouseY * rotationSpeed * deltaTime;
		}
	}
	//forward and backward translation with mouse
	else if (mouseState & SDL_BUTTON_LMASK)
	{
		if (mouseY < 0)
		{
			MoveForward(deltaTime, moveSpeed);
		}
		else if (mouseY > 0)
		{
			MoveBackward(deltaTime, moveSpeed);
		}
	}
}

void Camera::MoveForward(float deltaTime, float moveSpeed) { m_Origin.z += deltaTime * moveSpeed; }
void Camera::MoveBackward(float deltaTime, float moveSpeed) { m_Origin.z -= deltaTime * moveSpeed; }
void Camera::MoveUp(float deltaTime, float moveSpeed) { m_Origin.y += deltaTime * moveSpeed; }
void Camera::MoveDown(float deltaTime, float moveSpeed) { m_Origin.y -= deltaTime * moveSpeed; }