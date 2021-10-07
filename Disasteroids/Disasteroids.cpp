#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "SpaceObject.h"
#include "Particle.h"
#include "Laser.h"
using namespace std;


// Override base class with your custom functionality
class Disasteroids : public olc::PixelGameEngine
{
public:
	Disasteroids()
	{
		// Name you application
		sAppName = "Disasteroids";
	}

private:
	const int nAsteroidSize = 16;
	const float nAsteroidBreakMass = 8.0f;
	const float nLevelSwitchDelay = 5.0f;
	int level;
	vector<SpaceObject> vecAsteroids;
	vector<Laser> vecLasers;
	vector<Particle> vecParticles;
	SpaceObject player;
	int nScore;

	vector<olc::vf2d> vecModelAsteroid;

	bool IsPointInsideCircle(olc::vf2d c, float radius, olc::vf2d p)
	{
		return (p - c).mag() < radius;
	}

	// Implements "wrap around" for various in-game sytems
	void WrapCoordinates(float ix, float iy, float& ox, float& oy)
	{
		ox = ix;
		oy = iy;
		if (ix < 0.0f)	ox = ix + (float)ScreenWidth();
		if (ix >= (float)ScreenWidth())	ox = ix - (float)ScreenWidth();
		if (iy < 0.0f)	oy = iy + (float)ScreenHeight();
		if (iy >= (float)ScreenHeight()) oy = iy - (float)ScreenHeight();
	}

	void WrapCoordinates(olc::vf2d i, olc::vf2d& o)
	{
		WrapCoordinates(i.x, i.y, o.x, o.y);
	}

	virtual bool Draw(int32_t x, int32_t y, olc::Pixel p = olc::WHITE)
	{
		float ox = x;
		float oy = y;
		WrapCoordinates((float)x, (float)y, ox, oy);
		return PixelGameEngine::Draw(ox, oy, p);
	}

public:
	bool OnUserCreate() override
	{

		int verts = 26;
		for (int i = 0; i < verts; i++)
		{
			// Counter clockwise
			float noise = (float)rand() / (float)RAND_MAX * 0.6f + 1.6f;
			vecModelAsteroid.push_back({
				noise * sinf(((float)i / (float)verts) * 6.28318f) * nAsteroidSize,
				noise * cosf(((float)i / (float)verts) * 6.28318f) * nAsteroidSize
				});
		}

		ResetGame();

		return true;
	}

	void ResetGame()
	{
		vecAsteroids.clear();
		vecLasers.clear();

		//vecAsteroids.push_back({ {300, 300}, {-20.0f, 60.0f}, 1.1f, vecModelAsteroid, olc::YELLOW});
		//vecAsteroids.push_back({ {ScreenWidth() * 0.5f, ScreenHeight() * 0.1f}, {5.0f, -25.0f}, 0.0f, vecModelAsteroid, olc::YELLOW });
		player = SpaceObject(olc::vf2d(ScreenWidth() * 0.5f, ScreenHeight() * 0.5f),
			olc::vf2d(0, -40),
			0.0f,
			{
			{ 0.0f, -5.5f },
			{ -2.5f, 2.5f },
			{ 0.0f, -1.0f },
			{ 2.5f, 2.5f }
			},
			olc::WHITE);
		
		SpawnAsteroids(1);
		nScore = 0;
		level = 1;

	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		if (player.isDead()) {
			vecParticles.push_back({ player.position, player.vWorldVerticies, 3, 10, player.color });
			ResetGame();
		}

		Clear(olc::BLACK);


		// Steer player
		if (GetKey(olc::RIGHT).bHeld || GetKey(olc::D).bHeld)
			player.angle += 4.0f * fElapsedTime;
		if (GetKey(olc::LEFT).bHeld || GetKey(olc::A).bHeld)
			player.angle -= 4.0f * fElapsedTime;

		// Thrust / Acceleration
		if (GetKey(olc::UP).bHeld || GetKey(olc::W).bHeld)
			player.velocity += olc::vf2d(sin(player.angle), -cos(player.angle)) * 20.0 * fElapsedTime;

		// Shoot laser
		if (GetKey(olc::SPACE).bPressed)
			ShootLaser();

		UpdateEntities(fElapsedTime);

		CheckWinCondition();

		return true;
	}

