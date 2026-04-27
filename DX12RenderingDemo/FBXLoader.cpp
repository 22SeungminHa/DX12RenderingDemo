#include "FBXLoader.h"

void FBXLoader::PrintFBXVertices(const std::string& filePath)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene)
    {
        std::cout << "FBX Load Failed: " << importer.GetErrorString() << std::endl;
        return;
    }

    if (!scene->mRootNode)
    {
        std::cout << "RootNode is null" << std::endl;
        return;
    }

    std::cout << "FBX Load Success" << std::endl;
    std::cout << "Mesh Count: " << scene->mNumMeshes << std::endl;

    for (UINT meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
    {
        aiMesh* mesh = scene->mMeshes[meshIndex];

        std::cout << "\n========== Mesh " << meshIndex << " ==========" << std::endl;
        std::cout << "Mesh Name: " << mesh->mName.C_Str() << std::endl;
        std::cout << "Vertex Count: " << mesh->mNumVertices << std::endl;

        for (UINT vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
        {
            const aiVector3D& pos = mesh->mVertices[vertexIndex];

            std::cout
                << "Vertex[" << vertexIndex << "] "
                << "x: " << pos.x << ", "
                << "y: " << pos.y << ", "
                << "z: " << pos.z
                << std::endl;
        }
    }
}