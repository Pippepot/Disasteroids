#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
using namespace std;


// Override base class with your custom functionality
class Asteroids : public olc::PixelGameEngine
{
public:
	Asteroids()
	{
		// Name you application
		sAppName = "Asteroids";
	}

private:
	struct sSpaceObject
	{
		olc::vf2d position;
		olc::vf2d velocity;
		float angle;
		vector<olc::vf2d> vVerticies;
		olc::Pixel color;

	};

	struct sLaser
	{
		olc::vf2d sPosition;
		olc::vf2d ePosition;
		float timeLeft;

	};

	const int nAsteroidSize = 16;
	vector<sSpaceObject> vecAsteroids;
	vector<sLaser> vecLasers;
	const float fLaserTime = 1;
	sSpaceObject player;
	int nScore;
	bool bDead = false;

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
		player.vVerticies = 
		{ 
			{ 0.0f, -5.5f },
			{ -2.5f, 2.5f },
			{ 2.5f, 2.5f }
		};

		int verts = 20;
		for (int i = 0; i < verts; i++)
		{
			// Counter clockwise
			float noise = (float)rand() / (float)RAND_MAX * 0.4f + 0.8f;
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

		vecAsteroids.push_back({ {80, 20.0}, {8.0f, -6.0f}, 0.0f, vecModelAsteroid, olc::YELLOW });
		//vecAsteroids.push_back({ {100.0, 50.0}, {8.0f, -6.0f}, 0.0f, vecModelAsteroid, olc::YELLOW });

		player.position = olc::vf2d(ScreenWidth() * 0.5f, ScreenHeight() * 0.5f);
		player.velocity = olc::vf2d(0, 0);
		player.angle = 0;
		player.color = olc::WHITE;

		nScore = 0;

		bDead = false;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		if (bDead)
			ResetGame();

		Clear(olc::BLACK);

		// Steer player
		if (GetKey(olc::RIGHT).bHeld || GetKey(olc::D).bHeld)
			player.angle += 5.0f * fElapsedTime;
		if (GetKey(olc::LEFT).bHeld || GetKey(olc::A).bHeld)
			player.angle -= 5.0f * fElapsedTime;

		// Thrust / Acceleration
		if (GetKey(olc::UP).bHeld || GetKey(olc::W).bHeld)
			player.velocity += olc::vf2d(sin(player.angle), -cos(player.angle)) * 20.0 * fElapsedTime;

		vector<sSpaceObject> newAsteroids;

		// Shoot laser
		if (GetKey(olc::SPACE).bPressed)
		{
			olc::vi2d vEndPos;
			olc::vi2d vHorizontalIntersect;
			olc::vi2d vVerticalIntersect;
			float a = sin(player.angle) / cos(player.angle);

			// horizontal collisions
			if (cosf(player.angle) >= 0)
			{
				// Up
				vVerticalIntersect = olc::vf2d((player.position.y * tan(player.angle)) + player.position.x, 0);
			}
			else
			{
				// Down
				vVerticalIntersect = olc::vf2d(((ScreenHeight() - player.position.y) * (tan(-player.angle))) + player.position.x, ScreenHeight() - 1);
			}

			// vertical collisions
			if (sinf(player.angle) >= 0)
			{
				// Right side
				vHorizontalIntersect = olc::vf2d(ScreenWidth() - 1, (ScreenWidth() - player.position.x) * (tan(player.angle + 1.57079f)) + player.position.y);
			}
			else
			{
				// Left side
				vHorizontalIntersect = olc::vf2d(0, ((player.position.x) * (-tan(player.angle + 1.57079f))) + player.position.y);
			}

			vEndPos = (player.position - vHorizontalIntersect).mag2() > (player.position - vVerticalIntersect).mag2() ? vVerticalIntersect : vHorizontalIntersect;


			// Intersect with asteroids

			vector<olc::vf2d> vIntersectingVerticies;
			vector<int> vIntersectingVerticiesIndex;

			olc::vf2d laserForward = olc::vf2d(sin(player.angle), -cos(player.angle));
			olc::vf2d laserRight = olc::vf2d(-laserForward.y, laserForward.x);

			vector<olc::vf2d> v1;
			vector<olc::vf2d> v2;

			for (auto& a : vecAsteroids)
			{

				// Loop through every vertex on the asteroid and locate the intersecting verticies
				bool rightSide = laserRight.dot(player.position - (a.position + a.vVerticies.front())) > 0; // which side is the vertex on
				for (int i = 0; i < a.vVerticies.size(); i++)
				{
					bool lastRightSide = rightSide;

					olc::vf2d pos;
					// Wrap the coordinates so we can intersect the vertex lines even when the asteroid is on the other side of the screen
					WrapCoordinates(a.position + a.vVerticies[i], pos);

					// Don't check any verticies behind the player
					if (laserForward.dot(pos - player.position) < 0)
						continue;

					rightSide = laserRight.dot(pos - player.position) > 0;

					if (rightSide)
					{
						v1.push_back(a.vVerticies[i]);
					}
					else
					{
						v2.push_back(a.vVerticies[i]);
					}

					// If the verticies are on different sides of the laser
					if (lastRightSide != rightSide)
					{
						int nLastIndex = i - 1;
						if (nLastIndex < 0)
							nLastIndex += a.vVerticies.size();

						vIntersectingVerticies.push_back(a.vVerticies[nLastIndex]);	
						vIntersectingVerticies.push_back(a.vVerticies[i]);
						vIntersectingVerticiesIndex.push_back(v1.size());
						vIntersectingVerticiesIndex.push_back(v2.size());
					}
				}

				// Cut the asteroid

				// If we have not hit to sides 
				if (vIntersectingVerticies.size() < 4)
					continue;

				olc::vf2d p1;
				olc::vf2d p2;
				LineLineIntersect(a.position + vIntersectingVerticies[0], a.position + vIntersectingVerticies[1], player.position, vEndPos, p1);
				LineLineIntersect(a.position + vIntersectingVerticies[2], a.position + vIntersectingVerticies[3], player.position, vEndPos, p2);
				p1 -= a.position;
				p2 -= a.position;
	

				//split the asteroid in two and add the new verticies
				v1.insert(v1.begin() + vIntersectingVerticiesIndex[0], p1);
				v2.insert(v2.begin() + vIntersectingVerticiesIndex[1], p1);
				v1.insert(v1.begin() + vIntersectingVerticiesIndex[2], p2);
				v2.insert(v2.begin() + vIntersectingVerticiesIndex[3], p2);

				//newAsteroids.push_back({ a.position + olc::vf2d(0,0), laserRight * 2, a.angle, v1, olc::GREY });

				a.velocity = -laserRight * 2;
				a.vVerticies = v2;

				vecLasers.push_back({ a.position + p1,a.position + p1, 1000 });
				vecLasers.push_back({ a.position + p2,a.position + p2, 1000 });
			}

			vecLasers.push_back({ player.position, vEndPos, fLaserTime});
		}


		// Change position
		player.position += player.velocity * fElapsedTime;

		// Wrap spaceship coordiantes
		WrapCoordinates(player.position, player.position);

		//for (auto& a : vecAsteroids)
		//	if (IsPointInsideCircle(a.position, a.nSize, player.position))
		//		bDead = true;

		// Update asteroids position and velocity
		for (auto& a : vecAsteroids)
		{
			//a.position += a.velocity * fElapsedTime;
			//a.angle += 0.5f * fElapsedTime;
			WrapCoordinates(a.position, a.position);
		}
	

		// Update laser time
		for (auto& l : vecLasers)
			l.timeLeft -= fElapsedTime;


		// Remove lasers with no time left
		if (vecLasers.size() > 0)
		{
			auto i = remove_if(vecLasers.begin(), vecLasers.end(), [&](sLaser o) { return (o.timeLeft < 0); });
			if (i != vecLasers.end())
				vecLasers.erase(i);
		}


		// Create the newly added asteroids
		for (auto a : newAsteroids)
			vecAsteroids.push_back(a);


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
				{10.0f * (-player.velocity.y / fLength), 10.0f * player.velocity.x / fLength}, 0.0f, vecModelAsteroid });

			vecAsteroids.push_back({ {-30.0f * (-player.velocity.y / fLength) + player.position.x,
											  -30.0f * player.velocity.x / fLength + player.position.y},
				{-10.0f * (-player.velocity.y / fLength), 10.0f * player.velocity.x / fLength}, 0.0f, vecModelAsteroid });
		}

		

		
		// Draw asteroids
		for (auto& a : vecAsteroids)
			DrawWireFrameModel(a.vVerticies, a.position, a.angle, 1, a.color);

		// Draw lasers
		for (auto& l : vecLasers)
		{
			if (l.timeLeft > 0)
				DrawLine(l.sPosition, l.ePosition, { 0,0,255, (uint8_t)(l.timeLeft * 255 / fLaserTime) });
		}

		// Draw player
		DrawWireFrameModel(player.vVerticies, player.position, player.angle, 1, player.color);

		// Draw score
		DrawString(2, 2, "SCORE: " + to_string(nScore));

		return true;
	}

	void DrawWireFrameModel(const vector<olc::vf2d>& vecModelCoordinates, olc::vf2d pos, float r = 0.0f, float s = 1.0f, olc::Pixel p = olc::WHITE)
	{
		// pair.first = x coordinate
		// pair.second = y coordinate

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
		}
	}

	///Calculate intersection of two lines.
///\return true if found, false if not found or error
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
	Asteroids game;
	if (game.Construct(160, 100, 8, 8))
		game.Start();
	return 0;
}