	void UpdateEntities(float fElapsedTime)
	{
		DestroyDeadEntities();

		player.Update(fElapsedTime);
		player.CalculateVerticiesWorldSpace();
		ProcessVerticiesForCollision(player);
		WrapCoordinates(player.position, player.position);

		// Update asteroids position and velocity
		for (auto& a : vecAsteroids)
		{
			a.Update(fElapsedTime * 0.2f);
			a.angle += 0.1f * fElapsedTime;
			WrapCoordinates(a.position, a.position);
			a.CalculateVerticiesWorldSpace();
			ProcessVerticiesForCollision(a);
		}

		std::vector<SpaceObject*> collidingObjects;

		// Check for overlap
		for (int m = 0; m < vecAsteroids.size(); m++)
		{

			if (vecAsteroids[m].ShapeOverlap_DIAGS_STATIC(player)) {
				// Hit a big asteroid. Game over
				if (vecAsteroids[m].mass >= nAsteroidBreakMass) {
					player.Kill();
					return;
				}

				nScore += vecAsteroids[m].mass;
				vecAsteroids[m].Kill();	
			}

			for (int n = m + 1; n < vecAsteroids.size(); n++)
			{
				if (vecAsteroids[m].ShapeOverlap_DIAGS_STATIC(vecAsteroids[n]))
					collidingObjects.push_back(&vecAsteroids[n]);
			}
		}

		// TODO Fix multiple collisions. Pinching
		
		//while (collidingObjects.size() > 0)
		//{
		//	for (int i = 0; i < vecAsteroids.size(); i++)
		//	{
		//		if (collidingObjects[0] == &vecAsteroids[i])
		//			continue;
		//
		//		if (collidingObjects[0]->ShapeOverlap_DIAGS_STATIC(vecAsteroids[i])) {
		//			collidingObjects.push_back(&vecAsteroids[i]);
		//			collidingObjects.push_back(collidingObjects[0]);
		//		}
		//
		//		collidingObjects.erase(collidingObjects.begin());
		//	}
		//
		//}

		for (auto& l : vecLasers)
			l.Update(fElapsedTime);

		for (auto& p : vecParticles)
			p.Update(fElapsedTime);

		DrawEntities();
	}

	void DestroyDeadEntities()
	{
		// Remove lasers with no time left
		if (vecLasers.size() > 0)
		{
			auto i = remove_if(vecLasers.begin(), vecLasers.end(), [&](Laser o) { return (o.isDead()); });
			if (i != vecLasers.end())
				vecLasers.erase(i);
		}

		// Remove particles
		if (vecParticles.size() > 0)
		{
			auto i = remove_if(vecParticles.begin(), vecParticles.end(), [&](Particle o) { return (o.isDead()); });
			if (i != vecParticles.end())
				vecParticles.erase(i);
		}

		// Remove asteroids
		if (vecAsteroids.size() > 0)
		{
			auto i = find_if(vecAsteroids.begin(), vecAsteroids.end(), [&](SpaceObject o) { return (o.isDead()); });
			if (i != vecAsteroids.end()) {
				SpaceObject a = vecAsteroids[i - vecAsteroids.begin()];
				vecParticles.push_back({ a.position, a.vWorldVerticies, 3, 10, a.color });
				vecAsteroids.erase(i);
			}

		}

	}

	void DrawEntities()
	{
		// Draw asteroids
		for (auto& a : vecAsteroids)
			DrawWireFrameModel(a.vWorldVerticies, olc::vf2d(), 0, 1, a.color);
	
		// Draw lasers
		for (auto& l : vecLasers)
			DrawLine(l.position, l.endPosition, l.color);

		// Draw lasers
		for (auto& p : vecParticles)
			for (auto& i : p.subParticles)
				Draw(i.x, i.y, p.color);

		// Draw player
		DrawWireFrameModel(player.vRawVerticies, player.position, player.angle, 1, player.color);

		// Draw score
		DrawString(2, 2, "SCORE: " + to_string(nScore));
	}

