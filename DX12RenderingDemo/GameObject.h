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

public:
    void AddChild(std::unique_ptr<GameObject> child);
    void RemoveChild(GameObject* child);

    void ReleaseUploadResources();

    void Rotate(const Vector3& axis, float angle);

    virtual void Animate(float deltaTime);
    virtual void OnPrepareRender();

    void SetObjectCBIndex(UINT index) { objectCBIndex_ = index; }

    UINT GetObjectCBIndex() const { return objectCBIndex_; }
    Matrix GetWorldMatrix() const { return transform_.GetWorldMatrix(); }
    Transform* GetTransform() { return &transform_; }
    const std::vector<std::unique_ptr<GameObject>>& GetChildren() const { return children_; }
    const Transform* GetTransform() const { return &transform_; }

protected:
    Transform transform_;
    std::vector<std::unique_ptr<Component>> components_;
    std::vector<std::unique_ptr<GameObject>> children_;

    UINT objectCBIndex_ = 0;

public:
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
