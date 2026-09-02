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

void GameObject::Animate(float deltaTime)
{
    for (auto& component : components_)
    {
        if (component)
            component->Update(deltaTime);
    }
}

void GameObject::OnPrepareRender()
{
    for (auto& component : components_)
        if (component) component->OnPrepareRender();
}

void GameObject::RemovePendingDestroyChildren()
{
    for (auto& child : children_)
    {
        if (child)
            child->RemovePendingDestroyChildren();
    }

    children_.erase(std::remove_if(children_.begin(), children_.end(),
        [](const std::unique_ptr<GameObject>& child)
        {
            return !child || child->IsPendingDestroy();
        }),
        children_.end()
    );
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
    Vector3 normalizedAxis = axis;

    if (normalizedAxis.LengthSquared() <= 0.000001f)
        return;

    normalizedAxis.Normalize();

    const Quaternion deltaRotation =
        Quaternion::CreateFromAxisAngle(
            normalizedAxis,
            XMConvertToRadians(angle)
        );

    transform_.Rotate(deltaRotation);
}

void GameObject::SetMesh(const std::shared_ptr<Mesh>& mesh)
{
    auto* renderer = GetComponent<MeshRenderer>();

    if (!renderer)
        renderer = AddComponent<MeshRenderer>();

    renderer->SetMesh(mesh);
}

void GameObject::SetMaterial(const std::shared_ptr<Material>& material)
{
    auto* renderer = GetComponent<MeshRenderer>();

    if (!renderer)
        renderer = AddComponent<MeshRenderer>();

    renderer->SetMaterial(material);
}

void GameObject::OnCollision(const CollisionEvent& event)
{
    for (auto& component : components_)
    {
        if (component)
            component->OnCollision(event);
    }
}