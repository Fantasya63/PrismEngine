#pragma once

#include "Vertex.h"
#include "Index.h"

#include <vector>

struct Mesh
{
	std::vector<MeshVertex> Vertices;
	std::vector<meshIndex_t> Indices;
	int MaterialIndex = -1;
};