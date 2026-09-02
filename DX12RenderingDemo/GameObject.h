#pragma once
#include "pch.h"
#include "Transform.h"
#include "MeshRenderer.h"

class Material;
class Mesh;

enum class ObjectType
{
    Default,

    Map,
    Obstacle,
    Crystal,
    Projectile,
    GlassFragment
};

class GameObject
{
public:
    GameObject();
    virtual ~GameObject();

    virtual void Animate(float deltaTime);
    virtual void OnPrepareRender();
    virtual void OnCollision(const CollisionEvent& event);

    void MarkForDestroy() { pendingDestroy_ = true; }
    bool IsPendingDestroy() const { return pendingDestroy_; }

    void RemovePendingDestroyChildren();

    // Hierarchy
    void AddChild(std::unique_ptr<GameObject> child);
    void RemoveChild(GameObject* child);

    const std::vector<std::unique_ptr<GameObject>>& GetChildren() const { return children_; }
    
    // Transform
    Transform* GetTransform() { return &transform_; }
    const Transform* GetTransform() const { return &transform_; }

    void SetPosition(const Vector3& position) { transform_.SetPosition(position); }
    void SetPosition(float x, float y, float z) { SetPosition(Vector3(x, y, z)); }
    void SetRotation(const Quaternion& rotation) { transform_.SetRotation(rotation); }

    void SetRotationDegrees(float pitch, float yaw, float roll)
    {
        transform_.SetRotationEuler(Vector3(XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll)));
    }
    void SetScale(const Vector3& scale) { transform_.SetScale(scale); }
    void SetScale(float x, float y, float z) { SetScale(Vector3(x, y, z)); }
    void Translate(const Vector3& offset) { transform_.Translate(offset); }
    void TranslateWorld(const Vector3& offset) { transform_.TranslateWorld(offset); }

    void Rotate(const Vector3& axis, float angle);
    void Rotate(const Quaternion& deltaRotation) { transform_.Rotate(deltaRotation); }

    const Vector3& GetPosition() const { return transform_.position; }
    const Quaternion& GetRotation() const { return transform_.rotation; }
    const Vector3& GetScale() const { return transform_.scale; }

    Vector3 GetWorldPosition() const { return Vector3::Transform(Vector3::Zero, GetWorldMatrix()); }
    Matrix GetWorldMatrix() const { return transform_.GetWorldMatrix(); }

    void SetObjectCBIndex(UINT index) { objectCBIndex_ = index; }
    UINT GetObjectCBIndex() const { return objectCBIndex_; }

    void SetObjectType(ObjectType type) { objectType_ = type; }
    ObjectType GetObjectType() const { return objectType_; }

    void SetMesh(const std::shared_ptr<Mesh>& mesh);
    void SetMaterial(const std::shared_ptr<Material>& material);

protected:
    Transform transform_;
    std::vector<std::unique_ptr<Component>> components_;
    std::vector<std::unique_ptr<GameObject>> children_;

    UINT objectCBIndex_ = 0;

    bool pendingDestroy_ = false;

    ObjectType objectType_ = ObjectType::Default;

public:
    // Components
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = component.get();
        ptr->SetOwner(this);
        components_.push_back(std::move(component));

        return ptr;
    }

    template<typename T>
    T* GetComponent()
    {
        for (auto& component : components_)
            if (auto casted = dynamic_cast<T*>(component.get()))
                return casted;

        return nullptr;
    }

    template<typename T>
    const T* GetComponent() const
    {
        for (const auto& component : components_)
            if (auto casted = dynamic_cast<const T*>(component.get()))
                return casted;

        return nullptr;
    }
};
