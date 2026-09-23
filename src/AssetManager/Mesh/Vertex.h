#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include "AssetManager/Texture/Texture.h"
#include "utils.h"


#include <array>
#include <glm/glm.hpp>

struct MeshVertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 UV;

	// Binding and attribute descriptions for Vulkan
	static vk::VertexInputBindingDescription GetBindingDescription() {
		return { 0, sizeof(MeshVertex), vk::VertexInputRate::eVertex };
	}

	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
		return {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, Position)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, Normal)),
			vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(MeshVertex, UV))
		};
	}

	// Equality operator and hash function for vertex deduplication
	bool operator==(const MeshVertex& other) const {
		return Position == other.Position && Normal == other.Normal && UV == other.UV;
	}
};