	void CheckWinCondition()
	{
		// Level Clear
		if (find_if(vecAsteroids.begin(), vecAsteroids.end(), [&](SpaceObject o) { return (o.mass > nAsteroidBreakMass); }) == vecAsteroids.end())
		{
			// TODO: Make timer
			if (vecAsteroids.size() > 0) {
				nScore += vecAsteroids[0].mass;
				vecAsteroids[0].Kill();
				return;
			}

			vecAsteroids.clear();
			vecLasers.clear();

			level++;
			
			SpawnAsteroids(min(level, 4));
		}
	}

	// TODO: Fix. spawn in semicircle around player
	void SpawnAsteroids(int amount) {
		
		float radius = 10 * amount + nAsteroidSize * 2;

		float fLength = player.velocity.mag();

		float vxUnit = player.velocity.x / fLength;
		float vyUnit = player.velocity.y / fLength;

		float angleOffset = acos(vxUnit) - 3.14f * 0.5f;
		if (vyUnit > 0)
			angleOffset = 3.14f - angleOffset;

		for (int i = 0; i < amount; i++)
		{
			float angle = 0;
			if (amount > 1)
				angle = (i / ((float)amount - 1.0f)) * 3.14f;

			angle -= angleOffset;

			float xPos = player.position.x + cos(angle) * radius;
			float yPos = player.position.y + sin(angle) * radius;

			vecAsteroids.push_back({ {xPos,
					yPos},
					{40.0f * vyUnit * level, 40.0f * vxUnit * level},
					0.0f, vecModelAsteroid,
					olc::YELLOW });
		}
	}

