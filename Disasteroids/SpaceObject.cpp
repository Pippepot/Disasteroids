#include "SpaceObject.h"
#pragma warning( disable : 26451 )

SpaceObject::SpaceObject(olc::vf2d pos, olc::vf2d vel, float ang, std::vector<olc::vf2d> verts, olc::Pixel col)
{
	position = pos;
	velocity = vel;
	angle = ang;
	vRawVerticies = verts;
	int vertSize = verts.size();
	vWorldVerticies.resize(vertSize);
	bDead = false;
	CalculateMass();
	color = col;

	boundingCircleRadius = 0;

	for (int i = 0; i < vertSize; i++)
	{
		boundingCircleRadius = std::max(boundingCircleRadius, vRawVerticies[i].mag2());
	}
	boundingCircleRadius = std::sqrt(boundingCircleRadius);
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

	float cosAng = cosf(angle);
	float sinAng = sinf(angle);

	for (int i = 0; i < vRawVerticies.size(); i++)
	{
		olc::vf2d currentVert = {
			vRawVerticies[i].x * cosAng - vRawVerticies[i].y * sinAng + position.x,
			vRawVerticies[i].x * sinAng + vRawVerticies[i].y * cosAng + position.y
		};

		vWorldVerticies[i] = currentVert;
	}
}


// Use edge/diagonal intersections.
bool SpaceObject::ShapeOverlap_DIAGS_STATIC(SpaceObject& other)
{
	SpaceObject* poly1 = this;
	SpaceObject* poly2 = &other;

	// Broad phase
	float distance = poly1->boundingCircleRadius + poly2->boundingCircleRadius;
	bool closeEnough = false;
	for (int i = 0; i < poly1->vWorldPositions.size(); i++)
	{
		for (int j = 0; j < poly2->vWorldPositions.size(); j++)
		{
			if ((poly1->vWorldPositions[i] - poly2->vWorldPositions[j]).mag() < distance) {
				closeEnough = true;
				break;
			}
			
			if (closeEnough)
				break;
		}
	}

	if (!closeEnough)
		return false;

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
					// Works now I think
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

					if (!hasCollided)
						continue;
		
					olc::vf2d directionalDisplacement = displacement * (shape == 0 ? -0.5f : 0.5f);

					position += directionalDisplacement;

					other.position -= directionalDisplacement;
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

		//m1 = std::clamp(m1, -10.0f, 10.0f);
		//m2 = std::clamp(m2, -10.0f, 10.0f);
		float forceOnImpact = 0.9f; // How much force (velocity) is not lost on impact

		poly1->velocity.x = tx * dpTan1 + nx * m1 * forceOnImpact;
		poly1->velocity.y = ty * dpTan1 + ny * m1 * forceOnImpact;
		poly2->velocity.x = tx * dpTan2 + nx * m2 * forceOnImpact;
		poly2->velocity.y = ty * dpTan2 + ny * m2 * forceOnImpact;
	}

	// Can't overlap if static collision is resolved
	return hasCollided;
}

bool SpaceObject::ShapeOverlap_SAT_STATIC(SpaceObject& other)
{
	SpaceObject* poly1 = this;
	SpaceObject* poly2 = &other;

	// Broad phase
	float distance = poly1->boundingCircleRadius + poly2->boundingCircleRadius;
	bool closeEnough = false;
	for (int i = 0; i < poly1->vWorldPositions.size(); i++)
	{
		for (int j = 0; j < poly2->vWorldPositions.size(); j++)
		{
			if ((poly1->vWorldPositions[i] - poly2->vWorldPositions[j]).mag() < distance) {
				closeEnough = true;
				break;
			}

			if (closeEnough)
				break;
		}
	}

	if (!closeEnough)
		return false;

	bool hasCollided = false;
	float overlap = INFINITY;

	// For each polygon, for each subpolygon
	for (int shape = 0; shape < 2; shape++)
	{
		if (shape == 1)
		{
			poly1 = &other;
			poly2 = this;
		}

		for (int pw = 0; pw < poly1->vProcessedVerticies.size(); pw++)
		{
				for (int a = 0; a < poly1->vProcessedVerticies[pw].size(); a++)
				{
					int b = (a + 1) % poly1->vProcessedVerticies[pw].size();
					olc::vf2d axisProj = { -(poly1->vProcessedVerticies[pw][b].y - poly1->vProcessedVerticies[pw][a].y), poly1->vProcessedVerticies[pw][b].x - poly1->vProcessedVerticies[pw][a].x };

					// Optional normalisation of projection axis enhances stability slightly
					axisProj = axisProj.norm();

					// Work out min and max 1D points for r1
					float min_r1 = INFINITY, max_r1 = -INFINITY;
					for (int pw2 = 0; pw2 < poly1->vProcessedVerticies.size(); pw2++)
					{
						for (int p = 0; p < poly1->vProcessedVerticies[pw2].size(); p++)
						{
							float q = (poly1->vProcessedVerticies[pw2][p].x * axisProj.x + poly1->vProcessedVerticies[pw2][p].y * axisProj.y);
							min_r1 = std::min(min_r1, q);
							max_r1 = std::max(max_r1, q);
						}
					}

					// Work out min and max 1D points for r2
					float min_r2 = INFINITY, max_r2 = -INFINITY;
					for (int pw2 = 0; pw2 < poly2->vProcessedVerticies.size(); pw2++)
					{
						for (int p = 0; p < poly2->vProcessedVerticies[pw2].size(); p++)
						{
							float q = (poly2->vProcessedVerticies[pw2][p].x * axisProj.x + poly2->vProcessedVerticies[pw2][p].y * axisProj.y);
							min_r2 = std::min(min_r2, q);
							max_r2 = std::max(max_r2, q);
						}
					}

					// Calculate actual overlap along projected axis, and store the minimum
					overlap = std::min(std::min(max_r1, max_r2) - std::max(min_r1, min_r2), overlap);

					if (!(max_r2 >= min_r1 && max_r1 >= min_r2)) {
						hasCollided = false;
						break;
					}

					// TODO: fix this s***. has collided should be true only if all axes of one wrap are overlapping
					hasCollided = true;
				}
			
		}
	}

	if (!hasCollided)
		return false;

	// If we got here, the objects have collided, we will displace r1
	// by overlap along the vector between the two object centers
	olc::vf2d d = { other.position.x - position.x, other.position.y - position.y };
	float s = sqrtf(d.x * d.x + d.y * d.y);
	position.x -= overlap * d.x / s;
	position.y -= overlap * d.y / s;
	return true;
}

