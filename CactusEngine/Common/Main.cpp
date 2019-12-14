#include <iostream>
#include "GraphicsApplication.h"
#include "Global.h"
#include "DrawingSystem.h"
#include "InputSystem.h"
#include "AnimationSystem.h"
#include "ScriptSystem.h"
#include "ECSSceneWriter.h"
#include "ECSSceneReader.h"

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

	gpGlobal->GetConfiguration<AppConfiguration>(EConfigurationType::App)->SetAppName("Cactus Engine");
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetDeviceType(EGraphicsDeviceType::OpenGL);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetWindowSize(1280, 720);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetVSync(true);
}

void TestSetup(GraphicsApplication* pApp)
{
	auto pWorld = pApp->GetECSWorld();

	// Alert: the registration sequence may correspond to execution sequence
	pWorld->RegisterSystem<InputSystem>(ESystemType::Input);
	//pWorld->RegisterSystem<AnimationSystem>(ESystemType::Animation);
	pWorld->RegisterSystem<DrawingSystem>(ESystemType::Script);
	pWorld->RegisterSystem<ScriptSystem>(ESystemType::Drawing);

	// Read scene from file
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/UnityChanScene.json");
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/LucyScene.json");
	ReadECSWorldFromJson(pWorld, "Assets/Scene/SerapisScene.json");

	//WriteECSWorldToJson(pWorld, "Assets/Scene/SerapisScene.json");

	// Or manually add contents here
	// ...
}