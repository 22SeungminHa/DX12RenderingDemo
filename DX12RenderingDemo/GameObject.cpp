#include "GameObject.h"
#include "Shader.h"
#include "Camera.h"
#include "Material.h"
#include "Mesh.h"

GameObject::GameObject()
{
} 

GameObject::~GameObject()
{
}

void GameObject::ReleaseUploadResources()
{
    meshRenderer_.ReleaseUploadResources();

    for (auto& child : children_)
    {
        if (child)
            child->ReleaseUploadResources();
    }
}

void GameObject::Animate(float fTimeElapsed)
{
}

void GameObject::OnPrepareRender()
{
}

void GameObject::AddChild(std::unique_ptr<GameObject> child)
{
	if (!child) return;

	child->GetTransform()->SetParent(&transform_);
	children_.push_back(std::move(child));
}

void GameObject::RemoveChild(GameObject* child)
{
    auto iter = std::find_if(children_.begin(), children_.end(),
        [child](const std::unique_ptr<GameObject>& ptr) {
            return ptr.get() == child;
        });

    if (iter == children_.end()) return;

    (*iter)->GetTransform()->SetParent(nullptr);
    children_.erase(iter);
}

void GameObject::Rotate(const Vector3& axis, float angle)
{
	transform_.rotation += axis * XMConvertToRadians(angle);
}

RotatingObject::RotatingObject()
{
	rotationAxis_ = Vector3::Up;
	rotationSpeed_ = 90.0f;
}

RotatingObject::~RotatingObject()
{
}

void RotatingObject::Animate(float fTimeElapsed)
{
	GameObject::Rotate(rotationAxis_, rotationSpeed_ * fTimeElapsed);
}
