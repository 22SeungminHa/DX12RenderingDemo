#pragma once
#include "GameObject.h"

class Mesh;
class Material;
class GlassComponent;
class BoxColliderComponent;

class ObstacleObject : public GameObject
{
public:
    ObstacleObject();

    void Initialize(
        const std::shared_ptr<Mesh>& mesh,
        const std::shared_ptr<Material>& material,
        const Vector3& position,
        float width,
        float height,
        float depth
    );

    GlassComponent* GetGlassComponent() const { return glassComponent_; }
    BoxColliderComponent* GetCollider() const { return collider_; }

private:
    GlassComponent* glassComponent_ = nullptr;
    BoxColliderComponent* collider_ = nullptr;
};