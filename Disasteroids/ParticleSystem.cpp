#include "ParticleSystem.h"

void ParticleSystem::Update(float fElapsedTime)
{
	for (auto &p : particles)
	{
		p.Update(fElapsedTime);
	}
}

void ParticleSystem::AddParticlesFromVerts(std::vector<olc::vf2d> verts, olc::vf2d forceOrigin, float force, float duration, olc::Pixel color)
{
	for (int i = 0; i < verts.size(); i++)
	{
		olc::vf2d forceVector = (verts[i] - forceOrigin).norm() * force;
		AddParticleFromVerts(verts[i], forceVector, duration, color);
	}
}

void ParticleSystem::AddParticleFromVerts(olc::vf2d position, olc::vf2d force, float duration, olc::Pixel color)
{
	duration = duration + (float)rand() / RAND_MAX * 0.8f;
	AddParticle({ position, force, duration, color });
}

void ParticleSystem::AddParticle(Particle p) {
	particles[nextParticleIndex] = p;
	// Increment and wrap
	nextParticleIndex++;
	if (nextParticleIndex == particles.size())
		nextParticleIndex = 0;
}