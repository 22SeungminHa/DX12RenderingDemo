#include "FBXLoader.h"
#include "Mesh.h"

std::shared_ptr<Mesh> FBXLoader::LoadDiffusedMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const std::string& filePath)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0)
    {
        LOG("FBX Load Failed: " << importer.GetErrorString() << std::endl);
        return nullptr;
    }

    aiMesh* mesh = scene->mMeshes[0];

    std::vector<DiffusedVertex> vertices;
    std::vector<UINT> indices;

    vertices.reserve(mesh->mNumVertices);

    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        const aiVector3D& pos = mesh->mVertices[i];

        vertices.emplace_back(
            Vector3(pos.x, pos.y, pos.z),
            Vector4(1.0f, 1.0f, 1.0f, 1.0f)
        );
    }

    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];

        for (UINT j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    LOG("FBX Mesh Load Success" << std::endl);
    LOG("Vertex Count: " << vertices.size() << std::endl);
    LOG("Index Count: " << indices.size() << std::endl);

    return std::make_shared<LoadedMeshDiffused>(
        device,
        cmdList,
        vertices,
        indices
    );
}