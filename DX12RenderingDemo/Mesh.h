#pragma once
#include "Vertex.h"
#include "Asset.h"

class Mesh : public Asset
{
public:
	Mesh() = default;
	Mesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~Mesh();

public:
	void ReleaseUploadResources();
	virtual void Render(ID3D12GraphicsCommandList* cmdList);

	const BoundingBox& GetLocalBounds() const { return localBounds_; }
	bool HasLocalBounds() const { return hasLocalBounds_; }

protected:
	ComPtr<ID3D12Resource> vertexBuffer_;
	ComPtr<ID3D12Resource> vertexUploadBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	ComPtr<ID3D12Resource> indexBuffer_;
	ComPtr<ID3D12Resource> indexUploadBuffer_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT slot_ = 0;
	UINT vertexCnt_ = 0;
	UINT stride_ = 0;
	UINT startVertexLocation_ = 0;
	UINT indexCnt_ = 0;
	UINT startIndex_ = 0;
	int baseVertex_ = 0;

protected:
	void BuildLocalBounds(const std::vector<LitVertex>& vertices);

	BoundingBox localBounds_{};
	bool hasLocalBounds_ = false;
};

class LoadedMeshLit : public Mesh
{
public:
	LoadedMeshLit(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const std::vector<LitVertex>& vertices,
		const std::vector<UINT>& indices);

	virtual ~LoadedMeshLit() {}
};

class RuntimeMeshBufferLit
{
public:
	RuntimeMeshBufferLit(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const std::vector<LitVertex>& vertices,
		const std::vector<UINT>& indices,
		std::vector<ComPtr<ID3D12Resource>>& transientUploadResources
	);

	bool IsValid() const { return vertexBuffer_ && indexBuffer_; }

	const ComPtr<ID3D12Resource>& GetVertexBuffer() const { return vertexBuffer_; }
	const ComPtr<ID3D12Resource>& GetIndexBuffer() const { return indexBuffer_; }

private:
	ComPtr<ID3D12Resource> vertexBuffer_;
	ComPtr<ID3D12Resource> indexBuffer_;
};

class RuntimeMeshLit : public Mesh
{
public:
	RuntimeMeshLit(
		const std::shared_ptr<RuntimeMeshBufferLit>& buffer,
		UINT vertexOffset,
		UINT vertexCount,
		UINT indexOffset,
		UINT indexCount,
		const BoundingBox& localBounds
	);

	virtual ~RuntimeMeshLit() {}
};

class SkyboxMesh : public Mesh
{
public:
	SkyboxMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	virtual ~SkyboxMesh() {}
};

class CubeMesh : public Mesh
{
public:
	CubeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	virtual ~CubeMesh() {}
};

class GlassMesh : public Mesh
{
public:
	GlassMesh(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		float width,
		float height,
		float depth);

	virtual ~GlassMesh() {}

	const Vector3& GetSize() const { return size_; }

private:
	Vector3 size_ = Vector3::One;
};

class SphereMesh : public Mesh
{
public:
	SphereMesh(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		UINT sliceCount = 32,
		UINT stackCount = 16);

	virtual ~SphereMesh() {}
};