	void ShootLaser()
	{
		vector<SpaceObject> newAsteroids;

		// TODO: Implement new processed verticies way of doing things man

		olc::vf2d vEndPos;
		CalculateLineEndPosition(player.angle, player.position, vEndPos);

		// Old model. No good!
		// 1 For every asteroid
		// 2 Find last and current vertex
		// 3 Rotate and wrap them
		// 4 Wrap the other vertex or current vertex based on where the laser ends if they don't have the same wrap. pfew!
		// 5 Check if lazer intersects the line between the verts
		// if only hit one line
		// 6 Wrap laser around
		// 7 Repeat step 2 through 5 with the wrapped laser
		// 8 Make new verts
		// 9 Stich asteroids together from new verts
		// 10 Two astoroids are built!
		//
		// New model. Better!
		// For every asteroid
		// Check hits against every transformed vertex
		// Translate transformed verticies to raw verticies
		//  

		// For every asteroid
		for (auto& a : vecAsteroids)
		{
			// TODO: Doesn't work when wrapping. oof
			// For every list of processed verticies
			olc::vf2d firstIntersection;
			olc::vf2d secondIntersection;
			int firstIntersectionIndex = -1;
			int secondIntersectionIndex = -1;

			for (int listIndex = 0; listIndex < a.vProcessedVerticies.size(); listIndex++)
			{
				if (firstIntersectionIndex != -1)
					break;

				// For every processed vertex. Check if there is an intersection and where it is
				for (int vertIndex = 0; vertIndex < a.vProcessedVerticies[listIndex].size() - 1; vertIndex++)
				{
					// Possibly do some optimization. AABB
					if (LineLineIntersect(a.vProcessedVerticies[listIndex][vertIndex], a.vProcessedVerticies[listIndex][vertIndex + 1], player.position, vEndPos, firstIntersection)) {
						// Intersection is in wrapped world space. Transform to world space
						firstIntersection += a.position - a.vWorldPositions[listIndex];
						firstIntersectionIndex = a.vProcessedVerticiesRawIndicies[listIndex] + vertIndex;
						break;
					}
				}

			}

			// Laser doesn't hit the asteroid
			if (firstIntersectionIndex == -1)
				continue;

			for (int listIndex = 0; listIndex < a.vProcessedVerticies.size(); listIndex++)
			{
				if (secondIntersectionIndex != -1)
					break;

				// Find second intersection
				for (int vertIndex = 0; vertIndex < a.vWorldVerticies.size(); vertIndex++)
				{
					if (vertIndex + a.vProcessedVerticiesRawIndicies[listIndex] == firstIntersectionIndex)
						continue;

					olc::vf2d lineDirection = vEndPos - player.position;
					lineDirection = lineDirection.norm();
					olc::vf2d vNewEndPos = firstIntersection + lineDirection * 1000;
					olc::vf2d vNewStartPos = firstIntersection - lineDirection * 1000;
					// Possibly do some optimization. AABB
					if (LineLineIntersect(a.vWorldVerticies[vertIndex], a.vWorldVerticies[(vertIndex + 1) % a.vWorldVerticies.size()], vNewStartPos, vNewEndPos, secondIntersection)) {
						secondIntersectionIndex = a.vProcessedVerticiesRawIndicies[listIndex] + vertIndex;
						break;
					}
				}
			}

			// Hit both. Slice asteroid
			if (secondIntersectionIndex == -1)
				continue;

			olc::vf2d averageVertexPosition1;
			olc::vf2d averageVertexPosition2;
			vector<olc::vf2d> vVertices1;
			vector<olc::vf2d> vVertices2;

			// Swap. Because it works if we do.
			if (firstIntersectionIndex > secondIntersectionIndex) {
				int tempIndex = firstIntersectionIndex;
				firstIntersectionIndex = secondIntersectionIndex;
				secondIntersectionIndex = tempIndex;

				olc::vf2d tempVert = firstIntersection;
				firstIntersection = secondIntersection;
				secondIntersection = tempVert;
			}

			for (int i = 0; i < a.vWorldVerticies.size(); i++)
			{
				if (i > firstIntersectionIndex && i <= secondIntersectionIndex)
				{
					vVertices1.push_back(a.vWorldVerticies[i]);

					if (i == secondIntersectionIndex)
					{
						vVertices1.push_back(secondIntersection);
						vVertices2.push_back(secondIntersection);
					}
				}
				else
				{
					vVertices2.push_back(a.vWorldVerticies[i]);

					if (i == firstIntersectionIndex)
					{
						vVertices1.push_back(firstIntersection);
						vVertices2.push_back(firstIntersection);
					}
				}
			}

			// Make new center
			for (auto& v : vVertices1)
			{
				averageVertexPosition1 += v;
			}

			for (auto& v : vVertices2)
			{
				averageVertexPosition2 += v;
			}

			averageVertexPosition1 /= vVertices1.size();
			averageVertexPosition2 /= vVertices2.size();

			for (auto& v : vVertices1)
			{
				v -= averageVertexPosition1;
			}

			for (auto& v : vVertices2)
			{
				v -= averageVertexPosition2;
			}

			olc::vf2d newVelocity1 = 0.5f * (averageVertexPosition1 - averageVertexPosition2) + a.velocity;
			olc::vf2d newVelocity2 = 0.5f * (averageVertexPosition2 - averageVertexPosition1) + a.velocity;

			SpaceObject newAsteroid = { averageVertexPosition2, newVelocity2, 0, vVertices2, olc::YELLOW };

			if (newAsteroid.mass <= nAsteroidBreakMass)
				newAsteroid.color = olc::GREY;

			newAsteroids.push_back(newAsteroid);


			a.position = averageVertexPosition1;
			a.angle = 0;
			a.velocity = newVelocity1;
			a.vRawVerticies = vVertices1;
			a.CalculateMass();

			if (a.mass <= nAsteroidBreakMass)
				a.color = olc::GREY;
		}

		vecLasers.push_back({ player.position, vEndPos, olc::BLUE, 1 });

		// Create the newly added asteroids
		for (auto& a : newAsteroids)
			vecAsteroids.push_back(a);

		// Terrible fucken code
		/*
		// Intersect with asteroids
		for (auto& a : vecAsteroids)
		{
			vector<olc::vf2d> vIntersections;
			vector<int> vIntersectingVerticiesIndicies;
			// Loop through every vertex on the asteroid and locate the intersecting verticies
			int lastVertexIndex = a.vRawVerticies.size() - 1;
			olc::vf2d currentVertexWrapped;
			olc::vf2d lastVertexWrapped;
			olc::vf2d offset;
			olc::vf2d currentVertTest;
			olc::vf2d lastVertTest;
			bool useTempVerts = false;
			bool isWrapped;
			bool lastWrapped;
			bool sameWrapAsLaser;
			bool intersectionWrapped;

			olc::vf2d lastVertex;
			lastVertex.x = a.vRawVerticies.back().x * cosf(a.angle) - a.vRawVerticies.back().y * sinf(a.angle);
			lastVertex.y = a.vRawVerticies.back().x * sinf(a.angle) + a.vRawVerticies.back().y * cosf(a.angle);
			lastVertex += a.position;

			WrapCoordinates(lastVertex, lastVertexWrapped);

			for (int i = 0; i < a.vRawVerticies.size(); i++)
			{
				olc::vf2d currentVertex;
				currentVertex.x = a.vRawVerticies[i].x * cosf(a.angle) - a.vRawVerticies[i].y * sinf(a.angle);
				currentVertex.y = a.vRawVerticies[i].x * sinf(a.angle) + a.vRawVerticies[i].y * cosf(a.angle);
				currentVertex += a.position;

				WrapCoordinates(currentVertex, currentVertexWrapped);
				isWrapped = currentVertex != currentVertexWrapped;

				offset = currentVertexWrapped - currentVertex;

				currentVertTest = currentVertexWrapped;
				lastVertTest = lastVertexWrapped;

				// Check if the vertecies are not both wrapped
				if ((olc::vi2d)(lastVertexWrapped - currentVertexWrapped) != (olc::vi2d)(lastVertex - currentVertex))
				{
					// If the currentvertex is wrapped, make temporary vertex
					// Else make a temporary vertex out of the last vertex
					sameWrapAsLaser = (currentVertexWrapped - vEndPos).mag() < ScreenHeight() * 0.5f;
					if (sameWrapAsLaser)
					{
						// same wap move last vertex
						// different wrap move current vertex
						olc::vf2d vOffset = currentVertexWrapped - currentVertex;
						olc::vf2d lastVertCurrentWrap = lastVertex + vOffset;

						olc::vf2d currentIntersection;

						// Linelineintersect between edges of screen and two offset vertices
						LineScreenIntersect(currentVertexWrapped, lastVertCurrentWrap, currentIntersection);
						lastVertTest = currentIntersection;
						currentIntersection.y -= 1;
						offset = currentVertexWrapped - currentVertex;
					}
					else
					{
						olc::vf2d vOffset = lastVertexWrapped - lastVertex;
						olc::vf2d currentVertLastWrap = currentVertex + vOffset;
						// Do something different based on wrap
						olc::vf2d currentIntersection;
						// Since the last vertex is on the opposite wrap, invert the wrap for the new current vertex
						isWrapped = !isWrapped;
						offset = lastVertexWrapped - lastVertex;

						//// Linelineintersect between edges of screen and two offset vertices
						LineScreenIntersect(lastVertexWrapped, currentVertLastWrap, currentIntersection);
						currentVertTest = currentIntersection;
					}

				}

				olc::vf2d intersection;
				if (LineLineIntersect(currentVertTest, lastVertTest, player.position, vEndPos, intersection))
				{
					intersection -= a.position + offset;

					vIntersectingVerticiesIndicies.push_back(lastVertexIndex);
					vIntersections.push_back(intersection);
					lastWrapped = isWrapped;
					intersectionWrapped = isWrapped;
				}

				lastVertexIndex = i;
				lastVertex = currentVertex;
				lastVertexWrapped = currentVertexWrapped;
			}

			// The asteroid may be wrapped
			if (vIntersections.size() == 1)
			{
				// Create new ray
				olc::vf2d vStartPos = vEndPos;
				olc::vf2d screenOffset = olc::vf2d();

				// Wrap ray around
				if (vStartPos.x == 0.0f) { vStartPos.x = (float)ScreenWidth() - 1; screenOffset.x = ScreenWidth() - 1; }
				else if (vStartPos.x == (float)ScreenWidth() - 1) { vStartPos.x = 0; screenOffset.x = -ScreenWidth(); }
				if (vStartPos.y == 0.0f) { vStartPos.y = (float)ScreenHeight() - 1; screenOffset.y = ScreenHeight() - 1; }
				else if (vStartPos.y == (float)ScreenHeight() - 1) { vStartPos.y = 0; screenOffset.y = -ScreenHeight(); }

				lastVertexIndex = a.vRawVerticies.size() - 1;
				olc::vf2d vNewEndPos;
				CalculateLineEndPosition(player.angle, vStartPos, vNewEndPos);
				// The new ray should not intersect with the already intersected geometry

				for (int i = 0; i < a.vRawVerticies.size(); i++)
				{
					olc::vf2d currentVertex;
					currentVertex.x = a.vRawVerticies[i].x * cosf(a.angle) - a.vRawVerticies[i].y * sinf(a.angle);
					currentVertex.y = a.vRawVerticies[i].x * sinf(a.angle) + a.vRawVerticies[i].y * cosf(a.angle);
					currentVertex += a.position;

					WrapCoordinates(currentVertex, currentVertexWrapped);
					isWrapped = currentVertex != currentVertexWrapped;
					offset = olc::vf2d();
					screenOffset = currentVertexWrapped - currentVertex;

					currentVertTest = currentVertexWrapped;
					lastVertTest = lastVertexWrapped;

					// Check if the vertecies are not both wrapped and if the vertex is the opposite wrap of the first vertex
					if ((olc::vi2d)(lastVertexWrapped - currentVertexWrapped) != (olc::vi2d)(lastVertex - currentVertex))
					{

						sameWrapAsLaser = (currentVertexWrapped - vStartPos).mag() < ScreenHeight() * 0.5f;
						if (!sameWrapAsLaser)
						{
							olc::vf2d vOffset = lastVertexWrapped - lastVertex;
							olc::vf2d currentVertLastWrap = currentVertex + vOffset;

							olc::vf2d currentIntersection;

							//// Linelineintersect between edges of screen and two offset vertices
							LineScreenIntersect(lastVertexWrapped, currentVertLastWrap, currentIntersection);
							currentVertTest = currentIntersection;
							currentIntersection.y -= 1;
							offset = vStartPos - vEndPos;
							isWrapped = lastVertex != lastVertexWrapped;
						}
						else
						{
							olc::vf2d vOffset = currentVertexWrapped - currentVertex;
							olc::vf2d lastVertCurrentWrap = lastVertex + vOffset;

							olc::vf2d currentIntersection;

							// Linelineintersect between edges of screen and two offset vertices
							LineScreenIntersect(currentVertexWrapped, lastVertCurrentWrap, currentIntersection);
							lastVertTest = currentIntersection;
							offset = currentVertexWrapped - currentVertex - screenOffset;
						}
					}

					olc::vf2d intersection;
					if (isWrapped != intersectionWrapped && LineLineIntersect(currentVertTest, lastVertTest, vStartPos, vNewEndPos, intersection))
					{
						intersection -= a.position + offset + screenOffset;
						// - a.position + vertwrapped - vert + offset
						vIntersectingVerticiesIndicies.push_back(lastVertexIndex);
						vIntersections.push_back(intersection);
						break;
					}

					lastVertexIndex = i;
					lastVertex = currentVertex;
					lastVertexWrapped = currentVertexWrapped;
				}

			}

			// Cut the asteroid

			// If we have not hit two sides just ignore
			if (vIntersections.size() < 2)
				continue;

			olc::vf2d averageVertexPosition1;
			olc::vf2d averageVertexPosition2;
			vector<olc::vf2d> vVertices1;
			vector<olc::vf2d> vVertices2;

			// Put indecies into the right order. Somehow...
			if (vIntersectingVerticiesIndicies[0] > vIntersectingVerticiesIndicies[1])
			{
				swap(vIntersectingVerticiesIndicies[0], vIntersectingVerticiesIndicies[1]);
			}
			else
			{
				swap(vIntersections[0], vIntersections[1]);
			}

			olc::vf2d vertToAdd;
			for (int i = 0; i < a.vRawVerticies.size(); i++)
			{
				if (i > vIntersectingVerticiesIndicies[0] && i <= vIntersectingVerticiesIndicies[1])
				{
					Rotate(a.angle, a.vRawVerticies[i], vertToAdd);
					vVertices1.push_back(vertToAdd);

					if (i == vIntersectingVerticiesIndicies[1])
					{
						vVertices1.push_back(vIntersections[0]);
						vVertices2.push_back(vIntersections[0]);
					}
				}
				else
				{
					Rotate(a.angle, a.vRawVerticies[i], vertToAdd);
					vVertices2.push_back(vertToAdd);

					if (i == vIntersectingVerticiesIndicies[0])
					{
						vVertices1.push_back(vIntersections[1]);
						vVertices2.push_back(vIntersections[1]);
					}
				}
			}

			// Make new center
			for (auto& v : vVertices1)
			{
				averageVertexPosition1 += v;
			}

			for (auto& v : vVertices2)
			{
				averageVertexPosition2 += v;
			}

			averageVertexPosition1 /= vVertices1.size();
			averageVertexPosition2 /= vVertices2.size();

			for (auto& v : vVertices1)
			{
				v -= averageVertexPosition1;
			}

			for (auto& v : vVertices2)
			{
				v -= averageVertexPosition2;
			}

			olc::vf2d newPosition1 = a.position + averageVertexPosition1;
			olc::vf2d newPosition2 = a.position + averageVertexPosition2;

			olc::vf2d newVelocity1 = 0.1f * (newPosition1 - newPosition2) + a.velocity;
			olc::vf2d newVelocity2 = 0.1f * (newPosition2 - newPosition1) + a.velocity;

			newAsteroids.push_back({ newPosition2, newVelocity2, 0, vVertices2 });

			a.position = newPosition1;
			a.angle = 0;
			a.velocity = newVelocity1;
			a.vRawVerticies = vVertices1;
			a.CalculateMass();
		}*/
	}

