#include "BlackHole.h"

BlackHole::BlackHole(olc::vf2d pos, float size, float duration)
{
	position = pos;
	maxSize = size;
	color = olc::DARK_MAGENTA;
	this->duration = duration;
	timeLeft = duration;
	currentSize = 0;
}

void BlackHole::Attract(SpaceObject& obj, float fElapsedTime)
{
	olc::vf2d dir = position - obj.position;
	float dirSize = std::max(dir.mag() - obj.boundingCircleRadius, 7.0f);
	obj.velocity += dir.norm() * 20 * currentSize * currentSize / (dirSize * dirSize) * fElapsedTime;
}

float BlackHole::GetSize()
{
	return currentSize;
}

float BlackHole::GetRemainingLifeTime()
{
	return timeLeft;
}

void BlackHole::Update(float fElapsedTime)
{
	timeLeft -= fElapsedTime;
	if (timeLeft < 0) {
		bDead = true;
		return;
	}

	// Formula for a curve that grows and stays at 1 until it shrinks to 0
	// Steepness determines the growth and decline of the curve
	float steepness = 1.5f;
	float x = timeLeft / duration;
	currentSize = maxSize * std::fmin((-((x - 0.5f) * (x - 0.5f)) * 4 * steepness + steepness), 1);
}
