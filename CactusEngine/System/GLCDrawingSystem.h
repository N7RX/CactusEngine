#pragma once
#include "ISystem.h"
#include "Global.h"
#include "ECSWorld.h"
#include "GLCComponent.h"
#include "RenderTexture.h"

namespace Engine
{
	// This is a dedicated system for GLC experiment task
	// The entire GLC module is subject to removal in the future
	class GLCDrawingSystem : public ISystem
	{
	public:
		GLCDrawingSystem(ECSWorld* pWorld);
		~GLCDrawingSystem() = default;

		void SetSystemID(uint32_t id);
		uint32_t GetSystemID() const;

		void Initialize();
		void ShutDown();

		void FrameBegin();
		void Tick();
		void FrameEnd();

		void ConfigureDrawDimension(uint32_t width, uint32_t height);
		void SetBackgroundColor(Color3 color);

	private:
		void BuildRenderTask();
		void ConfigureRenderEnvironment();
		void ExecuteRenderTask();

		void RayTraceDraw(const std::shared_ptr<GLCComponent> pGLC, const std::vector<std::shared_ptr<IEntity>>& drawList);
		Color4 RayIntersect(const std::shared_ptr<GLCRay> pRay, const std::vector<std::shared_ptr<IEntity>>& drawList);
		bool RayTriangleIntersect(const std::shared_ptr<GLCRay> pRay, const Vector3 v1, const Vector3 v2, Vector3 v3);

		void WriteLinearResultToTexture();
		void WriteLinearResultToPPM();

	public:
		static uint32_t m_sGLCOrigin_Hori;
		static uint32_t m_sGLCOrigin_Vert;
		static std::shared_ptr<RenderTexture> m_sRenderResult;
		static Color3 m_sBackgroundColor;

	private:
		uint32_t m_systemID;
		ECSWorld* m_pECSWorld;
		std::vector<std::shared_ptr<IEntity>> m_renderTaskList;

		uint32_t m_drawWidth;
		uint32_t m_drawHeight;

		std::vector<float> m_linearResult;
		const int LINEAR_RESULT_WIDTH = 4;
	};
}