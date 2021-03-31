#pragma once
#include "Entity.h"
class Laser :
    public Entity
{
public:
	Laser(olc::vf2d sPosition, olc::vf2d ePosition, olc::Pixel pixelColor, float duration);
	virtual void Update(float fElapsedTime) override;
	olc::vf2d endPosition;
	float maxTime;
	float timeLeft;

};

