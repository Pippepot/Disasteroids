#pragma once
#include "Entity.h"
class SpaceObject :
    public Entity
{
public:
	SpaceObject() = default;
	SpaceObject(olc::vf2d pos, olc::vf2d vel, float ang, std::vector<olc::vf2d> verts, olc::Pixel col);
	virtual void Update(float fElapsedTime) override;
	olc::vf2d velocity;
	float angle;
	std::vector<olc::vf2d> vVerticies;
};

