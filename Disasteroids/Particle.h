#pragma once
#include "Entity.h"
class Particle :
    public Entity
{
public:
    Particle() = default;
    Particle(olc::vf2d pos, olc::vf2d force, float duration, olc::Pixel col);
    virtual void Update(float fElapsedTime) override;
    olc::vf2d velocity;
    float duration;
private:
    float timeLeft;
};

