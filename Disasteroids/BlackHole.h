#pragma once
#include "Entity.h"
#include "SpaceObject.h"

class BlackHole :
	public Entity
{
public:	
	BlackHole(olc::vf2d pos, float size, float duration);
	virtual void Update(float fElapsedTime) override;
	void Attract(SpaceObject& obj, float fElapsedTime);
	float GetSize();
	float GetRemainingLifeTime();

private:
	float maxSize;
	float currentSize;
	float duration;
	float timeLeft;
};

