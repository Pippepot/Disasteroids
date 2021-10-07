#include "Particle.h"

Particle::Particle(olc::vf2d pos, std::vector<olc::vf2d> verts, float duration, float force, olc::Pixel col) {
	position = pos;
	subParticles = verts;
	numStartParticles = verts.size();
	this->duration = duration;
	timeLeft = duration;
	forceFromOrigin = force;
	color = col;
}

void Particle::Update(float fElapsedTime)
{
	timeLeft -= fElapsedTime;
	if (timeLeft < 0) {
		bDead = true;
		return;
	}

	for (int i = 0; i < subParticles.size(); i++)
	{
		subParticles[i] += (subParticles[i] - position).norm() * forceFromOrigin * fElapsedTime;
	}

	int particlesToRemove = subParticles.size() - numStartParticles * (timeLeft / duration); // Particles removed in total

	if (particlesToRemove > 0)
		subParticles.erase(subParticles.begin(), subParticles.begin()+particlesToRemove);
}
