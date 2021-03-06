#include "LightComponent.h"
#include "TransformComponent.h"
#include "BaseEntity.h"

using namespace Engine;

LightComponent::LightComponent()
	: BaseComponent(EComponentType::Light)
{

}

LightComponent::LightComponent(const Profile& profile)
	: BaseComponent(EComponentType::Light), m_profile(profile)
{
	auto pTransformComp = std::static_pointer_cast<TransformComponent>(m_pParentEntity->GetComponent(EComponentType::Transform));
	assert(pTransformComp != nullptr);
	pTransformComp->SetScale(Vector3(m_profile.radius));
}

void LightComponent::UpdateProfile(const Profile& profile)
{
	m_profile = profile;

	auto pTransformComp = std::static_pointer_cast<TransformComponent>(m_pParentEntity->GetComponent(EComponentType::Transform));
	assert(pTransformComp != nullptr);
	pTransformComp->SetScale(Vector3(m_profile.radius / 19.5f));
}

const LightComponent::Profile& LightComponent::GetProfile() const
{
	return m_profile;
}