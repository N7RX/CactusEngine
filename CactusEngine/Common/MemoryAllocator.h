#pragma once
#include <atomic>

namespace Engine
{
	class MemoryAllocationTracker
	{
	public:
		MemoryAllocationTracker();
		~MemoryAllocationTracker();

		void Initialize();
		void ShutDown();

		void TrackNewAllocation(size_t size);
		void TrackDeallocation(); // Because derived type size cannot be determined from base type pointer directly without wrapper,
								  // deallocated size is not tracked

		// Size of all allocations so far, including deleted ones
		inline size_t GetTotalAllocatedSize() const { return m_totalAllocatedSize; }
		// Number of allocations that has not been deleted. Used for memory leak checking
		inline size_t GetActiveAllocationCount() const { return m_activeAllocationCount; }

		// TODO: provide ability to track active allocation size

	private:
		std::atomic<size_t> m_totalAllocatedSize;
		std::atomic<size_t> m_activeAllocationCount;
	};

	extern MemoryAllocationTracker gAllocationTrackerInstance;

#define CE_NEW(outPtr, type, ...) \
	gAllocationTrackerInstance.TrackNewAllocation(sizeof(type)); \
	outPtr = new type(__VA_ARGS__);

#define CE_NEW_ARRAY(outPtr, type, length) \
	gAllocationTrackerInstance.TrackNewAllocation(sizeof(type) * length); \
	outPtr = new type[length];

#define CE_DELETE(ptr) \
	gAllocationTrackerInstance.TrackDeallocation(); \
	delete ptr; \
	ptr = nullptr;

#define CE_SAFE_DELETE(ptr) \
	if (ptr) \
	{ \
		gAllocationTrackerInstance.TrackDeallocation(); \
		delete ptr; \
		ptr = nullptr; \
	}

#define CE_DELETE_ARRAY(ptr) \
	gAllocationTrackerInstance.TrackDeallocation(); \
	delete[] ptr; \
	ptr = nullptr;
}