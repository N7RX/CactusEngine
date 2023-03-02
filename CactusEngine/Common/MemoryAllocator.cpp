#include "MemoryAllocator.h"
#include "LogUtility.h"

namespace Engine
{
	MemoryAllocationTracker gAllocationTrackerInstance;

	MemoryAllocationTracker::MemoryAllocationTracker()
		: m_allocatedSize(0)
	{

	}

	MemoryAllocationTracker::~MemoryAllocationTracker()
	{
		//DEBUG_ASSERT_MESSAGE_CE(m_allocatedSize == 0, "Some of the allocated memory is not released upon closing.");
		LOG_MESSAGE("Unreleased allocation: " + std::to_string(m_allocatedSize) + " bytes");
	}

	void MemoryAllocationTracker::TrackNewAllocation(size_t size)
	{
		// TODO: this is not thread safe
		m_allocatedSize += size;
	}

	void MemoryAllocationTracker::TrackDeallocation(size_t size)
	{
		DEBUG_ASSERT_CE(m_allocatedSize >= size);
		m_allocatedSize -= size;
	}
}