	void ProcessVerticiesForCollision(SpaceObject& obj)
	{
		// map needs compare function. vf2d does not have a compare function.
		// Therefore do not use wrap

		// The indices corespond to eachother in these lists (key, value). wtf?. 
		
		int vertCount = obj.vWorldVerticies.size();

		if (vertCount < 2)
			return;

		obj.vProcessedVerticies.clear();
		obj.vProcessedVerticiesRawIndicies.clear();
		obj.vWorldPositions.clear();

		olc::vf2d lastVert = obj.vWorldVerticies[0];
		olc::vf2d currentVert;
		olc::vf2d nextVert;
		olc::vf2d currentWrap;

		for (int i = 1; i < vertCount; i++)
		{
			// Assign next and current vertex
			nextVert = obj.vWorldVerticies[(i + 1) % vertCount];
			currentVert = obj.vWorldVerticies[i];
			
			// Check Wrap
			WrapCoordinates(currentVert, currentWrap);
			currentWrap = currentWrap - currentVert;

			// If current or next or previous with wrap is not added to map, add it
			auto key = find(obj.vWorldPositions.begin(), obj.vWorldPositions.end(), currentWrap + obj.position);
			int index = std::distance(obj.vWorldPositions.begin(), key);

			// Not found
			if (key == obj.vWorldPositions.end()) {

				std::vector<olc::vf2d> verts;
				verts.push_back(lastVert + currentWrap);
				verts.push_back(currentVert + currentWrap);
				verts.push_back(nextVert + currentWrap);

				obj.vWorldPositions.push_back(currentWrap + obj.position);
				obj.vProcessedVerticies.push_back(verts);
				obj.vProcessedVerticiesRawIndicies.push_back(i-1);
			}
			else
			{
				// Found. Add last current and last vert if not already found in vector
				auto notFound = [](vector<olc::vf2d> v, olc::vf2d vert)
				{
					return !std::count(v.begin(), v.end(), vert);
				};

				auto v = obj.vProcessedVerticies[index];

				if (notFound(v, lastVert + currentWrap))
					v.push_back(lastVert + currentWrap);

				if (notFound(v, currentVert + currentWrap))
					v.push_back(currentVert + currentWrap);

				if (notFound(v, nextVert + currentWrap))
					v.push_back(nextVert + currentWrap);

				obj.vProcessedVerticies[index] = v;
			}

			// Assign last vertex after calculations
			lastVert = currentVert;
		}
	}

