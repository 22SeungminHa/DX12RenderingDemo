#include "Mesh.h"

Mesh::Mesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommanList)
{
}

Mesh::~Mesh()
{
}

void Mesh::ReleaseUploadResources()
{
	vertexUploadBuffer_.Reset();
	indexUploadBuffer_.Reset();
}

void Mesh::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->IASetPrimitiveTopology(primitiveTopology_);
	pd3dCommandList->IASetVertexBuffers(slot_, 1, &vertexBufferView_);

	if (indexBuffer_) {
		//인덱스 버퍼가 있으면 인덱스 버퍼를 파이프라인(IA: 입력 조립기)에 연결하고 인덱스를 사용하여 렌더링한다.
		pd3dCommandList->IASetIndexBuffer(&indexBufferView_);
		pd3dCommandList->DrawIndexedInstanced(indexCnt_, 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(vertexCnt_, 1, startVertexLocation_, 0);
	}
}

void Mesh::BuildLocalBounds(
    const std::vector<LitVertex>& vertices)
{
    if (vertices.empty())
    {
        localBounds_ = BoundingBox{};
        hasLocalBounds_ = false;
        return;
    }

    Vector3 minPosition = vertices[0].GetPosition();
    Vector3 maxPosition = vertices[0].GetPosition();

    for (const LitVertex& vertex : vertices)
    {
        const Vector3& position = vertex.GetPosition();

        minPosition.x = std::min(minPosition.x, position.x);
        minPosition.y = std::min(minPosition.y, position.y);
        minPosition.z = std::min(minPosition.z, position.z);

        maxPosition.x = std::max(maxPosition.x, position.x);
        maxPosition.y = std::max(maxPosition.y, position.y);
        maxPosition.z = std::max(maxPosition.z, position.z);
    }

    const Vector3 center =
        (minPosition + maxPosition) * 0.5f;

    const Vector3 extents =
        (maxPosition - minPosition) * 0.5f;

    localBounds_.Center =
    {
        center.x,
        center.y,
        center.z
    };

    localBounds_.Extents =
    {
        extents.x,
        extents.y,
        extents.z
    };

    hasLocalBounds_ = true;
}

LoadedMeshLit::LoadedMeshLit(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const std::vector<LitVertex>& vertices,
	const std::vector<UINT>& indices)
	: Mesh(device, cmdList)
{
    BuildLocalBounds(vertices);

    vertexCnt_ = static_cast<UINT>(vertices.size());
    indexCnt_ = static_cast<UINT>(indices.size());
	stride_ = sizeof(LitVertex);
	primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	vertexBuffer_ = D3DUtil::CreateBufferResource(
		device,
		cmdList,
		vertices.data(),
		stride_ * vertexCnt_,
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		vertexUploadBuffer_);

	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.StrideInBytes = stride_;
	vertexBufferView_.SizeInBytes = stride_ * vertexCnt_;

	indexBuffer_ = D3DUtil::CreateBufferResource(
		device,
		cmdList,
		indices.data(),
		sizeof(UINT) * indexCnt_,
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_INDEX_BUFFER,
		indexUploadBuffer_);

	indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView_.SizeInBytes = sizeof(UINT) * indexCnt_;
}

RuntimeMeshBufferLit::RuntimeMeshBufferLit(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const std::vector<LitVertex>& vertices,
    const std::vector<UINT>& indices,
    std::vector<ComPtr<ID3D12Resource>>& transientUploadResources)
{
    if (!device || !cmdList || vertices.empty() || indices.empty())
        return;

    ComPtr<ID3D12Resource> vertexUploadBuffer;
    ComPtr<ID3D12Resource> indexUploadBuffer;

    vertexBuffer_ = D3DUtil::CreateBufferResource(
        device,
        cmdList,
        vertices.data(),
        static_cast<UINT64>(sizeof(LitVertex)) * vertices.size(),
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        vertexUploadBuffer
    );

    indexBuffer_ = D3DUtil::CreateBufferResource(
        device,
        cmdList,
        indices.data(),
        static_cast<UINT64>(sizeof(UINT)) * indices.size(),
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_INDEX_BUFFER,
        indexUploadBuffer
    );

    if (!vertexBuffer_ || !indexBuffer_)
        return;

    if (vertexUploadBuffer)
        transientUploadResources.push_back(vertexUploadBuffer);

    if (indexUploadBuffer)
        transientUploadResources.push_back(indexUploadBuffer);
}

