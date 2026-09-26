#pragma once

#include "Mesh/Mesh.h"

#include <filesystem>


namespace AssetManager
{
    void LoadModel(const std::filesystem::path& path, Mesh& outMesh );
}