#include "SpaceObject.h"
#pragma warning( disable : 26451 )

SpaceObject::SpaceObject(olc::vf2d pos, olc::vf2d vel, float ang, std::vector<olc::vf2d> verts)
{
	position = pos;
	velocity = vel;
	angle = ang;
	vRawVerticies = verts;
	bDead = false;
	mass = CalculateMass();
}

void SpaceObject::Update(float fElapsedTime)
{
	position += velocity * fElapsedTime;
}

float SpaceObject::CalculateMass()
{
	float m = 0;
	for (auto& v : vRawVerticies)
	{
		m += v.mag();
	}
	m /= vRawVerticies.size();
	color = m > 8 ? olc::YELLOW : olc::GREY;
	return m;
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
					olc::vf2d line_r1s = poly1->position;
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
						}
					}

					position.x += displacement.x * (shape == 0 ? -1 : +1);
					position.y += displacement.y * (shape == 0 ? -1 : +1);
				}
			}
		}
	}

	// Cant overlap if static collision is resolved
	return false;
}

