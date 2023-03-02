#pragma once
#include "BaseComponent.h"

namespace Engine
{
	class Mesh;

	class LightComponent : public BaseComponent
	{
	public:
		enum class SourceType
		{
			Directional = 0,
			PointLight,
			COUNT
		};
		
		struct Profile
		{
			SourceType				sourceType;

			Color3					lightColor;
			float					lightIntensity;
			Mesh*	pVolumeMesh;

			Vector3					direction;
			float					radius;
		};

		LightComponent();
		LightComponent(const Profile& profile);
		~LightComponent() = default;

		void UpdateProfile(const Profile& profile);
		const Profile& GetProfile() const;

	private:
		Profile m_profile;
	};
}