#pragma once
#include "BaseComponent.h"

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
		float GetAperture() const;
		float GetFocalDistance() const;
		float GetImageDistance() const;

		void SetFOV(float fov);
		void SetClipDistance(float near, float far);
		void SetProjectionType(ECameraProjectionType type);
		void SetClearColor(Color4 color);
		void SetAperture(float val);
		void SetFocalDistance(float val);
		void SetImageDistance(float val);

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