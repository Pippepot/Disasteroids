#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "Entity.h"
#include "SpaceObject.h"
#include "Laser.h"
#include "Player.h"
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
	vector<SpaceObject> vecAsteroids;
	vector<Laser> vecLasers;
	Player player = Player();
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

		int verts = 16;
		for (int i = 0; i < verts; i++)
		{
			// Counter clockwise
			float noise = (float)rand() / (float)RAND_MAX * 0.4f + 2.0f;
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

		vecAsteroids.push_back({ {ScreenWidth() * 0.5f, ScreenWidth() * 0.1f}, {0.0f, -0.0f}, 0.0f, vecModelAsteroid, olc::YELLOW });
		//vecAsteroids.push_back({ {100.0, 50.0}, {8.0f, -6.0f}, 0.0f, vecModelAsteroid, olc::YELLOW });
		player = Player(olc::vf2d(ScreenWidth() * 0.5f + 10, ScreenHeight() * 0.5f),
			olc::vf2d(0, 0),
			0.047f,
			{
			{ 0.0f, -5.5f },
			{ -2.5f, 2.5f },
			{ 0.0f, -1.0f },
			{ 2.5f, 2.5f }
			},
			olc::WHITE);

		

		nScore = 0;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		if (player.isDead())
			ResetGame();

		Clear(olc::BLACK);


		// Steer player
		if (GetKey(olc::RIGHT).bHeld || GetKey(olc::D).bHeld)
			player.angle += 4.0f * fElapsedTime;
		if (GetKey(olc::LEFT).bHeld || GetKey(olc::A).bHeld)
			player.angle -= 4.0f * fElapsedTime;

		// Thrust / Acceleration
		if (GetKey(olc::UP).bHeld || GetKey(olc::W).bHeld)
			player.velocity += olc::vf2d(sin(player.angle), -cos(player.angle)) * 20.0 * fElapsedTime;

		vector<SpaceObject> newAsteroids;

		// Shoot laser
		if (GetKey(olc::SPACE).bPressed)
		{
			olc::vf2d vEndPos;
			CalculateLineEndPosition(player.angle, player.position, vEndPos);

			// Intersect with asteroids
			for (auto& a : vecAsteroids)
			{
				vector<olc::vf2d> vIntersections;
				vector<int> vIntersectingVerticiesIndicies;
				// Loop through every vertex on the asteroid and locate the intersecting verticies
				int lastVertexIndex = a.vVerticies.size() - 1;
				olc::vf2d currentVertexWrapped;
				olc::vf2d lastVertexWrapped;
				olc::vf2d offset;
				olc::vf2d tempVert1;
				olc::vf2d tempVert2;
				bool useTempVerts = false;
				bool isWrapped;
				bool lastWrapped;

				olc::vf2d lastVertex;
				lastVertex.x = a.vVerticies.back().x * cosf(a.angle) - a.vVerticies.back().y * sinf(a.angle);
				lastVertex.y = a.vVerticies.back().x * sinf(a.angle) + a.vVerticies.back().y * cosf(a.angle);
				lastVertex += a.position;

				WrapCoordinates(lastVertex, lastVertexWrapped);

				for (int i = 0; i < a.vVerticies.size(); i++)
				{
					olc::vf2d currentVertex;
					currentVertex.x = a.vVerticies[i].x * cosf(a.angle) - a.vVerticies[i].y * sinf(a.angle);
					currentVertex.y = a.vVerticies[i].x * sinf(a.angle) + a.vVerticies[i].y * cosf(a.angle);
					currentVertex += a.position;

					WrapCoordinates(currentVertex, currentVertexWrapped);
					isWrapped = currentVertex != currentVertexWrapped;
					offset = isWrapped ? currentVertexWrapped - a.position - a.vVerticies[i] : olc::vf2d(0,0);

					// Check if the vertecies are not both wrapped
					if ((olc::vi2d)(lastVertexWrapped - currentVertexWrapped) != (olc::vi2d)(lastVertex - currentVertex))
					{
						// Check for both last and current vertex

						// Lastvertex
						bool lWrap = lastVertex != lastVertexWrapped;
						olc::vf2d lOffset = lWrap ? currentVertexWrapped - a.position - lastVertex : olc::vf2d(0, 0);
						olc::vf2d currentVertLastWrap = a.vVerticies[i] + offset;

						// Linelineintersect between edges of screen and two offset vertices


						// Make temporary vertex

						// Test against temporary vertex
					}

					olc::vf2d intersection;
					if (LineLineIntersect(lastVertexWrapped, currentVertexWrapped, player.position, vEndPos, intersection))
					{
						intersection -= a.position + offset;

						vIntersectingVerticiesIndicies.push_back(lastVertexIndex);
						vIntersections.push_back(intersection);
						lastWrapped = isWrapped;
					}

					lastVertexIndex = i;
					lastVertex = currentVertex;
					lastVertexWrapped = currentVertexWrapped;
				}

				cout << "Intersection count before: " << vIntersections.size() << endl;
				cout << "angle " << player.angle << endl;
				// The asteroid may be wrapped
				if (vIntersections.size() == 1)
				{
					// Create new ray
					olc::vf2d vStartPos = vEndPos;
					offset = olc::vf2d(0, 0);

					// Wrap around
					if (vStartPos.x == 0.0f) { vStartPos.x = (float)ScreenWidth() - 1; offset.x = ScreenWidth() - 1; }
					else if (vStartPos.x ==  (float)ScreenWidth() - 1) { vStartPos.x = 0; offset.x = 0; }
					if (vStartPos.y == 0.0f) { vStartPos.y = (float)ScreenHeight() - 1; offset.y = ScreenHeight() - 1; }
					else if (vStartPos.y ==  (float)ScreenHeight() - 1) { vStartPos.y = 0; offset.y = 0; }

					lastVertexIndex = a.vVerticies.size() - 1;
					olc::vf2d vNewEndPos;
					CalculateLineEndPosition(player.angle, vStartPos, vNewEndPos);
					// The new ray should not intersect with the already intersected geometry
					 
					for (int i = 0; i < a.vVerticies.size(); i++)
					{
						olc::vf2d currentVertex;
						currentVertex.x = a.vVerticies[i].x * cosf(a.angle) - a.vVerticies[i].y * sinf(a.angle);
						currentVertex.y = a.vVerticies[i].x * sinf(a.angle) + a.vVerticies[i].y * cosf(a.angle);
						currentVertex += a.position;

						WrapCoordinates(currentVertex, currentVertexWrapped);
						isWrapped = currentVertex != currentVertexWrapped;

						// Check if the vertecies are not both wrapped and if the vertex is the opposite wrap of the first vertex
						if (isWrapped == lastWrapped || (olc::vi2d)(lastVertexWrapped - currentVertexWrapped) != (olc::vi2d)(lastVertex - currentVertex))
						{
							lastVertexIndex = i;
							lastVertex = currentVertex;
							lastVertexWrapped = currentVertexWrapped;
							continue;
						}
						cout << currentVertexWrapped << endl;
						olc::vf2d intersection;
						if (LineLineIntersect(lastVertexWrapped, currentVertexWrapped, vStartPos, vNewEndPos, intersection))
						{
							intersection -= a.position + offset;

							vIntersectingVerticiesIndicies.push_back(lastVertexIndex);
							vIntersections.push_back(intersection);
							break;
						}

						lastVertexIndex = i;
						lastVertex = currentVertex;
						lastVertexWrapped = currentVertexWrapped;
					}
					//vecLasers.push_back({ vStartPos, vNewEndPos, olc::BLUE, 1 });
				}

				// Cut the asteroid
				cout << "Intersection count after: " << vIntersections.size() << endl;
 				// If we have not hit two sides 
				if (vIntersections.size() < 2)
					continue;

				olc::vf2d averageVertexPosition1;
				olc::vf2d averageVertexPosition2;
				vector<olc::vf2d> vVertices1;
				vector<olc::vf2d> vVertices2;

				if (vIntersectingVerticiesIndicies[0] > vIntersectingVerticiesIndicies[1])
				{
					swap(vIntersectingVerticiesIndicies[0], vIntersectingVerticiesIndicies[1]);
				}
				else
				{
					swap(vIntersections[0], vIntersections[1]);
				}

				olc::vf2d vertToAdd;
				for (int i = 0; i < a.vVerticies.size(); i++)
				{
					if (i > vIntersectingVerticiesIndicies[0] && i <= vIntersectingVerticiesIndicies[1])
					{
						Rotate(a.angle, a.vVerticies[i], vertToAdd);
						vVertices1.push_back(vertToAdd);

						if (i == vIntersectingVerticiesIndicies[1])
						{
							vVertices1.push_back(vIntersections[0]);
							vVertices2.push_back(vIntersections[0]);
						}
					}
					else
					{
						Rotate(a.angle, a.vVerticies[i], vertToAdd);
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

				newAsteroids.push_back({ newPosition2, newVelocity2, 0, vVertices2, olc::GREY });

				a.position = newPosition1;
				a.angle = 0;
				a.velocity = newVelocity1;
				a.vVerticies = vVertices1;
				a.color = olc::YELLOW;
			}

			vecLasers.push_back({ player.position, vEndPos, olc::BLUE, 1});
		}

		// Create the newly added asteroids
		for (auto a : newAsteroids)
			vecAsteroids.push_back(a);


		UpdateEntities(fElapsedTime);

		CheckWinCondition();

		return true;
	}

	void UpdateEntities(float fElapsedTime)
	{
		DestroyDeadEntities();

		player.Update(fElapsedTime);
		// Wrap spaceship coordiantes
		WrapCoordinates(player.position, player.position);

		// Update asteroids position and velocity
		for (auto& a : vecAsteroids)
		{
			a.Update(fElapsedTime * 0.2f);
			//a.angle += 0.1f * fElapsedTime;
			WrapCoordinates(a.position, a.position);
		}

		for (auto& l : vecLasers)
		{
			l.Update(fElapsedTime);
			l.color.a = (uint8_t)(l.timeLeft * 255 / l.maxTime);
		}

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

		// Remove asteroids
		if (vecAsteroids.size() > 0)
		{
			auto i = remove_if(vecAsteroids.begin(), vecAsteroids.end(), [&](SpaceObject o) { return (o.isDead()); });
			if (i != vecAsteroids.end())
				vecAsteroids.erase(i);
		}

	}

	void DrawEntities()
	{
		// Draw asteroids
		for (auto& a : vecAsteroids)
			DrawWireFrameModel(a.vVerticies, a.position, a.angle, 1, a.color);
	
		// Draw lasers
		for (auto& l : vecLasers)
			DrawLine(l.position, l.endPosition, l.color);

		// Draw player
		DrawWireFrameModel(player.vVerticies, player.position, player.angle, 1, player.color);

		// Draw score
		//DrawString(2, 2, "SCORE: " + to_string(nScore));
	}

	void CheckWinCondition()
	{
		// Level Clear
		if (vecAsteroids.empty())
		{
			nScore += 1000; // Large score for level progression
			vecAsteroids.clear();
			vecLasers.clear();

			// Add two new asteroids, but in a place where the player is not, we'll simply
			// add them 90 degrees left and right to the player, their coordinates will
			// be wrapped by th enext asteroid update
			float fLength = player.velocity.mag();

			vecAsteroids.push_back({ {30.0f * (-player.velocity.y / fLength) + player.position.x,
											  30.0f * player.velocity.x / fLength + player.position.y},
				{10.0f * (-player.velocity.y / fLength), 10.0f * player.velocity.x / fLength}, 0.0f, vecModelAsteroid, olc::YELLOW });

			vecAsteroids.push_back({ {-30.0f * (-player.velocity.y / fLength) + player.position.x,
											  -30.0f * player.velocity.x / fLength + player.position.y},
				{-10.0f * (-player.velocity.y / fLength), 10.0f * player.velocity.x / fLength}, 0.0f, vecModelAsteroid, olc::YELLOW });
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
		vector<olc::vf2d> vecTransformedCoordinates;
		int verts = vecModelCoordinates.size();
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
			Draw(vecTransformedCoordinates[i % verts].x, vecTransformedCoordinates[i % verts].y, olc::RED);
		}
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
			std::cout << "up" << vVerticalIntersect << std::endl;
		}
		else
		{
			// Down
			vVerticalIntersect = olc::vf2d(((ScreenHeight() - position.y) * (tan(-a))) + position.x, ScreenHeight() - 1);
			std::cout << "down";
		}

		// vertical collisions
		if (sinf(a) >= 0)
		{
			// Right side
			vHorizontalIntersect = olc::vf2d(ScreenWidth() - 1, (ScreenWidth() - position.x) * (tan(a + 1.57079f)) + position.y);
			std::cout << "Right";
		}
		else
		{
			// Left side
			vHorizontalIntersect = olc::vf2d(0, ((position.x) * (-tan(a + 1.57079f))) + position.y);
			std::cout << "Left" << vHorizontalIntersect << std::endl;
		}



		vEndPos = (position - vHorizontalIntersect).mag2() > (position - vVerticalIntersect).mag2() ? vVerticalIntersect : vHorizontalIntersect;
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

		if (u < -0.01f || u > 1.0f || v < -0.01f || v > 1.0f)
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
	if (game.Construct(100*2, 100*2, 4, 4))
		game.Start();
	return 0;
}