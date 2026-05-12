#pragma once
#include "pch.h"

class Asset
{
public:
    Asset() = default;
    virtual ~Asset() = default;

    const std::string& GetKey() const { return key_; }
    void SetKey(const std::string& key) { key_ = key; }

private:
    std::string key_;
};
