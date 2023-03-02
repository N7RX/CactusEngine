#pragma once

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
		void TrackDeallocation(size_t size);

		size_t GetAllocatedSize() const { return m_allocatedSize; }

	private:
		size_t m_allocatedSize;
	};

	extern MemoryAllocationTracker gAllocationTrackerInstance;

#define CE_NEW(outPtr, type, ...) \
	gAllocationTrackerInstance.TrackNewAllocation(sizeof(type)); \
	outPtr = new type(__VA_ARGS__);

#define CE_NEW_ARRAY(outPtr, type, length) \
	gAllocationTrackerInstance.TrackNewAllocation(sizeof(type) * length); \
	outPtr = new type[length];

#define CE_DELETE(ptr) \
	gAllocationTrackerInstance.TrackDeallocation(sizeof(*ptr)); \
	delete ptr;

#define CE_DELETE_ARRAY(ptr) \
	gAllocationTrackerInstance.TrackDeallocation(sizeof(ptr)); \
	delete[] ptr;
}