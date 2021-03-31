#pragma once
#include "SpaceObject.h"
class Player :
    public SpaceObject
{
public:
    Player() = default;
    Player(olc::vf2d pos, olc::vf2d vel, float ang, std::vector<olc::vf2d> verts, olc::Pixel col);
    virtual void Update(float fElapsedTime) override;
};

