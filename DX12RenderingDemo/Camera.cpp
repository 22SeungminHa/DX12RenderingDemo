#include "Camera.h"

Camera::Camera()
{
	m_xmf4x4View = Matrix::Identity;
	m_xmf4x4Projection = Matrix::Identity;
	m_d3dViewport = { 0, 0, FRAME_BUFFER_WIDTH , FRAME_BUFFER_HEIGHT, 0.0f, 1.0f };
	m_d3dScissorRect = { 0, 0, FRAME_BUFFER_WIDTH , FRAME_BUFFER_HEIGHT };
}

Camera::~Camera()
{

}

void Camera::SetViewport(int xTopLeft, int yTopLeft, int nWidth, int nHeight, float fMinZ, float fMaxZ)
{
	m_d3dViewport.TopLeftX = float(xTopLeft);
	m_d3dViewport.TopLeftY = float(yTopLeft);
	m_d3dViewport.Width = float(nWidth);
	m_d3dViewport.Height = float(nHeight);
	m_d3dViewport.MinDepth = fMinZ;
	m_d3dViewport.MaxDepth = fMaxZ;
}

void Camera::SetScissorRect(LONG xLeft, LONG yTop, LONG xRight, LONG yBottom)
{
	m_d3dScissorRect.left = xLeft;
	m_d3dScissorRect.top = yTop;
	m_d3dScissorRect.right = xRight;
	m_d3dScissorRect.bottom = yBottom;
}

void Camera::GenerateProjectionMatrix(float nearPlaneDistance, float farPlaneDistance, float aspectRatio, float fovAngle)
{
	m_xmf4x4Projection = Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(fovAngle), aspectRatio, nearPlaneDistance, farPlaneDistance);
}

void Camera::GenerateViewMatrix(const Vector3& position, const Vector3& lookAt, const Vector3& up)
{
	m_xmf4x4View = Matrix::CreateLookAt(position, lookAt, up);
}

void Camera::CreateShaderVariables(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	passCB_ = std::make_unique<UploadBuffer<PassCB>>(device, 1, true);
}

void Camera::UpdateShaderVariables(ID3D12GraphicsCommandList* cmdList)
{
	PassCB cbData{};
	cbData.view = m_xmf4x4View.Transpose();
	cbData.proj = m_xmf4x4Projection.Transpose();

	passCB_->CopyData(0, cbData);

	cmdList->SetGraphicsRootConstantBufferView(1, passCB_->GetResource()->GetGPUVirtualAddress());
}

void Camera::ReleaseShaderVariables()
{
	passCB_.reset();
}

void Camera::SetViewportsAndScissorRects(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->RSSetViewports(1, &m_d3dViewport);
	cmdList->RSSetScissorRects(1, &m_d3dScissorRect);
}