RuntimeMeshLit::RuntimeMeshLit(
    const std::shared_ptr<RuntimeMeshBufferLit>& buffer,
    UINT vertexOffset,
    UINT vertexCount,
    UINT indexOffset,
    UINT indexCount,
    const BoundingBox& localBounds)
    : Mesh()
{
    if (!buffer || !buffer->IsValid() || vertexCount == 0 || indexCount == 0)
        return;

    vertexBuffer_ = buffer->GetVertexBuffer();
    indexBuffer_ = buffer->GetIndexBuffer();

    vertexCnt_ = vertexCount;
    indexCnt_ = indexCount;

    stride_ = sizeof(LitVertex);
    primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    const UINT64 vertexOffsetBytes =
        static_cast<UINT64>(vertexOffset) * sizeof(LitVertex);

    const UINT64 indexOffsetBytes =
        static_cast<UINT64>(indexOffset) * sizeof(UINT);

    vertexBufferView_.BufferLocation =
        vertexBuffer_->GetGPUVirtualAddress() + vertexOffsetBytes;

    vertexBufferView_.StrideInBytes = sizeof(LitVertex);
    vertexBufferView_.SizeInBytes =
        sizeof(LitVertex) * vertexCount;

    indexBufferView_.BufferLocation =
        indexBuffer_->GetGPUVirtualAddress() + indexOffsetBytes;

    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexBufferView_.SizeInBytes =
        sizeof(UINT) * indexCount;

    localBounds_ = localBounds;
    hasLocalBounds_ = true;
}

SkyboxMesh::SkyboxMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList)
    : Mesh(device, cmdList)
{
    std::vector<SkyboxVertex> vertices =
    {
        {{-1.0f, -1.0f, -1.0f}},
        {{-1.0f,  1.0f, -1.0f}},
        {{ 1.0f,  1.0f, -1.0f}},
        {{ 1.0f, -1.0f, -1.0f}},
        {{-1.0f, -1.0f,  1.0f}},
        {{-1.0f,  1.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f}},
        {{ 1.0f, -1.0f,  1.0f}},
    };

    std::vector<UINT> indices =
    {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    };

    vertexCnt_ = static_cast<UINT>(vertices.size());
    indexCnt_ = static_cast<UINT>(indices.size());
    stride_ = sizeof(SkyboxVertex);
    primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    vertexBuffer_ = D3DUtil::CreateBufferResource(
        device,
        cmdList,
        vertices.data(),
        stride_ * vertexCnt_,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        vertexUploadBuffer_);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.StrideInBytes = stride_;
    vertexBufferView_.SizeInBytes = stride_ * vertexCnt_;

    indexBuffer_ = D3DUtil::CreateBufferResource(
        device,
        cmdList,
        indices.data(),
        sizeof(UINT) * indexCnt_,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_INDEX_BUFFER,
        indexUploadBuffer_);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexBufferView_.SizeInBytes = sizeof(UINT) * indexCnt_;
}

