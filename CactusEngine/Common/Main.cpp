#include "GraphicsApplication.h"
#include "Global.h"
#include "MemoryAllocator.h"
#include "RenderingSystem.h"
#include "InputSystem.h"
#include "AnimationSystem.h"
#include "ScriptSystem.h"
#include "ECSSceneWriter.h"
#include "ECSSceneReader.h"
#include "LogUtility.h"

#include "LightComponent.h"
#include "TransformComponent.h"
#include "ScriptComponent.h"
#include "ExternalMesh.h"
#include "StandardEntity.h"

#include "SampleScript/LightScript.h"

// This is the entry of the program

using namespace Engine;

void ConfigSetup();
void TestSetup(GraphicsApplication* pApp);

int32_t main()
{
	LOG_SYSTEM_INITIALIZE();

	if (gpGlobal == nullptr)
	{
		CE_NEW(gpGlobal, Global);
	}

	GraphicsApplication* pApplication;
	CE_NEW(pApplication, GraphicsApplication);

	if (!pApplication)
	{
		return -1;
	}

	pApplication->SetApplicationID(0);
	pApplication->AddSetupFunction(TestSetup);

	gpGlobal->SetApplication(pApplication);

	ConfigSetup();

	try
	{
		pApplication->Initialize();
	}
	catch (const std::runtime_error& e)
	{
		LOG_ERROR(e.what());
		system("pause");
		return -1;
	}

	while (!pApplication->ShouldQuit())
	{
		try
		{
			pApplication->Tick();

			// For memory leak debugging
			//LOG_MESSAGE("Active allocation count: " + std::to_string(gAllocationTrackerInstance.GetActiveAllocationCount()));
		}
		catch (const std::runtime_error& e)
		{
			LOG_ERROR(e.what());
			system("pause");
			return -1;
		}
	}

	pApplication->ShutDown();

	CE_DELETE(pApplication);

	CE_DELETE(gpGlobal);
	LOG_SYSTEM_SHUTDOWN();

	return 0;
}

void ConfigSetup()
{
	gpGlobal->CreateConfiguration<AppConfiguration>(EConfigurationType::App);
	gpGlobal->CreateConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics);

	// TODO: read settings from file
	gpGlobal->GetConfiguration<AppConfiguration>(EConfigurationType::App)->SetAppName("Cactus Engine");
	gpGlobal->GetConfiguration<AppConfiguration>(EConfigurationType::App)->SetAppVersion("0.0.1");

	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetGraphicsAPIType(EGraphicsAPIType::Vulkan);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetPreferredGPUType(EGPUType::Discrete);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetWindowSize(1920, 1080);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetMaxFramesInFlight(3);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetVSync(false);
}

void TestSetup(GraphicsApplication* pApp)
{
	auto pWorld = pApp->GetECSWorld();

	pWorld->RegisterSystem<RenderingSystem>(ESystemType::Script, 0);
	pWorld->RegisterSystem<AnimationSystem>(ESystemType::Animation, 0);
	pWorld->RegisterSystem<InputSystem>(ESystemType::Input, 2);
	pWorld->RegisterSystem<ScriptSystem>(ESystemType::Rendering, 1);
	pWorld->SortSystems();

	// Read scene from file
	ReadECSWorldFromJson(pWorld, "Assets/Scene/UnityChanScene.json");
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/LucyScene.json");
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/SerapisScene.json");

	// Or manually add contents here
	// ...

	// 121 point lights for deferred lighting test

	LightComponent::Profile lightProfile{};
	lightProfile.sourceType = LightComponent::SourceType::PointLight;
	lightProfile.lightColor = Color3(1.0f);
	lightProfile.lightIntensity = 10.0f;
	lightProfile.radius = 1.0f;
	CE_NEW(lightProfile.pVolumeMesh, ExternalMesh, "Assets/Models/light_sphere.obj");

	Color3 colors[7] = {
		Color3(1.0f),
		Color3(1.0f, 0.0f, 0.0f),
		Color3(0.0f, 1.0f, 0.0f),
		Color3(0.0f, 0.0f, 1.0f),
		Color3(1.0f, 0.0f, 1.0f),
		Color3(0.0f, 1.0f, 1.0f),
		Color3(1.0f, 1.0f, 0.0f)
	};
	uint32_t colorIndex = 0;

	for (int32_t i = -5; i < 6; i++)
	{
		for (int32_t j = -5; j < 6; j++)
		{
			auto pTransformComp = pWorld->CreateComponent<TransformComponent>();		
			auto pLightComp = pWorld->CreateComponent<LightComponent>();
			auto pScriptComp = pWorld->CreateComponent<ScriptComponent>();

			auto pLight = pWorld->CreateEntity<StandardEntity>();
			pLight->AttachComponent(pTransformComp);
			pLight->AttachComponent(pLightComp);
			pLight->AttachComponent(pScriptComp);

			pTransformComp->SetPosition(Vector3(4.0f * (i / 5.0f), 0.1f, 8.0f * (j / 5.0f)));
			lightProfile.lightColor = colors[(colorIndex++) % 7];
			pLightComp->UpdateProfile(lightProfile);

			SampleScript::LightScript* pSript;
			CE_NEW(pSript, SampleScript::LightScript, pLight);
			pScriptComp->BindScript(pSript);
		}
	}

	// Write scene to file
	//WriteECSWorldToJson(pWorld, "Assets/Scene/NewScene.json");
}