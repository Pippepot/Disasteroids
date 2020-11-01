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

	vector<sSpaceObject> vecAsteroids;
	vector<sSpaceObject> vecBullets;
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
		vecBullets.clear();

		vecAsteroids.push_back({ {20.0, 20.0}, {8.0f, -6.0f}, (int)16, 0.0f });
		vecAsteroids.push_back({ {100.0, 20.0}, {-5.0f, -5.0f}, (int)16, 0.0f });

		player.position = olc::vf2d(ScreenWidth() * 0.5f, ScreenHeight() * 0.5f);
		player.velocity = olc::vf2d(0, -5);
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

		// Shoot bullets
		if (GetKey(olc::SPACE).bPressed)
			vecBullets.push_back({ player.position, {50.0f * sinf(player.angle), -50.0f * cosf(player.angle)}, 0, 0 });

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

		// Update bullet position and velocity
		for (auto& b : vecBullets)
		{
			b.position += b.velocity * fElapsedTime;
			WrapCoordinates(b.position, b.position);

			for (auto& a : vecAsteroids)
			{
				if (IsPointInsideCircle(a.position, a.nSize, b.position))
				{
					// Asteroid hit
					b.position.x = -100;

					if (a.nSize > 4)
					{
						// Create to children
						float angle1 = ((float)rand() / (float)(RAND_MAX) * 6.283185f);
						float angle2 = ((float)rand() / (float)(RAND_MAX) * 6.283185f);

						newAsteroids.push_back({ a.position, {10.0f * sinf(angle1), 10.0f * cosf(angle1)}, (int)a.nSize >> 1 , 0.0f });
						newAsteroids.push_back({ a.position, {10.0f * sinf(angle2), 10.0f * cosf(angle2)}, (int)a.nSize >> 1 , 0.0f });
					}

					// Remove asteroid - Same approach as bullets
					a.position.x = -100;
					nScore += 100;
				}
			}


		}

		for (auto a : newAsteroids)
			vecAsteroids.push_back(a);

		// Remove asteroids that have been blown up
		if (vecAsteroids.size() > 0)
		{
			auto i = remove_if(vecAsteroids.begin(), vecAsteroids.end(), [&](sSpaceObject o) { return (o.position.x < 0); });
			if (i != vecAsteroids.end())
				vecAsteroids.erase(i);
		}

		if (vecAsteroids.empty())
		{
			// Level Clear
			nScore += 1000; // Large score for level progression
			vecAsteroids.clear();
			vecBullets.clear();

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

		// Remove bullets that have gone off screen
		if (vecBullets.size() > 0)
		{
			auto i = remove_if(vecBullets.begin(), vecBullets.end(), [&](sSpaceObject o) { return (o.position.x < 1 || o.position.y < 1 || o.position.x >= ScreenWidth() - 1 || o.position.y >= ScreenHeight() - 1); });
			if (i != vecBullets.end())
				vecBullets.erase(i);
		}

		// Draw bullets
		for (auto& b : vecBullets)
			Draw(b.position.x, b.position.y);
		
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