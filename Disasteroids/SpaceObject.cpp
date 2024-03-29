#include "SpaceObject.h"
#pragma warning( disable : 26451 )

SpaceObject::SpaceObject(olc::vf2d pos, olc::vf2d vel, float ang, float angVel, std::vector<olc::vf2d> verts, olc::Pixel col)
{
	position = pos;
	velocity = vel;
	angle = ang;
	angularVelocity = angVel;
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
	angle += angularVelocity * fElapsedTime;
}

void SpaceObject::Kill()
{
	bDead = true;
}

void SpaceObject::CalculateMass()
{
	mass = 0.0f;
	inertiaTensor = 0.0f;
	olc::vf2d centroid = olc::vf2d();

	const float k_inv3 = 1.0f / 3.0f;

	olc::vf2d edge1 = vRawVerticies[vRawVerticies.size() - 1];
	for (int i = 0; i < vRawVerticies.size(); ++i)
	{
		olc::vf2d edge2 = vRawVerticies[i];

		float D = abs(edge1.cross(edge2));

		float triangleArea = 0.5f * D;
		mass += triangleArea;

		centroid += triangleArea * k_inv3 * (edge1 + edge2);

		float intx2 = edge1.x * edge1.x + edge2.x * edge1.x + edge2.x * edge2.x;
		float inty2 = edge1.y * edge1.y + edge2.y * edge1.y + edge2.y * edge2.y;

		inertiaTensor += (0.25f * k_inv3 * D) * (intx2 + inty2);

		edge1 = edge2;
	}

	centroid *= 1.0f / mass;
	position += centroid;

	for (int i = 0; i < vRawVerticies.size(); ++i)
	{
		vRawVerticies[i] -= centroid;
	}
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

	olc::vf2d normal = olc::vd2d();
	olc::vf2d contactPoint = olc::vd2d();

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
				int poly2Size = poly2->vProcessedVerticies[i2].size();

				// Check diagonals of this polygon...
				for (int p = 0; p < poly1->vProcessedVerticies[i1].size(); p++)
				{
					olc::vf2d line_r1s = poly1->vWorldPositions[i1];
					olc::vf2d line_r1e = poly1->vProcessedVerticies[i1][p];

					olc::vf2d displacement = { 0,0 };

					// ...against edges of this polygon
					for (int q = 0; q < poly2->vProcessedVerticies[i2].size(); q++)
					{
						olc::vf2d line_r2s = poly2->vProcessedVerticies[i2][q];
						olc::vf2d line_r2e = poly2->vProcessedVerticies[i2][(q + 1) % poly2Size];

						// Standard "off the shelf" line segment intersection
						float h = (line_r2e.x - line_r2s.x) * (line_r1s.y - line_r1e.y) - (line_r1s.x - line_r1e.x) * (line_r2e.y - line_r2s.y);
						float t1 = ((line_r2s.y - line_r2e.y) * (line_r1s.x - line_r2s.x) + (line_r2e.x - line_r2s.x) * (line_r1s.y - line_r2s.y)) / h;
						float t2 = ((line_r1s.y - line_r1e.y) * (line_r1s.x - line_r2s.x) + (line_r1e.x - line_r1s.x) * (line_r1s.y - line_r2s.y)) / h;

						if (t1 >= 0.0f && t1 < 1.0f && t2 >= 0.0f && t2 < 1.0f)
						{
							displacement.x += (1.0f - t1) * (line_r1e.x - line_r1s.x);
							displacement.y += (1.0f - t1) * (line_r1e.y - line_r1s.y);

							contactPoint = line_r1e + displacement;
							normal = line_r2e - line_r2s;
							float tempY = normal.y;
							normal.y = -normal.x;
							normal.x = tempY;
							normal = normal.norm();

							hasCollided = true;
						}
					}

					if (!hasCollided)
						continue;
		
					olc::vf2d directionalDisplacement = displacement * (shape == 0 ? -0.5f : 0.5f);

					position += directionalDisplacement;

					other.position -= directionalDisplacement;

					if (hasCollided)
					{
						// Collision impulse solver 
						// https://www.myphysicslab.com/engine2D/collision-en.html#resting_contact

						float invMass1 = 1 / poly1->mass;
						float invMass2 = 1 / poly2->mass;
						float invInertia1 = 1 / poly1->inertiaTensor;
						float invInertia2 = 1 / poly2->inertiaTensor;

						olc::vf2d rap = contactPoint - poly1->vWorldPositions[i1];
						olc::vf2d rbp = contactPoint - poly2->vWorldPositions[i2];

						olc::vf2d vap = poly1->velocity + olc::vf2d(-poly1->angularVelocity * rap.y, poly1->angularVelocity * rap.x);
						olc::vf2d vbp = poly2->velocity + olc::vf2d(-poly2->angularVelocity * rbp.y, poly2->angularVelocity * rbp.x);

						olc::vf2d vab = vap - vbp;

						const float elasticity = 0.9f;
						float divisor = invMass1 + invMass2 + rap.cross(normal) * rap.cross(normal) * invInertia1 + rbp.cross(normal) * rbp.cross(normal) * invInertia2;
						float j = -(1 + elasticity) * vab.dot(normal) / divisor;

						poly1->velocity += j * normal * invMass1;
						poly2->velocity -= j * normal * invMass2;

						poly1->angularVelocity += rap.cross(j * normal) * invInertia1;
						poly2->angularVelocity -= rbp.cross(j * normal) * invInertia2;
					}
				}
			}
		}
	}



	return hasCollided;
}