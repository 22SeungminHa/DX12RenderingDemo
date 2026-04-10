#pragma once

#include "stdafx.h"

#define ASPECT_RATIO (float(FRAME_BUFFER_WIDTH) / float(FRAME_BUFFER_HEIGHT))

struct VS_CB_CAMERA_INFO
{
    Matrix m_xmf4x4View;
    Matrix m_xmf4x4Projection;
};

class CCamera
{
protected:
    // 카메라 변환 행렬
    Matrix m_xmf4x4View;
    // 투영 변환 행렬
    Matrix m_xmf4x4Projection;
    // 뷰포트와 씨저 사각형
    D3D12_VIEWPORT m_d3dViewport;
    D3D12_RECT m_d3dScissorRect;

public:
    CCamera();
    virtual ~CCamera();

    virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
    virtual void ReleaseShaderVariables();
    virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);

    void GenerateViewMatrix(const Vector3& position, const Vector3& lookAt, const Vector3& up);
    void GenerateProjectionMatrix(float nearPlaneDistance, float farPlaneDistance, float aspectRatio, float fovAngle);

    void SetViewport(int xTopLeft, int yTopLeft, int nWidth, int nHeight, float fMinZ = 0.0f, float fMaxZ = 1.0f);
    void SetScissorRect(LONG xLeft, LONG yTop, LONG xRight, LONG yBottom);
    virtual void SetViewportsAndScissorRects(ID3D12GraphicsCommandList* pd3dCommandList);
};