#include "MeshRenderer.h"
#include "Mesh.h"

void MeshRenderer::ReleaseUploadResources()
{
    if (mesh_)
        mesh_->ReleaseUploadResources();
}
