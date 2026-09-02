#pragma once
#include "UploadBuffer.h"
#include "EngineTypes.h"

enum class CameraMode
{
    AutoForward,
    Free
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
    void SetDesc(const CameraDesc& desc);

    // viewport / scissor
    void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
    void SetScissorRect(LONG left, LONG top, LONG right, LONG bottom);

    // getters
    const Matrix& GetViewMatrix() const { return view_; }
    const Matrix& GetProjectionMatrix() const { return projection_; }
    const D3D12_VIEWPORT& GetViewport() const { return viewport_; }
    const D3D12_RECT& GetScissorRect() const { return scissorRect_; }
    const CameraDesc& GetDesc() const { return desc_; }

    Vector3 GetPosition() const { return position_; }
    Vector3 GetForward() const;
    Vector3 GetRight() const;
    Vector3 GetUp() const;

    // pass data
    PassCB BuildPassCB() const;

    void Rotate(float deltaYaw, float deltaPitch);
    void MoveForward(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);

    void Initialize(UINT width, UINT height);
    void Resize(UINT width, UINT height);

    Vector3 ScreenPointToWorldDirection(const Vector2& screenPosition) const;

    void Update(float deltaTime);

    void SetMode(CameraMode mode) { mode_ = mode; }
    CameraMode GetMode() const { return mode_; }
    void ToggleMode();

    void SetAutoForwardSpeed(float speed);
    float GetAutoForwardSpeed() const { return autoForwardSpeed_; }

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

    CameraDesc desc_;

    CameraMode mode_ = CameraMode::AutoForward;
    float autoForwardSpeed_ = 5.0f;
};