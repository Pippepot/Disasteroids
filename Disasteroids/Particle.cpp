#include "Particle.h"

Particle::Particle(olc::vf2d pos, olc::vf2d force, float duration, olc::Pixel col) {
	position = pos;
	this->duration = duration;
	timeLeft = duration;
	velocity = force;
	color = col;
}

void Particle::Update(float fElapsedTime)
{
	if (isDead())
		return;

	position += velocity * fElapsedTime;

	timeLeft -= fElapsedTime;
	if (timeLeft < 0) {
		bDead = true;
	}
}
