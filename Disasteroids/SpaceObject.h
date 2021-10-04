#pragma once
#include "Entity.h"
class SpaceObject :
    public Entity
{
public:
	SpaceObject() = default;
	SpaceObject(olc::vf2d pos, olc::vf2d vel, float ang, std::vector<olc::vf2d> verts, olc::Pixel col);
	bool ShapeOverlap_DIAGS_STATIC(SpaceObject& other);
	virtual void Update(float fElapsedTime) override;
	void Kill();
	void CalculateMass();
	void CalculateVerticiesWorldSpace();
	olc::vf2d velocity;
	float angle;
	float mass;
	std::vector<olc::vf2d> vRawVerticies;	// Verticies in local space
	std::vector<olc::vf2d> vWorldVerticies;	// Verticies in world space
	std::vector<olc::vf2d> vWorldPositions;	// Asteroids position with different wraps. Same indices as vProcessedVVerticies
	std::vector<std::vector<olc::vf2d>> vProcessedVerticies; // Verticies in world space wrapped to match the visuals
	std::vector<int> vProcessedVerticiesRawIndicies; // Original indicies of processed verticies.
private:
	void TransformVertexWorldSpace(olc::vf2d i, olc::vf2d& o);
};

