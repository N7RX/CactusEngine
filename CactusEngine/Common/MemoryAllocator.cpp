#include "MemoryAllocator.h"
#include "LogUtility.h"

namespace Engine
{
	MemoryAllocationTracker gAllocationTrackerInstance;

	MemoryAllocationTracker::MemoryAllocationTracker()
		: m_totalAllocatedSize(0),
		m_activeAllocationCount(0)
	{

	}

	MemoryAllocationTracker::~MemoryAllocationTracker()
	{
#if defined(DEVELOPMENT_MODE_CE)
		LOG_MESSAGE("Total allocated size: " + std::to_string(m_totalAllocatedSize) + " bytes");
		LOG_MESSAGE("Unreleased allocation(s) count: " + std::to_string(m_activeAllocationCount));
#endif
	}

	void MemoryAllocationTracker::TrackNewAllocation(size_t size)
	{
#if defined(DEVELOPMENT_MODE_CE)
		m_totalAllocatedSize += size;
		++m_activeAllocationCount;
#endif
	}

	void MemoryAllocationTracker::TrackDeallocation()
	{
#if defined(DEVELOPMENT_MODE_CE)
		DEBUG_ASSERT_CE(m_activeAllocationCount > 0);
		--m_activeAllocationCount;
#endif
	}
}