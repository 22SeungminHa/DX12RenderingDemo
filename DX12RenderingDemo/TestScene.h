#pragma once
#include "Scene.h"

class CTestScene1 : public CScene {
public:
    CTestScene1() = default;
    virtual ~CTestScene1() = default;

    virtual SCENE_TYPE GetSceneType() const override { return SCENE_TYPE::TEST1; }
    virtual void Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
};

class CTestScene2 : public CScene {
public:
    CTestScene2() = default;
    virtual ~CTestScene2() = default;

    virtual SCENE_TYPE GetSceneType() const override { return SCENE_TYPE::TEST2; }
    virtual void Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
};