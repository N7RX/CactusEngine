#pragma once
#include "ECSSceneReader.h"
#include "ResourceManager.h"
#include "ECSWorld.h"
#include "AllComponents.h"
#include "StandardEntity.h"
#include "ScriptSelector.h"
#include "ExternalMesh.h"
#include "Plane.h"

#include <json/json.h>
#include <stdio.h>
#include <fstream>
#include <sstream>

namespace Engine
{
	bool ReadECSWorldFromJson(ECSWorld* pWorld, const char* fileAddress)
	{
		if (!pWorld)
		{
			return false;
		}

		// Initialize readers

		std::ifstream fileReader;
		fileReader.open(fileAddress);

		if (fileReader.fail())
		{
			LOG_ERROR((std::string)"ECSSceneReader: Cannot open " + fileAddress);
			return false;
		}

		Json::CharReaderBuilder builder;
		Json::Value root;

		JSONCPP_STRING errs;
		if (!parseFromStream(builder, fileReader, &root, &errs))
		{
			LOG_ERROR(errs);
			return false;
		}

#if defined(DEVELOPMENT_MODE_CE)
		LOG_MESSAGE(std::string("Loading scene: ") + fileAddress);
#endif

		// Resolve the json structure and construct ECS world

		uint32_t entityCount = root["entityCount"].asInt();

		for (uint32_t i = 0; i < entityCount; ++i)
		{
#if defined(DEVELOPMENT_MODE_CE)
			LOG_MESSAGE("Loading finished " + std::to_string(i * 100.0f / entityCount) + "%");
#endif

			std::stringstream entityName;
			entityName << "entity" << i;
			Json::Value entity = root[entityName.str()];

			// Check and configure components

			std::stack<BaseComponent*> components;
			// TODO: find a better solution
			int32_t scriptID = -1;
			ScriptComponent* pScriptComp = nullptr;

			if (entity["transform"])
			{
				Json::Value component = entity["transform"];

				Vector3 position(0);
				position.x = component["position"]["x"].asFloat();
				position.y = component["position"]["y"].asFloat();
				position.z = component["position"]["z"].asFloat();

				Vector3 rotation(0);
				rotation.x = component["rotation"]["x"].asFloat();
				rotation.y = component["rotation"]["y"].asFloat();
				rotation.z = component["rotation"]["z"].asFloat();

				Vector3 scale(0);
				scale.x = component["scale"]["x"].asFloat();
				scale.y = component["scale"]["y"].asFloat();
				scale.z = component["scale"]["z"].asFloat();

				auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
				pTransformComp->SetPosition(position);
				pTransformComp->SetRotation(rotation);
				pTransformComp->SetScale(scale);

				components.push(pTransformComp);
			}
			if (entity["meshFilter"])
			{
				Json::Value component = entity["meshFilter"];

				bool attachMesh = true;
				Mesh* pMesh = nullptr;
				switch ((EBuiltInMeshType)(component["type"].asInt()))
				{
				case EBuiltInMeshType::External:
					if (ResourceManagement::LoadedMeshes.find(component["filePath"].asString()) == ResourceManagement::LoadedMeshes.end())
					{
						CE_NEW(pMesh, ExternalMesh, component["filePath"].asCString());
						ResourceManagement::LoadedMeshes.emplace(component["filePath"].asString(), pMesh);
					}
					else
					{
						pMesh = ResourceManagement::LoadedMeshes.at(component["filePath"].asString());
					}
					break;
				case EBuiltInMeshType::Plane:
					CE_NEW(pMesh, Plane, component["planeDimension"]["x"].asUInt(), component["planeDimension"]["y"].asUInt());
					break;
				default:
					LOG_WARNING("ECSSceneReader: Unhandled mesh type: " + std::to_string(component["type"].asInt()));
					attachMesh = false;
					break;
				}

				if (attachMesh && pMesh != nullptr)
				{
					auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
					pMeshFilterComp->SetMesh(pMesh);

					components.push(pMeshFilterComp);
				}
			}
			if (entity["material"])
			{
				Json::Value component = entity["material"];

				MaterialComponent* pMaterialComp;
				CE_NEW(pMaterialComp, MaterialComponent);

				static std::string pathTypes[(uint32_t)EMaterialTextureType::COUNT] =
				{
					"albedoTexturePath",
					"normalTexturePath",
					"roughnessTexturePath",
					"aoTexturePath",
					"noiseTexturePath",
					"toneTexturePath"
				};

				uint32_t materialCount = component["materialCount"].asUInt();

				for (uint32_t i = 0; i < materialCount; ++i)
				{
					std::stringstream materialName;
					materialName << "materialImpl" << i;
					Json::Value subComponent = component[materialName.str()];

					Material* pMaterial;
					CE_NEW(pMaterial, Material);

					pMaterial->SetShaderProgram((EBuiltInShaderProgramType)(subComponent["shaderType"].asInt()));

					for (uint32_t i = 0; i < (uint32_t)EMaterialTextureType::COUNT; ++i)
					{
						if (subComponent[pathTypes[i]])
						{
							Texture2D* pTexture = nullptr;
							if (ResourceManagement::LoadedImageTextures.find(subComponent[pathTypes[i]].asString()) == ResourceManagement::LoadedImageTextures.end())
							{
								CE_NEW(pTexture, ImageTexture, subComponent[pathTypes[i]].asCString());
								ResourceManagement::LoadedImageTextures.emplace(subComponent[pathTypes[i]].asString(), pTexture);
							}
							else
							{
								pTexture = ResourceManagement::LoadedImageTextures.at(subComponent[pathTypes[i]].asString());
							}
							pMaterial->SetTexture((EMaterialTextureType)((uint32_t)EMaterialTextureType::Albedo + i), pTexture); // Alert: here the check sequence must be consistent with enum sequence
						}
					}

					Color4 albedoColor(1);
					albedoColor.x = subComponent["albedoColor"]["r"].asFloat();
					albedoColor.y = subComponent["albedoColor"]["g"].asFloat();
					albedoColor.z = subComponent["albedoColor"]["b"].asFloat();
					albedoColor.w = subComponent["albedoColor"]["a"].asFloat();

					pMaterial->SetAlbedoColor(albedoColor);

					pMaterial->SetTransparent(subComponent["transparent"].asBool());

					pMaterial->SetAnisotropy(subComponent["anisotropy"].asFloat());
					pMaterial->SetRoughness(subComponent["roughness"].asFloat());

					pMaterialComp->AddMaterial(i, pMaterial);
				}

				components.push(pMaterialComp);
			}
			if (entity["camera"])
			{
				Json::Value component = entity["camera"];

				auto pCameraComp = pWorld->CreateComponent<CameraComponent>();

				pCameraComp->SetFOV(component["fov"].asFloat());
				pCameraComp->SetClipDistance(component["nearClip"].asFloat(), component["farClip"].asFloat());
				pCameraComp->SetProjectionType((ECameraProjectionType)(component["projection"].asInt()));
				pCameraComp->SetAperture(component["aperture"].asFloat());
				pCameraComp->SetFocalDistance(component["focalDistance"].asFloat());
				pCameraComp->SetImageDistance(component["imageDistance"].asFloat());

				Color4 clearColor(0);
				clearColor.x = component["clearColor"]["r"].asFloat();
				clearColor.y = component["clearColor"]["g"].asFloat();
				clearColor.z = component["clearColor"]["b"].asFloat();
				clearColor.w = component["clearColor"]["a"].asFloat();

				pCameraComp->SetClearColor(clearColor);

				components.push(pCameraComp);
			}
			if (entity["animation"])
			{
				// Anmiation component is unhandled at this moment
			}
			if (entity["script"])
			{
				Json::Value component = entity["script"];

				pScriptComp = pWorld->CreateComponent<ScriptComponent>();

				scriptID = component["scriptID"].asInt();

				components.push(pScriptComp);
			}

			// Instantiate entity and attach components

			auto pEntity = pWorld->CreateEntity<StandardEntity>();
			
			if (scriptID >= 0)
			{
				pScriptComp->BindScript(SampleScript::GenerateScriptByID((SampleScript::EScriptID)(scriptID), pEntity));
			}

			while (!components.empty())
			{
				pEntity->AttachComponent(components.top());
				components.pop();
			}

			pEntity->SetEntityTag((EEntityTag)(entity["tag"].asInt()));
		}

#if defined(DEVELOPMENT_MODE_CE)
		LOG_MESSAGE("Loading completed.");
#endif

		return true;
	}
}