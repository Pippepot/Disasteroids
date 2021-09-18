#pragma once
#include "Entity.h"
class SpaceObject :
    public Entity
{
public:
	SpaceObject() = default;
	SpaceObject(olc::vf2d pos, olc::vf2d vel, float ang, std::vector<olc::vf2d> verts);
	bool ShapeOverlap_DIAGS_STATIC(SpaceObject& other);
	virtual void Update(float fElapsedTime) override;
	float CalculateMass();
	void CalculateVerticiesWorldSpace();
	olc::vf2d velocity;
	float angle;
	float mass;
	std::vector<olc::vf2d> vRawVerticies;
	std::vector<olc::vf2d> vWorldVerticies;
	std::vector<std::vector<olc::vf2d>> vProcessedVerticies;
	// Processed verts have the same indicies as raw verticies but with blanks
private:
	void TransformVertexWorldSpace(olc::vf2d i, olc::vf2d& o);
};

