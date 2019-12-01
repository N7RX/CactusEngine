#pragma once
#include "BaseComponent.h"
#include "SharedTypes.h"

namespace Engine
{
	class CameraComponent : public BaseComponent
	{
	public:
		CameraComponent();
		~CameraComponent() = default;

		float GetFOV() const;
		float GetNearClip() const;
		float GetFarClip() const;
		ECameraProjectionType GetProjectionType() const;
		Color4 GetClearColor() const;

		void SetFOV(float fov);
		void SetClipDistance(float near, float far);
		void SetProjectionType(ECameraProjectionType type);
		void SetClearColor(Color4 color);

	private:
		float m_fov;
		float m_farClip;
		float m_nearClip;
		ECameraProjectionType m_projectionType;
		Color4 m_clearColor;

		// For depth-of-field
		float m_aperture;
		float m_focalDistance;
		float m_imageDistance;
	};
}