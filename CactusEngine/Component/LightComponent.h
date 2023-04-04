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
			Point,
			Cone,
			Area,
			Ambient,
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

		void UpdateProfile(const Profile& profile); // Alert: must have been attached to a valid entity
		const Profile& GetProfile() const;

	private:
		Profile m_profile;
	};
}