	void Rotate(float a, olc::vf2d i, olc::vf2d& o)
	{
		o.x = i.x * cosf(a) - i.y * sinf(a);
		o.y = i.x * sinf(a) + i.y * cosf(a);
	}

	void DrawWireFrameModel(const vector<olc::vf2d>& vecModelCoordinates, olc::vf2d pos, float r = 0.0f, float s = 1.0f, olc::Pixel p = olc::WHITE)
	{
		// Create translated model vector of coordinate pairs
		int verts = vecModelCoordinates.size();
		if (verts == 0)
			return;

		vector<olc::vf2d> vecTransformedCoordinates;
		vecTransformedCoordinates.resize(verts);

		// Rotate
		for (int i = 0; i < verts; i++)
		{
			vecTransformedCoordinates[i].x = vecModelCoordinates[i].x * cosf(r) - vecModelCoordinates[i].y * sinf(r);
			vecTransformedCoordinates[i].y = vecModelCoordinates[i].x * sinf(r) + vecModelCoordinates[i].y * cosf(r);
		}

		// Scale
		for (int i = 0; i < verts; i++)
		{
			vecTransformedCoordinates[i] *= s;
		}

		// Translate
		for (int i = 0; i < verts; i++)
		{
			vecTransformedCoordinates[i] += pos;
		}

		// Draw Closed Polygon
		for (int i = 0; i < verts + 1; i++)
		{
			int j = (i + 1);
			DrawLine(vecTransformedCoordinates[i % verts], vecTransformedCoordinates[j % verts], p);
			//Draw(vecTransformedCoordinates[i % verts].x, vecTransformedCoordinates[i % verts].y, olc::RED);
		}
		//Draw(vecTransformedCoordinates[1].x, vecTransformedCoordinates[1].y, olc::RED);
	}

