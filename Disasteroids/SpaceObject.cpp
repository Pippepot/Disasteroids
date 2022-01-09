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
	// Polygon mass, centroid, and inertia.
	// Let rho be the polygon density in mass per unit area.
	// Then:
	// mass = rho * int(dA)
	// centroid.x = (1/mass) * rho * int(x * dA)
	// centroid.y = (1/mass) * rho * int(y * dA)
	// I = rho * int((x*x + y*y) * dA)
	//
	// We can compute these integrals by summing all the integrals
	// for each triangle of the polygon. To evaluate the integral
	// for a single triangle, we make a change of variables to
	// the (u,v) coordinates of the triangle:
	// x = x0 + e1x * u + e2x * v
	// y = y0 + e1y * u + e2y * v
	// where 0 <= u && 0 <= v && u + v <= 1.
	//
	// We integrate u from [0,1-v] and then v from [0,1].
	// We also need to use the Jacobian of the transformation:
	// D = cross(e1, e2)
	//
	// Simplification: triangle centroid = (1/3) * (p1 + p2 + p3)
	//
	// The rest of the derivation is handled by computer algebra.
	mass = 0.0f;
	inertiaTensor = 0.0f;

	// Get a reference point for forming triangles.
	// Use the first vertex to reduce round-off errors.
	olc::vf2d s = vRawVerticies[0];

	const float k_inv3 = 1.0f / 3.0f;

	for (int i = 0; i < vRawVerticies.size(); ++i)
	{
		// Triangle vertices.
		olc::vf2d e1 = vRawVerticies[i] - s;
		olc::vf2d e2 = i + 1 < vRawVerticies.size() ? vRawVerticies[i + 1] - s : vRawVerticies[0] - s;

		float D = abs(e1.cross(e2));

		float triangleArea = 0.5f * D;
		mass += triangleArea;

		float ex1 = e1.x, ey1 = e1.y;
		float ex2 = e2.x, ey2 = e2.y;

		float intx2 = ex1 * ex1 + ex2 * ex1 + ex2 * ex2;
		float inty2 = ey1 * ey1 + ey2 * ey1 + ey2 * ey2;

		inertiaTensor += (0.25f * k_inv3 * D) * (intx2 + inty2);
	}

	// Inertia tensor relative to the local origin (point s).
	
	//mass = 0;
	//for (auto& v : vRawVerticies)
	//{
	//	mass += v.mag();
	//}
	//mass /= vRawVerticies.size();
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