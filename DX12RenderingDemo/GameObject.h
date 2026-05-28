#pragma once
#include "pch.h"
#include "Transform.h"
#include "MeshRenderer.h"

class Material;
class Mesh;

class GameObject
{
public:
    GameObject();
    virtual ~GameObject();

    // Lifecycle
    virtual void Animate(float deltaTime);
    virtual void OnPrepareRender();

    // Hierarchy
    void AddChild(std::unique_ptr<GameObject> child);
    void RemoveChild(GameObject* child);

    const std::vector<std::unique_ptr<GameObject>>& GetChildren() const { return children_; }
    
    // Transform
    Transform* GetTransform() { return &transform_; }
    const Transform* GetTransform() const { return &transform_; }

    void SetPosition(const Vector3& position) { transform_.position = position; }
    void SetRotation(const Vector3& rotation) { transform_.rotation = rotation; }
    void SetScale(const Vector3& scale) { transform_.scale = scale; }

    void SetPosition(float x, float y, float z) { SetPosition(Vector3(x, y, z)); }
    void SetScale(float x, float y, float z) { SetScale(Vector3(x, y, z)); }

    void SetRotationDegrees(float pitch, float yaw, float roll)
    {
        SetRotation(Vector3(XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll)));
    }

    void Translate(const Vector3& offset) { transform_.position += offset; }
    void Rotate(const Vector3& axis, float angle);

    const Vector3& GetPosition() const { return transform_.position; }
    const Vector3& GetRotation() const { return transform_.rotation; }
    const Vector3& GetScale() const { return transform_.scale; }

    Matrix GetWorldMatrix() const { return transform_.GetWorldMatrix(); }

    // Rendering
    void SetObjectCBIndex(UINT index) { objectCBIndex_ = index; }
    UINT GetObjectCBIndex() const { return objectCBIndex_; }

    void SetMesh(const std::shared_ptr<Mesh>& mesh);
    void SetMaterial(const std::shared_ptr<Material>& material);

protected:
    Transform transform_;
    std::vector<std::unique_ptr<Component>> components_;
    std::vector<std::unique_ptr<GameObject>> children_;

    UINT objectCBIndex_ = 0;

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
