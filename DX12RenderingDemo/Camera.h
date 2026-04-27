#pragma once
#include "UploadBuffer.h"
#include "ShaderTypes.h"

struct CameraDesc
{
    Vector3 eye = { 0,0,-10 };
    Vector3 target = { 0,0,0 };
    Vector3 up = Vector3::Up;

    float nearZ = 1.0f;
    float farZ = 1000.0f;
    float fovY = 90.0f;
};

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

    void Rotate(float deltaYaw, float deltaPitch);
    void MoveForward(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);

    Vector3 GetPosition() const { return position_; }
    Vector3 GetForward() const;
    Vector3 GetRight() const;
    Vector3 GetUp() const;

protected:
    void UpdateViewMatrix();

protected:
    Matrix view_ = Matrix::Identity;
    Matrix projection_ = Matrix::Identity;

    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};

    Vector3 position_ = { 0.0f, 0.0f, -10.0f };
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
};