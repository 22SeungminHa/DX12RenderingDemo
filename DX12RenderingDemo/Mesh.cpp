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