#pragma once
#include "pch.h"

class Vertex
{
protected:
	Vector3 position_;
public:
	Vertex() { position_ = Vector3::Zero;  }
	Vertex(const Vector3& xmf3Position) { position_ = xmf3Position; }
	~Vertex() {}
};

class DiffusedVertex : public Vertex
{
protected:
	Vector4 diffuse_;

public:
	DiffusedVertex() { position_ = Vector3::Zero; diffuse_ = Vector4::Zero; }
	DiffusedVertex(float x, float y, float z, const Vector4& xmf4Diffuse) { position_ = Vector3(x, y, z); diffuse_ = xmf4Diffuse; }
	DiffusedVertex(const Vector3& xmf3Position, const Vector4& xmf4Diffuse) { position_ = xmf3Position; diffuse_ = xmf4Diffuse; }
	~DiffusedVertex() { }
};

class Mesh
{
public:
	Mesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~Mesh();

public:
	void ReleaseUploadBuffers();

protected:
	ComPtr<ID3D12Resource> vertexBuffer_ = NULL;
	ComPtr<ID3D12Resource> vertexUploadBuffer_ = NULL;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;

	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT slot_ = 0;
	UINT vertexCount_ = 0;
	UINT stride_ = 0;
	UINT startVertexLocation_ = 0;

	ComPtr<ID3D12Resource> indexBuffer_ = NULL;
	ComPtr<ID3D12Resource> indexUploadBuffer_ = NULL;

	D3D12_INDEX_BUFFER_VIEW indexBufferView_;
	/*인덱스 버퍼(인덱스의 배열)와 인덱스 버퍼를 위한 업로드 버퍼에 대한 인터페이스 포인터이다. 
	인덱스 버퍼는 정점 버퍼(배열)에 대한 인덱스를 가진다.*/

	UINT indexCount_ = 0;		//인덱스 버퍼에 포함되는 인덱스의 개수이다.
	UINT startIndex_ = 0; 	//인덱스 버퍼에서 메쉬를 그리기 위해 사용되는 시작 인덱스이다. 
	int baseVertex_ = 0;		//인덱스 버퍼의 인덱스에 더해질 인덱스이다.

public:
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList);
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