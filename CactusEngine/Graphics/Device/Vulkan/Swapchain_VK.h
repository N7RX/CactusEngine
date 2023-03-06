#pragma once
#include "UploadAllocator_VK.h"
#include "SyncObjectManager_VK.h"
#include "ExtensionLoader_VK.h"

namespace Engine
{
	class  RenderTarget2D_VK;

	struct SwapchainCreateInfo_VK
	{
		VkSurfaceKHR				surface;
		VkSurfaceFormatKHR			surfaceFormat;
		VkPresentModeKHR			presentMode;
		VkExtent2D					swapExtent;
		SwapchainSupportDetails_VK	supportDetails;
		QueueFamilyIndices_VK		queueFamilyIndices;
		uint32_t					maxFramesInFlight;
	};

	class Swapchain_VK
	{
	public:
		Swapchain_VK(LogicalDevice_VK* pDevice, const SwapchainCreateInfo_VK& createInfo);
		~Swapchain_VK();

		bool UpdateBackBuffer(uint32_t currentFrame);
		bool Present(const std::vector<Semaphore_VK*>& waitSemaphores);

		uint32_t GetSwapchainImageCount() const;
		RenderTarget2D_VK* GetTargetImage() const;
		uint32_t GetTargetImageIndex() const;
		RenderTarget2D_VK* GetSwapchainImageByIndex(uint32_t index) const;
		VkExtent2D GetSwapExtent() const;
		Semaphore_VK* GetImageAvailableSemaphore(uint32_t currentFrame) const;

	public:
		const uint64_t ACQUIRE_IMAGE_TIMEOUT = 3e9; // 3 seconds

	private:
		LogicalDevice_VK* m_pDevice;
		VkSwapchainKHR m_swapchain;
		VkQueue m_presentQueue;
		VkExtent2D m_swapExtent;

		std::vector<RenderTarget2D_VK*> m_renderTargets; // Swapchain images
		uint32_t m_targetImageIndex;
		std::vector<Semaphore_VK*> m_imageAvailableSemaphores;

		friend class GraphicsHardwareInterface_VK;
	};
}