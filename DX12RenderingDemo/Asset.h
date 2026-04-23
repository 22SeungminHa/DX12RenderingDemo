#pragma once
#include "pch.h"

class Asset
{
public:
    Asset() = default;
    virtual ~Asset() = default;

    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }

private:
    std::string name_;
};
