#include "Plane.h"
#include "Global.h"
#include "GraphicsApplication.h"

using namespace Engine;

Plane::Plane(uint32_t dimLength, uint32_t dimWidth)
	: Mesh(std::dynamic_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->GetDrawingDevice())
{
	std::vector<float> positions((dimLength + 1) * (dimWidth + 1) * 3, 0);
	std::vector<float> normals((dimLength + 1) * (dimWidth + 1) * 3, 0);
	std::vector<float> texcoords((dimLength + 1) * (dimWidth + 1) * 2, 0);
	std::vector<int> vertexIndices(dimLength * dimWidth * 6, 0);

	float gridLength = 1.0f / dimLength;
	float gridWidth = 1.0f / dimWidth;

	for (uint32_t i = 0; i < dimWidth + 1; ++i)
	{
		for (uint32_t j = 0; j < dimLength + 1; ++j)
		{
			positions[(i * (dimLength + 1) + j) * 3] = j * gridLength;
			positions[(i * (dimLength + 1) + j) * 3 + 1] = i * gridWidth;
			// z is 0

			normals[(i * (dimLength + 1) + j) * 3 + 2] = 1.0f; // Facing towards positive z

			texcoords[(i * (dimLength + 1) + j) * 2] = j * gridLength; // This works when plane size is exactly [1.0, 1.0]
			texcoords[(i * (dimLength + 1) + j) * 2 + 1] = i * gridWidth;
		}
	}

	size_t index = 0;
	for (uint32_t i = 0; i < dimWidth; ++i)
	{
		for (uint32_t j = 0; j < dimLength; ++j)
		{
			// For each quad
			// Triangle 1
			vertexIndices[index++] = i * (dimLength + 1) + j;
			vertexIndices[index++] = i * (dimLength + 1) + j + 1;
			vertexIndices[index++] = (i + 1) * (dimLength + 1) + j + 1;

			// Triangle 2
			vertexIndices[index++] = i * (dimLength + 1) + j;
			vertexIndices[index++] = (i + 1) * (dimLength + 1) + j + 1;
			vertexIndices[index++] = (i + 1) * (dimLength + 1) + j;
		}
	}

	CreateVertexBufferFromVertices(positions, normals, texcoords, vertexIndices);
}