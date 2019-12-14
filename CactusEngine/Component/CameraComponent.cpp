#include "CameraComponent.h"
#include "Global.h"
#include <cmath>

using namespace Engine;

CameraComponent::CameraComponent()
	: BaseComponent(EComponentType::Camera), m_fov(45.0f), m_nearClip(0.1f), m_farClip(1000.0f), m_projectionType(ECameraProjectionType::Perspective), m_clearColor(Color4(0, 0, 0, 1)),
	m_aperture(4.0f), m_focalDistance(2.0f), m_imageDistance(2.0f)
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

float CameraComponent::GetAperture() const
{
	return m_aperture;
}

float CameraComponent::GetFocalDistance() const
{
	return m_focalDistance;
}

float CameraComponent::GetImageDistance() const
{
	return m_imageDistance;
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

void CameraComponent::SetAperture(float val)
{
	m_aperture = val;
}

void CameraComponent::SetFocalDistance(float val)
{
	m_focalDistance = val;
}

void CameraComponent::SetImageDistance(float val)
{
	m_imageDistance = val;
}