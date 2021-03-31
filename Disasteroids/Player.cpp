#include "Player.h"

Player::Player(olc::vf2d pos, olc::vf2d vel, float ang, std::vector<olc::vf2d> verts, olc::Pixel col) : SpaceObject::SpaceObject(pos, vel, ang, verts, col) { }

void Player::Update(float fElapsedTime)
{
	SpaceObject::Update(fElapsedTime);
}
