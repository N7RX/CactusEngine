#pragma once
#include "SharedTypes.h"
#include "EntityProperties.h"

#include <unordered_map>
#include <memory>

namespace Engine
{
	class BaseComponent;

	typedef std::unordered_map<EComponentType, std::shared_ptr<BaseComponent>> ComponentList;

	class BaseEntity : std::enable_shared_from_this<BaseEntity>
	{
	public:
		BaseEntity();
		virtual ~BaseEntity() = default;

		void SetEntityID(uint32_t id);
		uint32_t GetEntityID() const;

		void AttachComponent(const std::shared_ptr<BaseComponent> pComponent);
		void DetachComponent(EComponentType compType);

		const ComponentList& GetComponentList() const;
		std::shared_ptr<BaseComponent> GetComponent(EComponentType compType) const;

		template<typename T>
		inline std::shared_ptr<T> GetComponent(EComponentType compType) const
		{
			auto comp = GetComponent(compType);
			if (comp != nullptr)
			{
				return std::static_pointer_cast<T>(comp);
			}
			return nullptr;
		}

		EEntityTag GetEntityTag() const;
		void SetEntityTag(EEntityTag tag);

	protected:
		uint32_t m_entityID;
		EEntityTag m_tag;

		ComponentList m_componentList;
		uint32_t m_componentBitmap; // This is able to support up to 32 components, which should be enough for now
	};
}