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

void Camera::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList
	* pd3dCommandList)
{
}

void Camera::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	Matrix view = m_xmf4x4View.Transpose();
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 16, &view, 0);

	Matrix projection = m_xmf4x4Projection.Transpose();
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 16, &projection, 16);
}

void Camera::ReleaseShaderVariables()
{
}

void Camera::SetViewportsAndScissorRects(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->RSSetViewports(1, &m_d3dViewport);
	pd3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);
}