#pragma once
#include "Entity.h"
class Particle :
    public Entity
{
public:
    Particle(olc::vf2d pos, std::vector<olc::vf2d> verts, float duration, float force, olc::Pixel col);
    virtual void Update(float fElapsedTime) override;
    std::vector<olc::vf2d> subParticles;
    int numStartParticles;
    float forceFromOrigin;
    float duration;
private:
    float timeLeft;
};

