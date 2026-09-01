#pragma once
#include "Component.h"

class ColliderComponent : public Component
{
public:
    virtual ~ColliderComponent() = default;

    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

private:
    bool enabled_ = true;
};