//#include <volk/volk.h>

#include "AssetManager.h"
#include "Mesh/Mesh.h"

//#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
//#include <vulkan/vulkan_raii.hpp>
//#else
//import vulkan_hpp;
//#endif
//
//#include "Texture/Texture.h"
//#include "Utils.h"
//

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ktx.h>
#include <string_view>
#include <tiny_gltf_v3.h>
#include <vector>

namespace AssetManager
{
    // Helper to compute node transform matrix
    glm::mat4 GetNodeMatrix(const tg3_node& node)
    {
        // If explicit 4x4 matrix is provided
        if (node.has_matrix)
        {
            return glm::make_mat4(node.matrix);
        }

        return glm::mat4(1.0f);
    }


    void LoadModel(const std::filesystem::path& path, Mesh& outMesh)
    {
        tg3_parse_options opts;
        tg3_error_stack errors;
        tg3_model model;

        tg3_parse_options_init(&opts);
        tg3_error_stack_init(&errors);

        tg3_error_code err = tg3_parse_file(&model, &errors, path.string().c_str(), path.string().length(), &opts);
        if (err != TG3_OK) {
            for (uint32_t i = 0; i < errors.count; i++) {
                fprintf(stderr, "[%d] %s\n", (int)errors.entries[i].severity,
                        errors.entries[i].message ? errors.entries[i].message : "(null)");
            }

            exit(EXIT_FAILURE);
        }

        // ... use model ...
        
        // Take the first mesh only, todo: Improve this to take in multiple meshes from the file
        if (model.meshes_count == 0 || model.meshes[0].primitives_count == 0)
        {
            tg3_model_free(&model);
            tg3_error_stack_free(&errors);
            return;
        }

        
        //// Texture
        //std::vector<Texture> textures;
        //for (size_t i = 0; i < model.textures_count; i++)
        //{
        //    const auto& texture = model.textures[i];
        //    const auto& image = model.images[texture.source];

        //    Texture tex;
        //    tex.name = image.name.empty() ? "texture_" + std::to_string(i) : image.name;


        //    if (image.mime_type == "image/ktx2" && image.buffer_view >= 0)
        //    {
        //        const auto& bufferView = model.buffer_views[image.buffer_view];
        //        const auto& buffer = model.buffers[bufferView.buffer];

        //        // Extract KTX2 Data From the buffer
        //        const uint8_t* ktx2Data = buffer.data.data() + bufferView.byte_offset;
        //        size_t ktx2Size = bufferView.byte_length;

        //        ktxTexture2* ktxTexture = nullptr;
        //        KTX_error_code errorCode = ktxTexture2_CreateFromMemory(
        //            ktx2Data, ktx2Size,
        //            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        //            &ktxTexture
        //        );

        //        if (errorCode != KTX_SUCCESS)
        //        {
        //            std::cerr << "Failed to load KTX2 Texture " << ktxErrorString(result) << std::endl;
        //            continue;
        //        }

        //        // If the texture uses Basis Universal compression, transcode it to a GPU-friendly format
        //        if (ktxTexture->isCompressed && ktxTexture2_NeedsTranscoding(ktxTexture)) 
        //        {
        //            // Choose the appropriate format based on GPU capabilities
        //            ktx_transcode_fmt_e transcodeFormat = KTX_TTF_BC7_RGBA;

        //            // For devices that don't support BC7, use alternatives
        //            /*if (!deviceSupportsBC7) {
        //                transcodeFmt = KTX_TTF_ASTC_4x4_RGBA;
        //            }
        //            if (!deviceSupportsASTC) {
        //                transcodeFmt = KTX_TTF_ETC2_RGBA;
        //            }*/

        //            // Transcode the texture
        //            result = ktxTexture2_TranscodeBasis(ktxTexture, transcodeFormat, 0);
        //            if (result != KTX_SUCCESS)
        //            {
        //                std::cerr << "Failed to transcode KTX2 texture: " << ktxErrorString(result) << std::endl;
        //                ktxTexture2_Destroy(ktxTexture);
        //                continue;
        //            }

        //            VkFormat format = static_cast<VkFormat>(ktxTexture2_GetVkFormat(ktxTexture));
        //            vk::Extent3D extent{
        //                static_cast<uint32_t>(ktxTexture->baseWidth),
        //                static_cast<uint32_t>(ktxTexture->baseHeight),
        //                static_cast<uint32_t>(ktxTexture->baseDepth)
        //            };
        //            uint32_t mipLevels = ktxTexture->numLevels;

        //            VkImageCreateInfo imageCreateInfo{
        //                .imageType = vk::ImageType::e2D,
        //                .format = format,
        //                .extent = extent,
        //                .mipLevels = mipLevels,
        //                .arrayLayers = 1,
        //                .samples = vk::SampleCountFlagBits::e1,
        //                .tiling = vk::ImageTiling::eOptimal,
        //                .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
        //                .sharingMode = vk::SharingMode::eExclusive,
        //                .initialLayout = vk::ImageLayout::eUndefined
        //            };
        //        }
        //    }
        //}
       
            
        const tg3_mesh& gltfMesh = model.meshes[0];
        // Take also the only primitive. Refactor this when we have proper node system
        const tg3_primitive& primitive = gltfMesh.primitives[0];

        int posAccessorIndex = -1;
        int normAccessorIndex = -1;
        int uvAccessorIndex = -1;
        
        for (size_t a = 0; a < primitive.attributes_count; a++)
        {
            const tg3_str_int_pair& attr = primitive.attributes[a];
            const std::string_view attrKey{ attr.key.data, attr.key.len };

            if (attr.key.data && attrKey == "POSITION")
            {
                posAccessorIndex = attr.value;
            }
            else if (attr.key.data && attrKey == "NORMAL")
            {
                normAccessorIndex = attr.value;
            }
            else if (attr.key.data && attrKey == "TEXCOORD_0")
            {
                uvAccessorIndex = attr.value;
            }

        }
        // Parse Position
        if (posAccessorIndex >= 0)
        {
            const tg3_accessor& accessor = model.accessors[posAccessorIndex];
            const tg3_buffer_view& bufferView = model.buffer_views[accessor.buffer_view];
            const tg3_buffer& buffer = model.buffers[bufferView.buffer];

            const uint8_t* pData = static_cast<const uint8_t*>(buffer.data.data) + bufferView.byte_offset + accessor.byte_offset;

            size_t stride = bufferView.byte_stride ? bufferView.byte_stride : sizeof(glm::vec3);

            outMesh.Vertices.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; i++)
            {
                // FIll with vertex position
                const float* pos = reinterpret_cast<const float*>(pData + i * stride);
                outMesh.Vertices[i].Position = glm::make_vec3(pos);
            }
        }

