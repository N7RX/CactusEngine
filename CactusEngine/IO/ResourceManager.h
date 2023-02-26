#pragma once
#include <unordered_map>

#include "Mesh.h"
#include "ImageTexture.h"

namespace Engine
{
	namespace ResourceManagement
	{
		typedef std::unordered_map<std::string, std::shared_ptr<Texture2D>> FileTextureList; // TODO: store hashed strings instead of strings
		typedef std::unordered_map<std::string, std::shared_ptr<Mesh>> FileMeshList;

		// Record already external resources to prevent duplicate loading
		static FileTextureList LoadedImageTextures;
		static FileMeshList LoadedMeshes;
	}
}