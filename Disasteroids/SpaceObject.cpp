#include "SpaceObject.h"
#pragma warning( disable : 26451 )

SpaceObject::SpaceObject(olc::vf2d pos, olc::vf2d vel, float ang, std::vector<olc::vf2d> verts, olc::Pixel col)
{
	position = pos;
	velocity = vel;
	angle = ang;
	vRawVerticies = verts;
	bDead = false;
	CalculateMass();
	color = col;
}

void SpaceObject::Update(float fElapsedTime)
{
	position += velocity * fElapsedTime;
}

void SpaceObject::Kill()
{
	bDead = true;
}

void SpaceObject::CalculateMass()
{
	mass = 0;
	for (auto& v : vRawVerticies)
	{
		mass += v.mag();
	}
	mass /= vRawVerticies.size();
}


void SpaceObject::CalculateVerticiesWorldSpace()
{
	vWorldVerticies.clear();

	for (auto& vert : vRawVerticies)
	{
		olc::vf2d currentVert;
		
		TransformVertexWorldSpace(vert, currentVert);

		vWorldVerticies.push_back(currentVert);
	}
}


void SpaceObject::TransformVertexWorldSpace(olc::vf2d i, olc::vf2d &o)
{
	o.x = i.x * cosf(angle) - i.y * sinf(angle);
	o.y = i.x * sinf(angle) + i.y * cosf(angle);
	o += position;
}


// Use edge/diagonal intersections.
bool SpaceObject::ShapeOverlap_DIAGS_STATIC(SpaceObject& other)
{
	SpaceObject* poly1 = this;
	SpaceObject* poly2 = &other;

	bool hasCollided = false;

	// For each polygon, for each subpolygon
	for (int shape = 0; shape < 2; shape++)
	{
		if (shape == 1)
		{
			poly1 = &other;
			poly2 = this;
		}

		for (int i1 = 0; i1 < poly1->vProcessedVerticies.size(); i1++)
		{
			for (int i2 = 0; i2 < poly2->vProcessedVerticies.size(); i2++)
			{
				// Check diagonals of this polygon...
				for (int p = 0; p < poly1->vProcessedVerticies[i1].size(); p++)
				{
					// This boy needs to be different for every wrap
					// This boy do be different forevery wrap now :) Still don't work tho
					olc::vf2d line_r1s = poly1->vWorldPositions[i1];
					olc::vf2d line_r1e = poly1->vProcessedVerticies[i1][p];

					olc::vf2d displacement = { 0,0 };

					// ...against edges of this polygon
					for (int q = 0; q < poly2->vProcessedVerticies[i2].size(); q++)
					{
						olc::vf2d line_r2s = poly2->vProcessedVerticies[i2][q];
						olc::vf2d line_r2e = poly2->vProcessedVerticies[i2][(q + 1) % poly2->vProcessedVerticies[i2].size()];

						// Standard "off the shelf" line segment intersection
						float h = (line_r2e.x - line_r2s.x) * (line_r1s.y - line_r1e.y) - (line_r1s.x - line_r1e.x) * (line_r2e.y - line_r2s.y);
						float t1 = ((line_r2s.y - line_r2e.y) * (line_r1s.x - line_r2s.x) + (line_r2e.x - line_r2s.x) * (line_r1s.y - line_r2s.y)) / h;
						float t2 = ((line_r1s.y - line_r1e.y) * (line_r1s.x - line_r2s.x) + (line_r1e.x - line_r1s.x) * (line_r1s.y - line_r2s.y)) / h;

						if (t1 >= 0.0f && t1 < 1.0f && t2 >= 0.0f && t2 < 1.0f)
						{
							displacement.x += (1.0f - t1) * (line_r1e.x - line_r1s.x);
							displacement.y += (1.0f - t1) * (line_r1e.y - line_r1s.y);
							hasCollided = true;
						}
					}

					position.x += displacement.x * (shape == 0 ? -1 : +1);
					position.y += displacement.y * (shape == 0 ? -1 : +1);
				}
			}
		}
	}

	if (hasCollided)
	{
		float fDistance = sqrtf((poly1->position.x - poly2->position.x) * (poly1->position.x - poly2->position.x) + (poly1->position.y - poly2->position.y) * (poly1->position.y - poly2->position.y));

		// Normal
		float nx = (poly2->position.x - poly1->position.x) / fDistance;
		float ny = (poly2->position.y - poly1->position.y) / fDistance;

		// Tangent
		float tx = -ny;
		float ty = nx;

		// Dot product tangent
		float dpTan1 = poly1->velocity.x * tx + poly1->velocity.y * ty;
		float dpTan2 = poly2->velocity.x * tx + poly2->velocity.y * ty;

		// Dot product Normal
		float dpNorm1 = poly1->velocity.x * nx + poly1->velocity.y * ny;
		float dpNorm2 = poly2->velocity.x * nx + poly2->velocity.y * ny;

		// Conservation of momentum in 1D
		float m1 = (dpNorm1 * (poly1->mass - poly2->mass) + 2.0f * poly2->mass * dpNorm2) / (poly1->mass + poly2->mass);
		float m2 = (dpNorm2 * (poly2->mass - poly1->mass) + 2.0f * poly1->mass * dpNorm1) / (poly1->mass + poly2->mass);

		poly1->velocity.x = tx * dpTan1 + nx * m1;
		poly1->velocity.y = ty * dpTan1 + ny * m1;
		poly2->velocity.x = tx * dpTan2 + nx * m2;
		poly2->velocity.y = ty * dpTan2 + ny * m2;
	}

	// Can't overlap if static collision is resolved
	return hasCollided;
}