        // 3. Parse Vertex Normals
        if (normAccessorIndex >= 0)
        {
            const tg3_accessor& accessor = model.accessors[normAccessorIndex];
            const tg3_buffer_view& bufferView = model.buffer_views[accessor.buffer_view];
            const tg3_buffer& buffer = model.buffers[bufferView.buffer];

            const uint8_t* dataPtr = static_cast<const uint8_t*>(buffer.data.data)
                + bufferView.byte_offset
                + accessor.byte_offset;

            size_t stride = bufferView.byte_stride ? bufferView.byte_stride : sizeof(glm::vec3);

            for (size_t i = 0; i < accessor.count && i < outMesh.Vertices.size(); i++)
            {
                const float* norm = reinterpret_cast<const float*>(dataPtr + i * stride);
                outMesh.Vertices[i].Normal = glm::make_vec3(norm);
            }
        }

        // 4. Parse UV Coordinates
        if (uvAccessorIndex >= 0)
        {
            const tg3_accessor& accessor = model.accessors[uvAccessorIndex];
            const tg3_buffer_view& bufferView = model.buffer_views[accessor.buffer_view];
            const tg3_buffer& buffer = model.buffers[bufferView.buffer];

            const uint8_t* dataPtr = static_cast<const uint8_t*>(buffer.data.data)
                + bufferView.byte_offset
                + accessor.byte_offset;

            size_t stride = bufferView.byte_stride ? bufferView.byte_stride : sizeof(glm::vec2);

            for (size_t i = 0; i < accessor.count && i < outMesh.Vertices.size(); i++)
            {
                const float* uv = reinterpret_cast<const float*>(dataPtr + i * stride);
                outMesh.Vertices[i].UV = glm::make_vec2(uv);
            }
        }

        // Parse Indices
        if (primitive.indices >= 0)
        {
            const tg3_accessor& accessor = model.accessors[primitive.indices];
            const tg3_buffer_view& bufferView = model.buffer_views[accessor.buffer_view];
            const tg3_buffer& buffer = model.buffers[bufferView.buffer];

            const uint8_t* dataPtr = static_cast<const uint8_t*>(buffer.data.data)
                + bufferView.byte_offset
                + accessor.byte_offset;

            outMesh.Indices.resize(accessor.count);

            for (size_t i = 0; i < accessor.count; i++)
            {
                if (accessor.component_type == 5123) { // UNSIGNED_SHORT
                    outMesh.Indices[i] = reinterpret_cast<const uint16_t*>(dataPtr)[i];
                }
                else if (accessor.component_type == 5125) { // UNSIGNED_INT
                    outMesh.Indices[i] = reinterpret_cast<const uint32_t*>(dataPtr)[i];
                }
                else if (accessor.component_type == 5121) { // UNSIGNED_BYTE
                    outMesh.Indices[i] = dataPtr[i];
                }
            }
        }


        tg3_model_free(&model);
        tg3_error_stack_free(&errors);

    }
}