#include "LightComponent.h"
#include "TransformComponent.h"
#include "BaseEntity.h"
#include "LogUtility.h"

namespace Engine
{
	LightComponent::LightComponent()
		: BaseComponent(EComponentType::Light),
		m_profile()
	{

	}

	LightComponent::LightComponent(const Profile& profile)
		: BaseComponent(EComponentType::Light),
		m_profile(profile)
	{
		auto pTransformComp = (TransformComponent*)m_pParentEntity->GetComponent(EComponentType::Transform);
		DEBUG_ASSERT_CE(pTransformComp != nullptr);
		pTransformComp->SetScale(Vector3(m_profile.radius));
	}

	void LightComponent::UpdateProfile(const Profile& profile)
	{
		m_profile = profile;

		auto pTransformComp = (TransformComponent*)m_pParentEntity->GetComponent(EComponentType::Transform);
		DEBUG_ASSERT_CE(pTransformComp != nullptr);
		pTransformComp->SetScale(Vector3(m_profile.radius / 19.5f));
	}

	const LightComponent::Profile& LightComponent::GetProfile() const
	{
		return m_profile;
	}
}