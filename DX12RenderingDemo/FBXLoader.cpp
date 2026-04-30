#include "FBXLoader.h"
#include "Mesh.h"
#include "GameObject.h"

Matrix FBXLoader::ToMatrix(const aiMatrix4x4& m)
{
    aiVector3D scaling;
    aiQuaternion rotation;
    aiVector3D translation;

    m.Decompose(scaling, rotation, translation);

    Matrix S = Matrix::CreateScale(
        scaling.x,
        scaling.y,
        scaling.z
    );

    Quaternion Q(
        rotation.x,
        rotation.y,
        rotation.z,
        rotation.w
    );

    Matrix R = Matrix::CreateFromQuaternion(Q);

    Matrix T = Matrix::CreateTranslation(
        translation.x,
        translation.y,
        translation.z
    );

    return S * R * T;
}

std::shared_ptr<Mesh> FBXLoader::CreateDiffusedMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    aiMesh* mesh)
{
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

    return std::make_shared<LoadedMeshDiffused>(
        device,
        cmdList,
        vertices,
        indices
    );
}

std::unique_ptr<GameObject> FBXLoader::LoadDiffusedModel(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const std::string& filePath,
    const std::shared_ptr<Material>& material,
    UINT& objectCBIndex)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->mRootNode)
    {
        LOG("FBX Load Failed: " << importer.GetErrorString());
        return nullptr;
    }

    return ProcessNode(
        device,
        cmdList,
        scene,
        scene->mRootNode,
        material,
        objectCBIndex,
        0
    );
}

std::unique_ptr<GameObject> FBXLoader::ProcessNode(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const aiScene* scene,
    aiNode* node,
    const std::shared_ptr<Material>& material,
    UINT& objectCBIndex,
    int depth)
{
    std::string indent(depth * 2, ' ');
    LOG(indent << "Node: " << node->mName.C_Str());
    LOG(indent << "MeshCount: " << node->mNumMeshes);

    auto object = std::make_unique<GameObject>();

    object->SetObjectCBIndex(objectCBIndex++);
    object->GetTransform()->SetLocalMatrix(ToMatrix(node->mTransformation));

    for (UINT i = 0; i < node->mNumMeshes; ++i)
    {
        UINT meshIndex = node->mMeshes[i];
        aiMesh* aiMesh = scene->mMeshes[meshIndex];

        LOG(indent << "  Mesh[" << i << "] Index: " << meshIndex);

        auto mesh = CreateDiffusedMesh(device, cmdList, aiMesh);

        auto meshObject = std::make_unique<GameObject>();
        meshObject->SetObjectCBIndex(objectCBIndex++);
        meshObject->SetMesh(mesh);
        meshObject->SetMaterial(material);

        object->AddChild(std::move(meshObject));
    }

    for (UINT i = 0; i < node->mNumChildren; ++i)
    {
        auto child = ProcessNode(
            device,
            cmdList,
            scene,
            node->mChildren[i],
            material,
            objectCBIndex,
            depth + 1
        );

        if (child)
            object->AddChild(std::move(child));
    }

    return object;
}
