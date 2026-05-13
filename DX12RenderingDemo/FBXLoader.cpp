#include "FBXLoader.h"
#include "Mesh.h"
#include "GameObject.h"
#include "Material.h"
#include "Texture.h"
#include "Shader.h"
#include "AssetManager.h"

Matrix FBXLoader::ToMatrix(const aiMatrix4x4& m)
{
    aiVector3D scaling;
    aiQuaternion rotation;
    aiVector3D translation;

    m.Decompose(scaling, rotation, translation);

    Matrix S = Matrix::CreateScale(scaling.x, scaling.y, scaling.z);
    Quaternion Q(rotation.x, rotation.y, rotation.z, rotation.w);
    Matrix R = Matrix::CreateFromQuaternion(Q);
    Matrix T = Matrix::CreateTranslation(translation.x, translation.y, translation.z);

    return S * R * T;
}

std::shared_ptr<Mesh> FBXLoader::CreateLitMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    aiMesh* mesh)
{
    std::vector<LitVertex> vertices;
    std::vector<UINT> indices;

    vertices.reserve(mesh->mNumVertices);

    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        const aiVector3D& pos = mesh->mVertices[i];

        aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0, 1, 0);
        aiVector3D texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f, 0.0f, 0.0f);
        aiVector3D tangent = mesh->HasTangentsAndBitangents() ? mesh->mTangents[i] : aiVector3D(1.0f, 0.0f, 0.0f);

        vertices.emplace_back(
            Vector3(pos.x, pos.y, pos.z),
            Vector4(1.0f, 1.0f, 1.0f, 1.0f),
            Vector3(normal.x, normal.y, normal.z),
            Vector3(tangent.x, tangent.y, tangent.z),
            Vector2(texCoord.x, texCoord.y)
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

    return std::make_shared<LoadedMeshLit>(
        device,
        cmdList,
        vertices,
        indices
    );
}

std::vector<std::shared_ptr<Material>> FBXLoader::LoadMaterialsFromMatFiles(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager,
    const aiScene* scene,
    const std::string& modelPath)
{
    std::vector<std::shared_ptr<Material>> materials;

    if (!scene)
        return materials;

    materials.reserve(scene->mNumMaterials);

    const std::filesystem::path modelDir =
        std::filesystem::path(modelPath).parent_path();

    const std::filesystem::path materialDir =
        modelDir.parent_path() / "Materials";

    for (UINT i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* aiMat = scene->mMaterials[i];

        aiString aiMaterialName;
        aiMat->Get(AI_MATKEY_NAME, aiMaterialName);

        const std::string materialName = aiMaterialName.C_Str();
        const std::filesystem::path matPath = materialDir / (materialName + ".mat");

        auto material = assetManager.LoadMaterialFromFile(device, cmdList, rootSignature, matPath);

        if (!material)
            LOG("Material load failed: " << matPath.string());

        materials.push_back(material);

        LOG("Loaded Material[" << i << "]: " << materialName);
    }

    return materials;
}

std::unique_ptr<GameObject> FBXLoader::LoadLitModel(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager,
    const std::string& filePath,
    UINT& objectCBIndex) 
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace
    );

    if (!scene || !scene->mRootNode)
    {
        LOG("FBX Load Failed: " << importer.GetErrorString());
        return nullptr;
    }

    auto materials = LoadMaterialsFromMatFiles(
        device,
        cmdList,
        rootSignature,
        assetManager,
        scene,
        filePath
    );

    return ProcessNode(
        device,
        cmdList,
        scene,
        scene->mRootNode,
        materials,
        objectCBIndex,
        0
    );
}

std::unique_ptr<GameObject> FBXLoader::ProcessNode(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const aiScene* scene,
    aiNode* node,
    const std::vector<std::shared_ptr<Material>>& materials,
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

        auto mesh = CreateLitMesh(device, cmdList, aiMesh);

        UINT materialIndex = aiMesh->mMaterialIndex;

        std::shared_ptr<Material> meshMaterial = nullptr;

        if (materialIndex < materials.size())
            meshMaterial = materials[materialIndex];
        else if (!materials.empty())
            meshMaterial = materials[0];

        LOG(indent << "  MaterialIndex: " << materialIndex);

        auto meshObject = std::make_unique<GameObject>();
        meshObject->SetObjectCBIndex(objectCBIndex++);

        auto* meshRenderer = meshObject->AddComponent<MeshRenderer>();
        meshRenderer->SetMesh(mesh);
        meshRenderer->SetMaterial(meshMaterial);

        object->AddChild(std::move(meshObject));
    }

    for (UINT i = 0; i < node->mNumChildren; ++i)
    {
        auto child = ProcessNode(
            device,
            cmdList,
            scene,
            node->mChildren[i],
            materials,
            objectCBIndex,
            depth + 1
        );

        if (child)
            object->AddChild(std::move(child));
    }

    return object;
}
