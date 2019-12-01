#include "CameraComponent.h"
#include "Global.h"
#include <cmath>

using namespace Engine;

CameraComponent::CameraComponent()
	: BaseComponent(eCompType_Camera), m_fov(45.0f), m_nearClip(0.1f), m_farClip(1000.0f), m_projectionType(eProjectionType_Perspective), m_clearColor(Color4(0, 0, 0, 1))
{
}

float CameraComponent::GetFOV() const
{
	return m_fov;
}

float CameraComponent::GetNearClip() const
{
	return m_nearClip;
}

float CameraComponent::GetFarClip() const
{
	return m_farClip;
}

ECameraProjectionType CameraComponent::GetProjectionType() const
{
	return m_projectionType;
}

Color4 CameraComponent::GetClearColor() const
{
	return m_clearColor;
}

void CameraComponent::SetFOV(float fov)
{
	m_fov = abs(fov);
}

void CameraComponent::SetClipDistance(float near, float far)
{
	m_nearClip = abs(near);
	m_farClip = abs(far);
}

void CameraComponent::SetProjectionType(ECameraProjectionType type)
{
	m_projectionType = type;
}

void CameraComponent::SetClearColor(Color4 color)
{
	m_clearColor = color;
}