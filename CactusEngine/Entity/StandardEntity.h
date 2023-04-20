#pragma once
#include "BaseEntity.h"

namespace Engine
{
	class StandardEntity : public BaseEntity
	{
	public:
		StandardEntity();
		~StandardEntity() = default;

		void SetEntityName(const char* name);
		const char* GetEntityName() const;

		uint32_t EstimateMaxDrawCallCount() override;

	private:
		const char* m_entityName;

		uint32_t m_maxDrawCallCount;
	};
}