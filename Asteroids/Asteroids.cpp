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
		int nSize;
		float angle;

	};

	struct sLaser
	{
		olc::vf2d sPosition;
		olc::vf2d ePosition;
		float timeLeft;

	};

	vector<sSpaceObject> vecAsteroids;
	vector<sLaser> vecLasers;
	const float fLaserTime = 2;
	sSpaceObject player;
	int nScore;
	bool bDead = false;

	vector<olc::vf2d> vecModelShip;
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
		vecModelShip = 
		{ 
			{ 0.0f, -5.5f },
			{ -2.5f, 2.5f },
			{ 2.5f, 2.5f }
		};

		int verts = 20;
		for (int i = 0; i < verts; i++)
		{
			float noise = (float)rand() / (float)RAND_MAX * 0.4f + 0.8f;
			vecModelAsteroid.push_back({
				noise * sinf(((float)i / (float)verts) * 6.28318f),
				noise * cosf(((float)i / (float)verts) * 6.28318f)
				});
		}

		ResetGame();

		return true;
	}

	void ResetGame()
	{
		vecAsteroids.clear();
		vecLasers.clear();

		//vecAsteroids.push_back({ {20.0, 20.0}, {8.0f, -6.0f}, (int)16, 0.0f });
		//vecAsteroids.push_back({ {100.0, 20.0}, {-5.0f, -5.0f}, (int)16, 0.0f });

		player.position = olc::vf2d(ScreenWidth() * 0.5f, ScreenHeight() * 0.5f);
		player.velocity = olc::vf2d(0, 0);
		player.angle = 0;

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

			vecLasers.push_back({ player.position, vEndPos, fLaserTime});
		}


		// Change position
		player.position += player.velocity * fElapsedTime;

		// Wrap spaceship coordiantes
		WrapCoordinates(player.position, player.position);

		for (auto& a : vecAsteroids)
			if (IsPointInsideCircle(a.position, a.nSize, player.position))
				bDead = true;

		// Update asteroids position and velocity
		for (auto& a : vecAsteroids)
		{
			a.position += a.velocity * fElapsedTime;
			a.angle += 0.5f * fElapsedTime;
			WrapCoordinates(a.position, a.position);
		}

		vector<sSpaceObject> newAsteroids;
	

		// Update bullet time
		for (auto& l : vecLasers)
			l.timeLeft -= fElapsedTime;


		if (vecLasers.size() > 0)
		{
			auto i = remove_if(vecLasers.begin(), vecLasers.end(), [&](sLaser o) { return (o.timeLeft < 0); });
			if (i != vecLasers.end())
				vecLasers.erase(i);
		}


		for (auto a : newAsteroids)
			vecAsteroids.push_back(a);


		if (vecAsteroids.empty())
		{
			// Level Clear
			nScore += 1000; // Large score for level progression
			vecAsteroids.clear();
			vecLasers.clear();

			// Add two new asteroids, but in a place where the player is not, we'll simply
			// add them 90 degrees left and right to the player, their coordinates will
			// be wrapped by th enext asteroid update
			float fLength = player.velocity.mag();

			vecAsteroids.push_back({ {30.0f * (-player.velocity.y / fLength) + player.position.x,
											  30.0f * player.velocity.x / fLength + player.position.y},
				{10.0f * (-player.velocity.y / fLength), 10.0f * player.velocity.x / fLength}, (int)16, 0.0f });

			vecAsteroids.push_back({ {-30.0f * (-player.velocity.y / fLength) + player.position.x,
											  -30.0f * player.velocity.x / fLength + player.position.y},
				{-10.0f * (-player.velocity.y / fLength), 10.0f * player.velocity.x / fLength}, (int)16, 0.0f });
		}

		

		// Draw lasers
		for (auto& l : vecLasers)
		{
			if (l.timeLeft > 0)
				DrawLine(l.sPosition, l.ePosition, { 0,0,255, (uint8_t)(l.timeLeft * 255 / fLaserTime) });
		}

		
		// Draw asteroids
		for (auto& a : vecAsteroids)
			DrawWireFrameModel(vecModelAsteroid, a.position, a.angle, a.nSize, olc::YELLOW);

		// Draw player
		DrawWireFrameModel(vecModelShip, player.position, player.angle);

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

};

int main()
{
	Asteroids game;
	if (game.Construct(160, 100, 8, 8))
		game.Start();
	return 0;
}