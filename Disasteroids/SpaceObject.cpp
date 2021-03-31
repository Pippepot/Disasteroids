#include "SpaceObject.h"

SpaceObject::SpaceObject(olc::vf2d pos, olc::vf2d vel, float ang, std::vector<olc::vf2d> verts, olc::Pixel col)
{
	position = pos;
	velocity = vel;
	angle = ang;
	vVerticies = verts;
	color = col;
	bDead = false;
}

void SpaceObject::Update(float fElapsedTime)
{
	position += velocity * fElapsedTime;
}
