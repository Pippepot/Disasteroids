#pragma once
#include "Particle.h"

class ParticleSystem
{
public:
	void Update(float fElapsedTime);
	void AddParticlesFromVerts(std::vector<olc::vf2d> verts, olc::vf2d forceOrigin, float force, float duration, olc::Pixel color);
	void AddParticleFromVerts(olc::vf2d position, olc::vf2d force, float duration, olc::Pixel color);
	void AddParticle(Particle p);
	std::array<Particle, 100> particles;

private:
	int nextParticleIndex;
};

