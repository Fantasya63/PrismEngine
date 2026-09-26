#pragma once
#include <glm/glm.hpp>

struct Material
{
	glm::vec4 Albedo;
	glm::vec3 Emission;
	float Roughness;
	float Metallic;

	int baseColorTextureIndex = -1;
	int metallicRoughnessTextureIndex = -1;
	int normalTextureIndex = -1;
	int occlusionTextureIndex = -1;
	int emissiveTextureIndex = -1;
};