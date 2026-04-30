#include "MeshRenderer.h"
#include "Mesh.h"

void MeshRenderer::ReleaseUploadBuffers()
{
    if (mesh_)
        mesh_->ReleaseUploadBuffers();
}