CubeMesh::CubeMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList)
    : Mesh(device, cmdList)
{
    std::vector<LitVertex> vertices =
    {
        // Front (-Z)
        {{-1,-1,-1}, Vector4::One, { 0, 0,-1}, { 1, 0, 0}, {0,1}},
        {{-1, 1,-1}, Vector4::One, { 0, 0,-1}, { 1, 0, 0}, {0,0}},
        {{ 1, 1,-1}, Vector4::One, { 0, 0,-1}, { 1, 0, 0}, {1,0}},
        {{ 1,-1,-1}, Vector4::One, { 0, 0,-1}, { 1, 0, 0}, {1,1}},

        // Back (+Z)
        {{ 1,-1, 1}, Vector4::One, { 0, 0, 1}, {-1, 0, 0}, {0,1}},
        {{ 1, 1, 1}, Vector4::One, { 0, 0, 1}, {-1, 0, 0}, {0,0}},
        {{-1, 1, 1}, Vector4::One, { 0, 0, 1}, {-1, 0, 0}, {1,0}},
        {{-1,-1, 1}, Vector4::One, { 0, 0, 1}, {-1, 0, 0}, {1,1}},

        // Left (-X)
        {{-1,-1, 1}, Vector4::One, {-1, 0, 0}, { 0, 0,-1}, {0,1}},
        {{-1, 1, 1}, Vector4::One, {-1, 0, 0}, { 0, 0,-1}, {0,0}},
        {{-1, 1,-1}, Vector4::One, {-1, 0, 0}, { 0, 0,-1}, {1,0}},
        {{-1,-1,-1}, Vector4::One, {-1, 0, 0}, { 0, 0,-1}, {1,1}},

        // Right (+X)
        {{ 1,-1,-1}, Vector4::One, { 1, 0, 0}, { 0, 0, 1}, {0,1}},
        {{ 1, 1,-1}, Vector4::One, { 1, 0, 0}, { 0, 0, 1}, {0,0}},
        {{ 1, 1, 1}, Vector4::One, { 1, 0, 0}, { 0, 0, 1}, {1,0}},
        {{ 1,-1, 1}, Vector4::One, { 1, 0, 0}, { 0, 0, 1}, {1,1}},

        // Top (+Y)
        {{-1, 1,-1}, Vector4::One, { 0, 1, 0}, { 1, 0, 0}, {0,1}},
        {{-1, 1, 1}, Vector4::One, { 0, 1, 0}, { 1, 0, 0}, {0,0}},
        {{ 1, 1, 1}, Vector4::One, { 0, 1, 0}, { 1, 0, 0}, {1,0}},
        {{ 1, 1,-1}, Vector4::One, { 0, 1, 0}, { 1, 0, 0}, {1,1}},

        // Bottom (-Y)
        {{-1,-1, 1}, Vector4::One, { 0,-1, 0}, { 1, 0, 0}, {0,1}},
        {{-1,-1,-1}, Vector4::One, { 0,-1, 0}, { 1, 0, 0}, {0,0}},
        {{ 1,-1,-1}, Vector4::One, { 0,-1, 0}, { 1, 0, 0}, {1,0}},
        {{ 1,-1, 1}, Vector4::One, { 0,-1, 0}, { 1, 0, 0}, {1,1}},
    };

    BuildLocalBounds(vertices);

    std::vector<UINT> indices =
    {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23
    };

    vertexCnt_ = static_cast<UINT>(vertices.size());
    indexCnt_ = static_cast<UINT>(indices.size());

    stride_ = sizeof(LitVertex);

    primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    vertexBuffer_ = D3DUtil::CreateBufferResource(
        device,
        cmdList,
        vertices.data(),
        stride_ * vertexCnt_,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        vertexUploadBuffer_);

    vertexBufferView_.BufferLocation =
        vertexBuffer_->GetGPUVirtualAddress();

    vertexBufferView_.StrideInBytes = stride_;
    vertexBufferView_.SizeInBytes = stride_ * vertexCnt_;

    indexBuffer_ = D3DUtil::CreateBufferResource(
        device,
        cmdList,
        indices.data(),
        sizeof(UINT) * indexCnt_,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_INDEX_BUFFER,
        indexUploadBuffer_);

    indexBufferView_.BufferLocation =
        indexBuffer_->GetGPUVirtualAddress();

    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexBufferView_.SizeInBytes = sizeof(UINT) * indexCnt_;
}

GlassMesh::GlassMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    float width,
    float height,
    float depth)
    : Mesh(device, cmdList),
    size_(width, height, depth)
{
    const float hx = width * 0.5f;
    const float hy = height * 0.5f;
    const float hz = depth * 0.5f;

    std::vector<LitVertex> vertices =
    {
        // Front (-Z)
        {{-hx,-hy,-hz}, Vector4::One, { 0, 0,-1}, { 1, 0, 0}, {0,1}},
        {{-hx, hy,-hz}, Vector4::One, { 0, 0,-1}, { 1, 0, 0}, {0,0}},
        {{ hx, hy,-hz}, Vector4::One, { 0, 0,-1}, { 1, 0, 0}, {1,0}},
        {{ hx,-hy,-hz}, Vector4::One, { 0, 0,-1}, { 1, 0, 0}, {1,1}},

        // Back (+Z)
        {{ hx,-hy, hz}, Vector4::One, { 0, 0, 1}, {-1, 0, 0}, {0,1}},
        {{ hx, hy, hz}, Vector4::One, { 0, 0, 1}, {-1, 0, 0}, {0,0}},
        {{-hx, hy, hz}, Vector4::One, { 0, 0, 1}, {-1, 0, 0}, {1,0}},
        {{-hx,-hy, hz}, Vector4::One, { 0, 0, 1}, {-1, 0, 0}, {1,1}},

        // Left (-X)
        {{-hx,-hy, hz}, Vector4::One, {-1, 0, 0}, { 0, 0,-1}, {0,1}},
        {{-hx, hy, hz}, Vector4::One, {-1, 0, 0}, { 0, 0,-1}, {0,0}},
        {{-hx, hy,-hz}, Vector4::One, {-1, 0, 0}, { 0, 0,-1}, {1,0}},
        {{-hx,-hy,-hz}, Vector4::One, {-1, 0, 0}, { 0, 0,-1}, {1,1}},

        // Right (+X)
        {{ hx,-hy,-hz}, Vector4::One, { 1, 0, 0}, { 0, 0, 1}, {0,1}},
        {{ hx, hy,-hz}, Vector4::One, { 1, 0, 0}, { 0, 0, 1}, {0,0}},
        {{ hx, hy, hz}, Vector4::One, { 1, 0, 0}, { 0, 0, 1}, {1,0}},
        {{ hx,-hy, hz}, Vector4::One, { 1, 0, 0}, { 0, 0, 1}, {1,1}},

        // Top (+Y)
        {{-hx, hy,-hz}, Vector4::One, { 0, 1, 0}, { 1, 0, 0}, {0,1}},
        {{-hx, hy, hz}, Vector4::One, { 0, 1, 0}, { 1, 0, 0}, {0,0}},
        {{ hx, hy, hz}, Vector4::One, { 0, 1, 0}, { 1, 0, 0}, {1,0}},
        {{ hx, hy,-hz}, Vector4::One, { 0, 1, 0}, { 1, 0, 0}, {1,1}},

        // Bottom (-Y)
        {{-hx,-hy, hz}, Vector4::One, { 0,-1, 0}, { 1, 0, 0}, {0,1}},
        {{-hx,-hy,-hz}, Vector4::One, { 0,-1, 0}, { 1, 0, 0}, {0,0}},
        {{ hx,-hy,-hz}, Vector4::One, { 0,-1, 0}, { 1, 0, 0}, {1,0}},
        {{ hx,-hy, hz}, Vector4::One, { 0,-1, 0}, { 1, 0, 0}, {1,1}},
    };

    BuildLocalBounds(vertices);

    std::vector<UINT> indices =
    {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23
    };

    vertexCnt_ = static_cast<UINT>(vertices.size());
    indexCnt_ = static_cast<UINT>(indices.size());
    stride_ = sizeof(LitVertex);
    primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    vertexBuffer_ = D3DUtil::CreateBufferResource(
        device,
        cmdList,
        vertices.data(),
        stride_ * vertexCnt_,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        vertexUploadBuffer_);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.StrideInBytes = stride_;
    vertexBufferView_.SizeInBytes = stride_ * vertexCnt_;

    indexBuffer_ = D3DUtil::CreateBufferResource(
        device,
        cmdList,
        indices.data(),
        sizeof(UINT) * indexCnt_,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_INDEX_BUFFER,
        indexUploadBuffer_);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexBufferView_.SizeInBytes = sizeof(UINT) * indexCnt_;
}

