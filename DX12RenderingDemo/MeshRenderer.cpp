#include "MeshRenderer.h"
#include "Mesh.h"
#include "Material.h"
#include "Shader.h"
#include "Camera.h"

void MeshRenderer::ReleaseUploadBuffers()
{
    if (mesh_)
        mesh_->ReleaseUploadBuffers();
}

void MeshRenderer::Render(ID3D12GraphicsCommandList* cmdList, Camera* camera)
{
    if (!cmdList)
        return;

    if (material_ && material_->GetShader())
        material_->GetShader()->Render(cmdList, camera);

    if (mesh_)
        mesh_->Render(cmdList);
}