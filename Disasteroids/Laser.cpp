#include "Laser.h"

Laser::Laser(olc::vf2d sPosition, olc::vf2d ePosition, olc::Pixel pixelColor, float duration)
{
	position = sPosition;
	endPosition = ePosition;
	color = pixelColor;
	this->duration = duration;
	timeLeft = duration;
}

void Laser::Update(float fElapsedTime)
{
	timeLeft -= fElapsedTime;
	if (timeLeft < 0) {
		bDead = true;
		return;
	}

	color.a = timeLeft * 255 / duration;
}