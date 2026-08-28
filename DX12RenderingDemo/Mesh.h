#pragma once
#include "Vertex.h"
#include "Asset.h"

class Mesh : public Asset
{
public:
	Mesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~Mesh();

public:
	void ReleaseUploadResources();
	virtual void Render(ID3D12GraphicsCommandList* cmdList);

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

class RuntimeMeshLit : public Mesh
{
public:
	RuntimeMeshLit(
		ID3D12Device* device,
		const std::vector<LitVertex>& vertices,
		const std::vector<UINT>& indices);

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