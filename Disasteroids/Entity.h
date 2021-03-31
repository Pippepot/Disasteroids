#pragma once
#include "olcPixelGameEngine.h"

class Entity
{
public:
	virtual void Update(float fElapsedTime) = 0;
	bool isDead();
	olc::vf2d position;
	olc::Pixel color;

protected:
	bool bDead = false;
};

