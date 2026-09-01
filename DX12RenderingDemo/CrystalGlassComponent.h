#pragma once
#include "Component.h"

class Scene;
class Material;
struct FBXNodeData;

class CrystalGlassComponent : public Component
{
public:
    void Initialize(
        const std::shared_ptr<Material>& material,
        const std::shared_ptr<FBXNodeData>& crashedModel
    );

    bool Break(Scene& scene);

    bool IsBroken() const { return isBroken_; }

private:
    std::shared_ptr<Material> material_;
    std::shared_ptr<FBXNodeData> crashedModel_;

    bool isBroken_ = false;
};