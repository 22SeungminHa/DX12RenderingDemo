#pragma once
#include "Vertex.h"
#include "Asset.h"

class Mesh : public Asset
{
public:
	Mesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~Mesh();

public:
	void ReleaseUploadBuffers();
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

class TriangleMesh : public Mesh
{
public:
	TriangleMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~TriangleMesh(){ }
};

class CubeMeshDiffused : public Mesh
{
public:
	//직육면체의 가로, 세로, 깊이의 길이를 지정하여 직육면체 메쉬를 생성한다.
	CubeMeshDiffused(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth = 2.0f, float fHeight = 2.0f, float fDepth = 2.0f);
	virtual ~CubeMeshDiffused();
};

class LoadedMeshDiffused : public Mesh
{
public:
	LoadedMeshDiffused(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const std::vector<DiffusedVertex>& vertices,
		const std::vector<UINT>& indices);

	virtual ~LoadedMeshDiffused() {}
};