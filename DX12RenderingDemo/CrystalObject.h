#pragma once
#include "GameObject.h"

class Scene;
class Mesh;
class Material;
class BoxColliderComponent;

struct FBXNodeData;

class CrystalObject : public GameObject
{
public:
    CrystalObject();

    void Initialize(
        const std::shared_ptr<Mesh>& mesh,
        const std::shared_ptr<Material>& material,
        const std::shared_ptr<FBXNodeData>& crashedModel,
        const Vector3& position,
        const Vector3& scale
    );

    void OnCollision(const CollisionEvent& event) override;

    bool Break(Scene& scene, const Vector3& impactPoint);
    bool IsBroken() const { return isBroken_; }

private:
    std::shared_ptr<Material> material_;
    std::shared_ptr<FBXNodeData> crashedModel_;

    BoxColliderComponent* collider_ = nullptr;

    bool isBroken_ = false;
};