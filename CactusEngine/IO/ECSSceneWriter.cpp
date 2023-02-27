#pragma once
#include <json/json.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "ECSSceneWriter.h"
#include "AllComponents.h"

namespace Engine
{
	bool WriteECSWorldToJson(const std::shared_ptr<ECSWorld> pWorld, const char* fileAddress)
	{
		if (!pWorld)
		{
			return false;
		}

		// Initialize writers

		Json::StreamWriterBuilder builder;
		const std::unique_ptr<Json::StreamWriter> jsonWriter(builder.newStreamWriter());

		std::ofstream fileWriter;
		fileWriter.open(fileAddress);

		if (fileWriter.fail())
		{
			LOG_ERROR((std::string)"ECSSceneWriter: Cannot access " + fileAddress);
			return false;
		}

		// Construct json hierarchy struture

		Json::Value root;

		auto pEntityList = pWorld->GetEntityList();
		unsigned int entityIndex = 0;
		for (auto itr = pEntityList->begin(); itr != pEntityList->end(); ++itr)
		{
			Json::Value entity;
			entity["tag"] = (uint32_t)itr->second->GetEntityTag();
			
			// Write components of each entity

			auto componentList = itr->second->GetComponentList();
			for (auto componentEntry : componentList)
			{
				switch (componentEntry.first)
				{
				case EComponentType::Transform:
				{
					auto pTransformComp = std::static_pointer_cast<TransformComponent>(componentEntry.second);
					Json::Value component;

					Vector3 position = pTransformComp->GetPosition();
					component["position"]["x"] = position.x;
					component["position"]["y"] = position.y;
					component["position"]["z"] = position.z;

					Vector3 rotation = pTransformComp->GetRotation();
					component["rotation"]["x"] = rotation.x;
					component["rotation"]["y"] = rotation.y;
					component["rotation"]["z"] = rotation.z;

					Vector3 scale = pTransformComp->GetScale();
					component["scale"]["x"] = scale.x;
					component["scale"]["y"] = scale.y;
					component["scale"]["z"] = scale.z;

					entity["transform"] = component;
					break;
				}
				case EComponentType::MeshFilter:
				{
					auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(componentEntry.second);
					Json::Value component;

					auto pMesh = pMeshFilterComp->GetMesh();

					component["type"] = (uint32_t)pMesh->GetMeshType();

					switch (pMesh->GetMeshType())
					{
					case EBuiltInMeshType::External:
						component["filePath"] = pMesh->GetFilePath();
						break;
					case EBuiltInMeshType::Plane:
						auto planeDimension = pMesh->GetPlaneDimenstion();
						component["planeDimension"]["x"] = planeDimension.x;
						component["planeDimension"]["y"] = planeDimension.y;
						break;
					default:
						LOG_WARNING("ECSSceneWriter: Unhandled mesh type: " + std::to_string((uint32_t)pMesh->GetMeshType()));
						break;
					}

					entity["meshFilter"] = component;
					break;
				}
				case EComponentType::Material:
				{
					auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(componentEntry.second);
					Json::Value component;

					auto materialList = pMaterialComp->GetMaterialList();

					component["materialCount"] = materialList.size();

					for (auto& material : materialList)
					{
						Json::Value subComponent;
						subComponent["shaderType"] = (uint32_t)material.second->GetShaderProgramType();

						auto textureList = material.second->GetTextureList();
						for (auto& textureEntry : textureList)
						{
							switch (textureEntry.first)
							{
							case EMaterialTextureType::Albedo:
								subComponent["albedoTexturePath"] = textureEntry.second->GetFilePath();
								break;
							case EMaterialTextureType::Normal:
								subComponent["normalTexturePath"] = textureEntry.second->GetFilePath();
								break;
							case EMaterialTextureType::Roughness:
								subComponent["roughnessTexturePath"] = textureEntry.second->GetFilePath();
								break;
							case EMaterialTextureType::AO:
								subComponent["aoTexturePath"] = textureEntry.second->GetFilePath();
								break;
							case EMaterialTextureType::Noise:
								subComponent["noiseTexturePath"] = textureEntry.second->GetFilePath();
								break;
							case EMaterialTextureType::Tone:
								subComponent["toneTexturePath"] = textureEntry.second->GetFilePath();
								break;
							default:
								LOG_WARNING("ECSSceneWriter: Unhandled texture type: " + std::to_string((uint32_t)textureEntry.first));
								break;
							}
						}

						Color4 albedoColor = material.second->GetAlbedoColor();
						subComponent["albedoColor"]["r"] = albedoColor.x;
						subComponent["albedoColor"]["g"] = albedoColor.y;
						subComponent["albedoColor"]["b"] = albedoColor.z;
						subComponent["albedoColor"]["a"] = albedoColor.w;

						subComponent["transparent"] = material.second->IsTransparent();

						subComponent["anisotropy"] = material.second->GetAnisotropy();
						subComponent["roughness"] = material.second->GetRoughness();

						std::stringstream materialName;
						materialName << "materialImpl" << material.first;
						component[materialName.str()] = subComponent;
					}

					entity["material"] = component;
					break;
				}
				case EComponentType::Camera:
				{
					auto pCameraComp = std::static_pointer_cast<CameraComponent>(componentEntry.second);
					Json::Value component;

					component["fov"] = pCameraComp->GetFOV();
					component["nearClip"] = pCameraComp->GetNearClip();
					component["farClip"] = pCameraComp->GetFarClip();
					component["projection"] = (uint32_t)pCameraComp->GetProjectionType();
					component["aperture"] = pCameraComp->GetAperture();
					component["focalDistance"] = pCameraComp->GetFocalDistance();
					component["imageDistance"] = pCameraComp->GetImageDistance();

					Color4 clearColor = pCameraComp->GetClearColor();
					component["clearColor"]["r"] = clearColor.x;
					component["clearColor"]["g"] = clearColor.y;
					component["clearColor"]["b"] = clearColor.z;
					component["clearColor"]["a"] = clearColor.w;

					entity["camera"] = component;
					break;
				}
				case EComponentType::Animation:
				{
					// Anmiation component is unhandled at this moment
					break;
				}
				case EComponentType::Script:
				{
					auto pScriptComp = std::static_pointer_cast<ScriptComponent>(componentEntry.second);
					Json::Value component;

					component["scriptID"] = (uint32_t)pScriptComp->GetScript()->GetScriptID();
					
					entity["script"] = component;
					break;
				}
				default:
					LOG_WARNING("ECSSceneWriter: Unhandled component type: " + std::to_string((uint32_t)componentEntry.first));
					break;
				}
			}
			
			std::stringstream entityName;
			entityName << "entity" << entityIndex;
			root[entityName.str()] = entity;
			entityIndex++;
		}

		root["entityCount"] = entityIndex;

		// Write to json file

		jsonWriter->write(root, &fileWriter);
		fileWriter.close();

		return true;
	}
}