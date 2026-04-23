#pragma once
#include "pch.h"

class Vertex
{
protected:
	Vector3 position_;
public:
	Vertex() { position_ = Vector3::Zero; }
	Vertex(const Vector3& xmf3Position) { position_ = xmf3Position; }
	~Vertex() {}
};

class DiffusedVertex : public Vertex
{
protected:
	Vector4 diffuse_;

public:
	DiffusedVertex() { position_ = Vector3::Zero; diffuse_ = Vector4::Zero; }
	DiffusedVertex(float x, float y, float z, const Vector4& xmf4Diffuse) { position_ = Vector3(x, y, z); diffuse_ = xmf4Diffuse; }
	DiffusedVertex(const Vector3& xmf3Position, const Vector4& xmf4Diffuse) { position_ = xmf3Position; diffuse_ = xmf4Diffuse; }
	~DiffusedVertex() {}
};