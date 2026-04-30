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

TriangleMesh::TriangleMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) : Mesh(pd3dDevice, pd3dCommandList)
{
	// 삼각형 메쉬 정의
	vertexCnt_ = 3;
	stride_ = sizeof(LitVertex);
	primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 정점의 색상을 시계방향 순서대로 R, G, B 로 지정 RGBA로 색상을 표현.
	LitVertex pVertices[3];
	pVertices[0] = LitVertex(Vector3(0.0f, 6.0f, 0.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f));
	pVertices[1] = LitVertex(Vector3(6.0f, -6.0f, 0.0f), Vector4(0.0f, 1.0f, 0.0f, 1.0f));
	pVertices[2] = LitVertex(Vector3(-6.0f, -6.0f, 0.0f), Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	
	vertexBuffer_ = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, stride_ * vertexCnt_, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertexUploadBuffer_);

	// 정점 버퍼 뷰 생성
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.StrideInBytes = stride_;
	vertexBufferView_.SizeInBytes = stride_ * vertexCnt_;
}

CubeMeshLit::CubeMeshLit(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList * pd3dCommandList, float fWidth, float fHeight, float fDepth) : Mesh(pd3dDevice, pd3dCommandList)
{
	//직육면체는 꼭지점(정점)이 8개이다.
	vertexCnt_ = 8;
	stride_ = sizeof(LitVertex);
	primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	float fx = fWidth * 0.5f, fy = fHeight * 0.5f, fz = fDepth * 0.5f;

	//정점 버퍼는 직육면체의 꼭지점 8개에 대한 정점 데이터를 가진다.
	LitVertex pVertices[8];
	pVertices[0] = LitVertex(Vector3(-fx, +fy, -fz), RANDOM_COLOR);
	pVertices[1] = LitVertex(Vector3(+fx, +fy, -fz), RANDOM_COLOR);
	pVertices[2] = LitVertex(Vector3(+fx, +fy, +fz), RANDOM_COLOR);
	pVertices[3] = LitVertex(Vector3(-fx, +fy, +fz), RANDOM_COLOR);
	pVertices[4] = LitVertex(Vector3(-fx, -fy, -fz), RANDOM_COLOR);
	pVertices[5] = LitVertex(Vector3(+fx, -fy, -fz), RANDOM_COLOR);
	pVertices[6] = LitVertex(Vector3(+fx, -fy, +fz), RANDOM_COLOR);
	pVertices[7] = LitVertex(Vector3(-fx, -fy, +fz), RANDOM_COLOR);
	vertexBuffer_ = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, stride_ * vertexCnt_, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertexUploadBuffer_);
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.StrideInBytes = stride_;
	vertexBufferView_.SizeInBytes = stride_ * vertexCnt_;

	/*인덱스 버퍼는 직육면체의 6개의 면(사각형)에 대한 기하 정보를 갖는다. 삼각형 리스트로 직육면체를 표현할 것이
   므로 각 면은 2개의 삼각형을 가지고 각 삼각형은 3개의 정점이 필요하다. 즉, 인덱스 버퍼는 전체 36(=6*2*3)개의 인
   덱스를 가져야 한다.*/

	indexCnt_ = 36;
	UINT pnIndices[36];

	//ⓐ 앞면(Front) 사각형의 위쪽 삼각형
	pnIndices[0] = 3; pnIndices[1] = 1; pnIndices[2] = 0;

	//ⓑ 앞면(Front) 사각형의 아래쪽 삼각형
	pnIndices[3] = 2; pnIndices[4] = 1; pnIndices[5] = 3;

	//ⓒ 윗면(Top) 사각형의 위쪽 삼각형
	pnIndices[6] = 0; pnIndices[7] = 5; pnIndices[8] = 4;

	//ⓓ 윗면(Top) 사각형의 아래쪽 삼각형
	pnIndices[9] = 1; pnIndices[10] = 5; pnIndices[11] = 0;

	//ⓔ 뒷면(Back) 사각형의 위쪽 삼각형
	pnIndices[12] = 3; pnIndices[13] = 4; pnIndices[14] = 7;

	//ⓕ 뒷면(Back) 사각형의 아래쪽 삼각형
	pnIndices[15] = 0; pnIndices[16] = 4; pnIndices[17] = 3;

	//ⓖ 아래면(Bottom) 사각형의 위쪽 삼각형
	pnIndices[18] = 1; pnIndices[19] = 6; pnIndices[20] = 5;

	//ⓗ 아래면(Bottom) 사각형의 아래쪽 삼각형
	pnIndices[21] = 2; pnIndices[22] = 6; pnIndices[23] = 1;

	//ⓘ 옆면(Left) 사각형의 위쪽 삼각형
	pnIndices[24] = 2; pnIndices[25] = 7; pnIndices[26] = 6;

	//ⓙ 옆면(Left) 사각형의 아래쪽 삼각형
	pnIndices[27] = 3; pnIndices[28] = 7; pnIndices[29] = 2;

	//ⓚ 옆면(Right) 사각형의 위쪽 삼각형
	pnIndices[30] = 6; pnIndices[31] = 4; pnIndices[32] = 5;

	//ⓛ 옆면(Right) 사각형의 아래쪽 삼각형
	pnIndices[33] = 7; pnIndices[34] = 4; pnIndices[35] = 6;

	//인덱스 버퍼를 생성한다.
	indexBuffer_ = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pnIndices,
		sizeof(UINT) * indexCnt_, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER,
		indexUploadBuffer_);

	//인덱스 버퍼 뷰를 생성한다.
	indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView_.SizeInBytes = sizeof(UINT) * indexCnt_;
}

CubeMeshLit::~CubeMeshLit()
{
}

LoadedMeshLit::LoadedMeshLit(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const std::vector<LitVertex>& vertices,
	const std::vector<UINT>& indices)
	: Mesh(device, cmdList)
{
	vertexCnt_ = static_cast<UINT>(vertices.size());
	indexCnt_ = static_cast<UINT>(indices.size());
	stride_ = sizeof(LitVertex);
	primitiveTopology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	vertexBuffer_ = ::CreateBufferResource(
		device,
		cmdList,
		const_cast<LitVertex*>(vertices.data()),
		stride_ * vertexCnt_,
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		vertexUploadBuffer_);

	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.StrideInBytes = stride_;
	vertexBufferView_.SizeInBytes = stride_ * vertexCnt_;

	indexBuffer_ = ::CreateBufferResource(
		device,
		cmdList,
		const_cast<UINT*>(indices.data()),
		sizeof(UINT) * indexCnt_,
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_INDEX_BUFFER,
		indexUploadBuffer_);

	indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView_.SizeInBytes = sizeof(UINT) * indexCnt_;
}