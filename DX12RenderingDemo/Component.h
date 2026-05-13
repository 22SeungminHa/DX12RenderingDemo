#pragma once
#include "pch.h"

class GameObject;

class Component
{
public:
    virtual ~Component() = default;

    virtual void Awake() {}
    virtual void Update(float deltaTime) {}
    virtual void OnPrepareRender() {}

    void SetOwner(GameObject* owner) { owner_ = owner; }
    GameObject* GetOwner() const { return owner_; }

protected:
    GameObject* owner_ = nullptr;
};