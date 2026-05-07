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

class LitVertex : public Vertex
{
protected:
    Vector4 diffuse_;
    Vector3 normal_;
    Vector2 texCoord_;

public:
    LitVertex()
    {
        position_ = Vector3::Zero;
        diffuse_ = Vector4::One;
        normal_ = Vector3::Up;
        texCoord_ = Vector2::Zero;
    }

    LitVertex(
        const Vector3& position,
        const Vector4& diffuse,
        const Vector3& normal,
        const Vector2& texCoord)
    {
        position_ = position;
        diffuse_ = diffuse;
        normal_ = normal;
        texCoord_ = texCoord;
    }

    ~LitVertex() {}
};