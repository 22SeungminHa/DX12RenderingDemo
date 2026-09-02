#pragma once
#include "pch.h"

class GameObject;
class Scene;
class ColliderComponent;

struct CollisionEvent
{
    Scene* scene = nullptr;

    GameObject* self = nullptr;
    GameObject* other = nullptr;

    ColliderComponent* selfCollider = nullptr;
    ColliderComponent* otherCollider = nullptr;

    float hitT = 0.0f;
    Vector3 hitPoint = Vector3::Zero;
};

class Component
{
public:
    virtual ~Component() = default;

    virtual void Awake() {}
    virtual void Update(float deltaTime) {}
    virtual void OnPrepareRender() {}
    virtual void OnCollision(const CollisionEvent& event) {}

    void SetOwner(GameObject* owner) { owner_ = owner; }
    GameObject* GetOwner() const { return owner_; }

protected:
    GameObject* owner_ = nullptr;
};