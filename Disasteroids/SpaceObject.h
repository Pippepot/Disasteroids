#pragma once
#include "Entity.h"
class SpaceObject :
    public Entity
{
public:
	SpaceObject() = default;
	SpaceObject(olc::vf2d pos, olc::vf2d vel, float ang, std::vector<olc::vf2d> verts);
	bool ShapeOverlap_DIAGS_STATIC(SpaceObject& other);
	virtual void Update(float fElapsedTime) override;
	virtual float CalculateMass();
	olc::vf2d velocity;
	float angle;
	float mass;
	std::vector<olc::vf2d> vVerticies;
};

