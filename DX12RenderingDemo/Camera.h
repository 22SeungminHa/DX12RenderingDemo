#pragma once
#include "UploadBuffer.h"
#include "ShaderTypes.h"

#define ASPECT_RATIO (float(FRAME_BUFFER_WIDTH) / float(FRAME_BUFFER_HEIGHT))

class Camera
{
public:
    Camera();
    virtual ~Camera() = default;

public:
    // view / projection
    void SetLookAt(const Vector3& position, const Vector3& target, const Vector3& up = Vector3::Up);
    void SetProjection(float nearPlane, float farPlane, float aspectRatio, float fovY);

    // viewport / scissor
    void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
    void SetScissorRect(LONG left, LONG top, LONG right, LONG bottom);

    // getters
    const Matrix& GetViewMatrix() const { return view_; }
    const Matrix& GetProjectionMatrix() const { return projection_; }
    const D3D12_VIEWPORT& GetViewport() const { return viewport_; }
    const D3D12_RECT& GetScissorRect() const { return scissorRect_; }

    // pass data
    PassCB BuildPassCB() const;

protected:
    Matrix view_ = Matrix::Identity;
    Matrix projection_ = Matrix::Identity;

    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};
};