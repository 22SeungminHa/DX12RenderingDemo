#pragma once
#include "Scene.h"

class TestScene1 : public Scene {
public:
    TestScene1(UINT width, UINT height) : Scene(width, height) {};
    virtual ~TestScene1() = default;

    virtual SCENE_TYPE GetSceneType() const override { return SCENE_TYPE::TEST1; }
    virtual void OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
    virtual void SetupCameraDesc() override;
};

class TestScene2 : public Scene {
public:
    TestScene2(UINT width, UINT height) : Scene(width, height) {};
    virtual ~TestScene2() = default;

    virtual SCENE_TYPE GetSceneType() const override { return SCENE_TYPE::TEST2; }
    virtual void OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
    virtual void SetupCameraDesc() override;
};