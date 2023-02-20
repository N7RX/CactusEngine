#include <iostream>
#include "GraphicsApplication.h"
#include "Global.h"
#include "RenderingSystem.h"
#include "InputSystem.h"
#include "AnimationSystem.h"
#include "ScriptSystem.h"
#include "ECSSceneWriter.h"
#include "ECSSceneReader.h"

#include "LightComponent.h"
#include "TransformComponent.h"
#include "ExternalMesh.h"
#include "StandardEntity.h"
#include "LightScript.h"

// This is the entry of the program

using namespace Engine;

void ConfigSetup();
void TestSetup(GraphicsApplication* pApp);

int main()
{
	if (gpGlobal == nullptr)
	{
		gpGlobal = new Global();
	}

	auto pApplication = std::make_shared<GraphicsApplication>();

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
		std::cout << e.what() << std::endl;
		system("pause");
		return -1;
	}

	while (!pApplication->ShouldQuit())
	{
		try
		{
			pApplication->Tick();
		}
		catch (const std::runtime_error& e)
		{
			std::cout << e.what() << std::endl;
			system("pause");
			return -1;
		}
	}

	pApplication->ShutDown();

	return 0;
}

void ConfigSetup()
{
	gpGlobal->CreateConfiguration<AppConfiguration>(EConfigurationType::App);
	gpGlobal->CreateConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics);

	// TODO: read setitings from file
	gpGlobal->GetConfiguration<AppConfiguration>(EConfigurationType::App)->SetAppName("Cactus Engine");
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetDeviceType(EGraphicsAPIType::Vulkan);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetWindowSize(1920, 1080);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetVSync(false);
}

void TestSetup(GraphicsApplication* pApp)
{
	auto pWorld = pApp->GetECSWorld();

	// Alert: the registration sequence may correspond to execution sequence
	pWorld->RegisterSystem<InputSystem>(ESystemType::Input);
	pWorld->RegisterSystem<AnimationSystem>(ESystemType::Animation);
	pWorld->RegisterSystem<RenderingSystem>(ESystemType::Script);
	pWorld->RegisterSystem<ScriptSystem>(ESystemType::Drawing);

	// Read scene from file
	ReadECSWorldFromJson(pWorld, "Assets/Scene/UnityChanScene.json");
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/LucyScene.json");
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/SerapisScene.json");

	// Or manually add contents here
	// ...

	// 121 point lights for deferred lighting test

	LightComponent::Profile lightProfile = {};
	lightProfile.sourceType = LightComponent::SourceType::PointLight;
	lightProfile.lightColor = Color3(1.0f);
	lightProfile.lightIntensity = 10.0f;
	lightProfile.radius = 1.0f;
	lightProfile.pVolumeMesh = std::make_shared<ExternalMesh>("Assets/Models/light_sphere.obj");

	Color3 colors[7] = {
		Color3(1.0f),
		Color3(1.0f, 0.0f, 0.0f),
		Color3(0.0f, 1.0f, 0.0f),
		Color3(0.0f, 0.0f, 1.0f),
		Color3(1.0f, 0.0f, 1.0f),
		Color3(0.0f, 1.0f, 1.0f),
		Color3(1.0f, 1.0f, 0.0f)
	};
	int colorIndex = 0;

	for (int i = -5; i < 6; i++)
	{
		for (int j = -5; j < 6; j++)
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
			pScriptComp->BindScript(std::make_shared<SampleScript::LightScript>(pLight));
		}
	}

	// Write scene to file
	//WriteECSWorldToJson(pWorld, "Assets/Scene/NewScene.json");
}