SphereMesh::SphereMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    UINT sliceCount,
    UINT stackCount)
    : Mesh(device, cmdList)
{
    sliceCount = std::max<UINT>(sliceCount, 3);
    stackCount = std::max<UINT>(stackCount, 2);

    std::vector<LitVertex> vertices;
    std::vector<UINT> indices;

    vertices.reserve((stackCount + 1) * (sliceCount + 1));
    indices.reserve(stackCount * sliceCount * 6);

    for (UINT stack = 0; stack <= stackCount; ++stack)
    {
        const float phi = XM_PI * static_cast<float>(stack) / static_cast<float>(stackCount);

        for (UINT slice = 0; slice <= sliceCount; ++slice)
        {
            const float theta = XM_2PI * static_cast<float>(slice) / static_cast<float>(sliceCount);

            Vector3 position(
                sinf(phi) * cosf(theta),
                cosf(phi),
                sinf(phi) * sinf(theta)
            );

            Vector3 normal = position;
            normal.Normalize();

            Vector3 tangent(
                -sinf(theta),
                0.0f,
                cosf(theta)
            );
            tangent.Normalize();

            Vector2 texCoord(
                static_cast<float>(slice) / static_cast<float>(sliceCount),
                static_cast<float>(stack) / static_cast<float>(stackCount)
            );

            vertices.emplace_back(
                position,
                Vector4::One,
                normal,
                tangent,
                texCoord
            );
        }
    }

    BuildLocalBounds(vertices);

    const UINT ringVertexCount = sliceCount + 1;

    for (UINT stack = 0; stack < stackCount; ++stack)
    {
        for (UINT slice = 0; slice < sliceCount; ++slice)
        {
            const UINT i0 = stack * ringVertexCount + slice;
            const UINT i1 = i0 + 1;
            const UINT i2 = i0 + ringVertexCount;
            const UINT i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i2);
        }
    }

    vertexCnt_ = static_cast<UINT>(vertices.size());
    indexCnt_ = static_cast<UINT>(indices.size());
    stride_ = sizeof(LitVertex);
    primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    vertexBuffer_ = D3DUtil::CreateBufferResource(
        device,
        cmdList,
        vertices.data(),
        stride_ * vertexCnt_,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        vertexUploadBuffer_);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.StrideInBytes = stride_;
    vertexBufferView_.SizeInBytes = stride_ * vertexCnt_;

    indexBuffer_ = D3DUtil::CreateBufferResource(
        device,
        cmdList,
        indices.data(),
        sizeof(UINT) * indexCnt_,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_INDEX_BUFFER,
        indexUploadBuffer_);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexBufferView_.SizeInBytes = sizeof(UINT) * indexCnt_;
}