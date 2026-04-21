#include "Camera.h"

Camera::Camera()
{
	viewport_ = { 0.0f, 0.0f, float(FRAME_BUFFER_WIDTH), float(FRAME_BUFFER_HEIGHT), 0.0f, 1.0f };
	scissorRect_ = { 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT };
}

void Camera::SetLookAt(const Vector3& position, const Vector3& target, const Vector3& up)
{
    view_ = Matrix::CreateLookAt(position, target, up);
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