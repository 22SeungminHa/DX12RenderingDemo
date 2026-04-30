#include "Camera.h"

Camera::Camera()
{
	viewport_ = { 0.0f, 0.0f, float(FRAME_BUFFER_WIDTH), float(FRAME_BUFFER_HEIGHT), 0.0f, 1.0f };
	scissorRect_ = { 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT };
}

void Camera::SetDesc(const CameraDesc& desc)
{
    desc_ = desc;
}

void Camera::SetLookAt(const Vector3& position, const Vector3& target, const Vector3& up)
{
    position_ = position;

    Vector3 forward = target - position;
    forward.Normalize();

    pitch_ = std::asin(forward.y);
    yaw_ = std::atan2(forward.x, forward.z);

    UpdateViewMatrix();
}

void Camera::SetProjection(float nearPlane, float farPlane, float aspectRatio, float fovY)
{
    projection_ = Matrix::CreatePerspectiveFieldOfView(
        XMConvertToRadians(fovY), aspectRatio, nearPlane, farPlane);
}

void Camera::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    viewport_.TopLeftX = x;
    viewport_.TopLeftY = y;
    viewport_.Width = width;
    viewport_.Height = height;
    viewport_.MinDepth = minDepth;
    viewport_.MaxDepth = maxDepth;
}

void Camera::SetScissorRect(LONG left, LONG top, LONG right, LONG bottom)
{
    scissorRect_.left = left;
    scissorRect_.top = top;
    scissorRect_.right = right;
    scissorRect_.bottom = bottom;
}

PassCB Camera::BuildPassCB() const
{
    PassCB passCB{};
    passCB.view = view_.Transpose();
    passCB.proj = projection_.Transpose();
    return passCB;
}

Vector3 Camera::GetForward() const
{
    Vector3 forward;
    forward.x = std::cos(pitch_) * std::sin(yaw_);
    forward.y = std::sin(pitch_);
    forward.z = std::cos(pitch_) * std::cos(yaw_);
    forward.Normalize();
    return forward;
}

Vector3 Camera::GetRight() const
{
    Vector3 right = Vector3::Up.Cross(GetForward());
    right.Normalize();
    return right;
}

Vector3 Camera::GetUp() const
{
    Vector3 up = GetForward().Cross(GetRight());
    up.Normalize();
    return up;
}

void Camera::Rotate(float deltaYaw, float deltaPitch)
{
    yaw_ += deltaYaw;
    pitch_ += deltaPitch;

    constexpr float limit = XMConvertToRadians(89.0f);
    pitch_ = std::clamp(pitch_, -limit, limit);

    UpdateViewMatrix();
}

void Camera::MoveForward(float distance)
{
    position_ += GetForward() * distance;
    UpdateViewMatrix();
}

void Camera::MoveRight(float distance)
{
    position_ += GetRight() * distance;
    UpdateViewMatrix();
}

void Camera::MoveUp(float distance)
{
    position_ += GetUp() * distance;
    UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
    Vector3 target = position_ + GetForward();
    view_ = Matrix::CreateLookAt(position_, target, Vector3::Up);
}

void Camera::Initialize(UINT width, UINT height)
{
    Resize(width, height);
    SetLookAt(desc_.eye, desc_.target, desc_.up);
}

void Camera::Resize(UINT width, UINT height)
{
    float aspect = (height == 0) ? 1.0f : static_cast<float>(width) / height;

    SetViewport(0, 0, static_cast<float>(width), static_cast<float>(height));
    SetScissorRect(0, 0, width, height);
    SetProjection(desc_.nearZ, desc_.farZ, aspect, desc_.fovY);
}