	void CalculateLineEndPosition(float a, olc::vf2d position, olc::vf2d& vEndPos)
	{
		olc::vi2d vHorizontalIntersect;
		olc::vi2d vVerticalIntersect;

		// horizontal collisions
		if (cosf(a) >= 0)
		{
			// Up
			vVerticalIntersect = olc::vf2d((position.y * tan(a)) + position.x, 0);
		}
		else
		{
			// Down
			vVerticalIntersect = olc::vf2d(((ScreenHeight() - position.y) * (tan(-a))) + position.x, ScreenHeight() - 1);
		}

		// vertical collisions
		if (sinf(a) >= 0)
		{
			// Right side
			vHorizontalIntersect = olc::vf2d(ScreenWidth() - 1, (ScreenWidth() - position.x) * (tan(a + 1.57079f)) + position.y);
		}
		else
		{
			// Left side
			vHorizontalIntersect = olc::vf2d(0, ((position.x) * (-tan(a + 1.57079f))) + position.y);
		}



		vEndPos = (position - vHorizontalIntersect).mag2() > (position - vVerticalIntersect).mag2() ? vVerticalIntersect : vHorizontalIntersect;
	}

	void LineScreenIntersect(olc::vf2d p1, olc::vf2d p2, olc::vf2d& iOut)
	{
		if (LineLineIntersect(p1, p2, olc::vf2d(0,0), olc::vf2d(ScreenWidth(), 0), iOut))
			return;
		if (LineLineIntersect(p1, p2, olc::vf2d(ScreenWidth(), 0), olc::vf2d(ScreenWidth(), ScreenHeight()), iOut))
			return;
		if (LineLineIntersect(p1, p2, olc::vf2d(ScreenWidth(), ScreenHeight()), olc::vf2d(0, ScreenHeight()), iOut))
			return;
		if (LineLineIntersect(p1, p2, olc::vf2d(0, ScreenHeight()), olc::vf2d(0, 0), iOut))
			return;
	}

	///Calculate intersection of two lines.
	//return true if found, false if not found or error
	bool LineLineIntersect(olc::vf2d p1, //Line 1 start
		olc::vf2d p2, //Line 1 end
		olc::vf2d p3, //Line 2 start
		olc::vf2d p4, //Line 2 end
		olc::vf2d& iOut) //Output 
	{
		float d = (p2.x - p1.x) * (p4.y - p3.y) - (p2.y - p1.y) * (p4.x - p3.x);

		if (d == 0.0f)
		{
			return false;
		}

		float u = ((p3.x - p1.x) * (p4.y - p3.y) - (p3.y - p1.y) * (p4.x - p3.x)) / d;
		float v = ((p3.x - p1.x) * (p2.y - p1.y) - (p3.y - p1.y) * (p2.x - p1.x)) / d;

		if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
		{
			return false;
		}

		iOut.x = p1.x + u * (p2.x - p1.x);
		iOut.y = p1.y + u * (p2.y - p1.y);

		return true; //All OK
	}

};

int main()
{
	Disasteroids game;
	if (game.Construct(240, 240, 4, 4))
		game.Start();
	return 0;
}