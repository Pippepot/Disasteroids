#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_SOUND
#include "olcPGEX_Sound.h"

#include "SpaceObject.h"
#include "ParticleSystem.h"
#include "Laser.h"
#include "DelayManager.h"
#include "KeyCharMap.h"
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

	// Variables
#pragma region Asteroid Settings
	const int nAsteroidSize = 16;
	const float nAsteroidBreakMass = 8.0f;
	const float nAsteroidDisintegrateMass = 2.0f;
#pragma endregion

#pragma region References
	vector<SpaceObject> vecAsteroids;
	vector<Laser> vecLasers;
	ParticleSystem particleSystem;
	DelayManager delayManager;
	SpaceObject player;
	olc::Decal* titleDecal;
#pragma endregion

#pragma region Tracking
	bool bOnTitleScreen = true;
	bool bGameIsStarting;
	float titlePositionY = -60;
	int nAsteroidCountAtLevelSwitch;
	int nLevel;
	int nScore;
#pragma endregion

#pragma region Audio
	int musicSample;
	int laserShootSample;
	int asteroidHitSample;
	int asteroidBreakSample;
	int playerBreakSample;
	int playerThrustSample;
	int levelCompleteSample;
#pragma endregion





	vector<olc::vf2d> vecModelAsteroid;

	// Implements "wrap around" for various in-game sytems
	void WrapCoordinates(float ix, float iy, float& ox, float& oy)
	{
		ox = ix;
		oy = iy;
		if (ix < 0.0f)	ox = ix + (float)ScreenWidth();
		else if (ix >= (float)ScreenWidth())	ox = ix - (float)ScreenWidth();
		if (iy < 0.0f)	oy = iy + (float)ScreenHeight();
		else if (iy >= (float)ScreenHeight()) oy = iy - (float)ScreenHeight();
	}

	void WrapCoordinates(olc::vf2d i, olc::vf2d& o)
	{
		WrapCoordinates(i.x, i.y, o.x, o.y);
	}

	virtual bool Draw(int32_t x, int32_t y, olc::Pixel p = olc::WHITE)
	{
		float ox;
		float oy;
		WrapCoordinates((float)x, (float)y, ox, oy);
		return PixelGameEngine::Draw(ox, oy, p);
	}

public:
	bool OnUserCreate() override
	{
		InitializeAudio();

		CreateAsteroidModel();

		InitializeTitleScreen();

		bOnTitleScreen = true;

		return true;
	}

	void InitializeAudio()
	{
		olc::SOUND::InitialiseAudio(44100, 1, 8, 512);

		musicSample = olc::SOUND::LoadAudioSample("SampleA.wav");
		laserShootSample = olc::SOUND::LoadAudioSample("Laser.wav");
		asteroidHitSample = olc::SOUND::LoadAudioSample("AsteroidHit.wav");
		asteroidBreakSample = olc::SOUND::LoadAudioSample("AsteroidBreak.wav");
		playerBreakSample = olc::SOUND::LoadAudioSample("PlayerBreak.wav");
		playerThrustSample = olc::SOUND::LoadAudioSample("PlayerThrust.wav");

		olc::SOUND::PlaySample(musicSample);
	}

	void InitializeTitleScreen()
	{
		olc::Sprite* titleSprite = new olc::Sprite("./D6.png");
		titleDecal = new olc::Decal(titleSprite);

		for (int i = 0; i < 4; i++)
		{
			vecAsteroids.push_back({ {(float)rand(),
					(float)rand()},
					{(float)rand() / RAND_MAX * 8.0f, (float)rand() / RAND_MAX * 8.0f},
					0.0f, vecModelAsteroid,
					olc::YELLOW });
		}

		ShowTitleScreen(0);
	}

	void ShowTitleScreen(float fElapsedTime) {

		if (!bGameIsStarting)
			DrawString(ScreenWidth() * 0.12f, ScreenHeight() * 0.7f, "PRESS ANY KEY TO START");

		titlePositionY = std::fmin(titlePositionY + fElapsedTime * 20.0f, 0);
		DrawDecal({ 0.0f, titlePositionY },
			titleDecal);

		UpdateEntities(fElapsedTime);

		DrawEntities();
	}

	void StartGame() {
		delayManager.PutOnCooldown(DelayManager::delayTypes::levelSwitch);
		nAsteroidCountAtLevelSwitch = vecAsteroids.size();

		bOnTitleScreen = false;
		bGameIsStarting = true;
	}

	void CreateAsteroidModel() {
		int verts = 24;
		for (int i = 0; i < verts; i++)
		{
			// Counter clockwise
			float noise = (float)rand() / (float)RAND_MAX * 0.6f + 1.6f;
			vecModelAsteroid.push_back({
				noise * sinf(((float)i / (float)verts) * 6.28318f) * nAsteroidSize,
				noise * cosf(((float)i / (float)verts) * 6.28318f) * nAsteroidSize
				});
		}
	}

	void ResetGame()
	{
		vecAsteroids.clear();
		vecLasers.clear();

		player = SpaceObject(olc::vf2d(ScreenWidth() * 0.51f, ScreenHeight() * 0.5f),
			olc::vf2d(0, -25),
			0.0f,
			{
			{ 0.0f, -5.5f },
			{ -2.5f, 2.5f },
			{ 0.0f, -1.0f },
			{ 2.5f, 2.5f }
			},
			olc::WHITE);
		
		nLevel = 5;
		nScore = 0;

		SpawnAsteroids(nLevel);
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::BLACK);

		if (bOnTitleScreen) {

			ShowTitleScreen(fElapsedTime);

			for (auto& m : valueInputKeys) {
				if (GetKey(m.key).bPressed) {
					StartGame();
					return true;
				}
			}

			return true;
		}

		if (bGameIsStarting)
		{
			ShowTitleScreen(fElapsedTime);

			if (!DestroyAsteroidsOnLevelSwitch(fElapsedTime)) {
				bGameIsStarting = false;
				ResetGame();
			}


			return true;
		}

		// Player dead. Reset
		DelayManager::delayTypes type = DelayManager::delayTypes::playerKilled;
		if (player.isDead()) {
			if (!delayManager.OnCooldown(type)) {
				particleSystem.AddParticlesFromVerts(player.vWorldVerticies, player.position, 10, 1, player.color);
				player.vRawVerticies.clear();
				delayManager.PutOnCooldown(type);
			}
			else
			{
				delayManager.Update(fElapsedTime, type);
				if (!delayManager.OnCooldown(type))
					ResetGame();
			}
		}
		else
		{
			UpdatePlayerControls(fElapsedTime);
		}

		UpdateEntities(fElapsedTime);

		// Draw score
		DrawString(2, 2, "SCORE: " + to_string(nScore));

		CheckWinCondition(fElapsedTime);

		return true;
	}

	void UpdatePlayerControls(float fElapsedTime) {

		// Steer player
		if (GetKey(olc::RIGHT).bHeld || GetKey(olc::D).bHeld)
			player.angle += 4.0f * fElapsedTime;
		if (GetKey(olc::LEFT).bHeld || GetKey(olc::A).bHeld)
			player.angle -= 4.0f * fElapsedTime;

		// Thrust / Acceleration
		DelayManager::delayTypes type = DelayManager::delayTypes::throttle;
		if (GetKey(olc::UP).bHeld || GetKey(olc::W).bHeld) {

			olc::vf2d direction = olc::vf2d(sin(player.angle), -cos(player.angle)) * 20.0 * fElapsedTime;
			player.velocity += direction;

			if (!delayManager.OnCooldown(type)) {

				for (int i = 0; i < 3; i++)
				{
					particleSystem.AddParticleFromVerts(player.position, player.velocity + (olc::vf2d(rand(), rand()).norm() * 0.5f - direction.norm()) * 10.0f, 1.0f, olc::DARK_BLUE);
					olc::SOUND::PlaySample(playerThrustSample);
				}

				delayManager.PutOnCooldown(type);
			}
		}

		delayManager.Update(fElapsedTime, type);

		// Shoot laser
		if (GetKey(olc::SPACE).bPressed)
			ShootLaser();
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
			a.Update(fElapsedTime);
			a.angle += 0.1f * fElapsedTime;
			WrapCoordinates(a.position, a.position);
			a.CalculateVerticiesWorldSpace();
			ProcessVerticiesForCollision(a);
		}

		// Check for overlap
		for (int m = 0; m < vecAsteroids.size(); m++)
		{
			if (!player.isDead()) {
				if (vecAsteroids[m].ShapeOverlap_DIAGS_STATIC(player)) {
					// Hit a big asteroid. Game over
					if (vecAsteroids[m].mass >= nAsteroidBreakMass) {
						//olc::SOUND::PlaySample(playerBreakSample);
						//player.Kill();
						//return;
						break;
					}

					nScore += vecAsteroids[m].mass;
					vecAsteroids[m].Kill();
				}
			}


			for (int n = m + 1; n < vecAsteroids.size(); n++)
				vecAsteroids[m].ShapeOverlap_DIAGS_STATIC(vecAsteroids[n]);
		}


		for (auto& l : vecLasers)
			l.Update(fElapsedTime);

		particleSystem.Update(fElapsedTime);

		DrawEntities();
	}

	void DestroyDeadEntities()
	{
		// Remove lasers with no time left
		for (int i = vecLasers.size() - 1; i >= 0; i--)
		{
			if (vecLasers[i].isDead())
				vecLasers.erase(vecLasers.begin() + i);
		}

		// Remove asteroids
		for (int i = vecAsteroids.size() - 1; i >= 0; i--)
		{
			if (vecAsteroids[i].isDead()) {
				SpaceObject a = vecAsteroids[i];
				olc::SOUND::PlaySample(asteroidBreakSample);
				particleSystem.AddParticlesFromVerts(a.vWorldVerticies, a.position, 10, 1.0f, olc::DARK_GREY);
				vecAsteroids.erase(vecAsteroids.begin() + i);
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
		for (auto& p : particleSystem.particles) {
			if (!p.isDead())
				Draw(p.position.x, p.position.y, p.color);
		}

		// Draw player
		DrawWireFrameModel(player.vRawVerticies, player.position, player.angle, 1, player.color);
	}

	void CheckWinCondition(float fElapsedTime)
	{
		if (delayManager.OnCooldown(DelayManager::delayTypes::levelSwitch))
		{
			if (DestroyAsteroidsOnLevelSwitch(fElapsedTime))
				return;

			vecAsteroids.clear();
			vecLasers.clear();

			nLevel++;

			SpawnAsteroids(min(nLevel, 5));

			return;
		}


		// Level Clear
		if (find_if(vecAsteroids.begin(), vecAsteroids.end(), [&](SpaceObject o) { return (o.mass > nAsteroidBreakMass); }) == vecAsteroids.end())
		{
			delayManager.PutOnCooldown(DelayManager::delayTypes::levelSwitch);
			nAsteroidCountAtLevelSwitch = vecAsteroids.size();
		}
	}

	// Returns true if there are any asteroids left
	bool DestroyAsteroidsOnLevelSwitch(float fElapsedTime)
	{
		DelayManager::delayTypes type = DelayManager::delayTypes::levelSwitch;
		float levelSwitchTimeLeft = delayManager.GetCooldown(type);

		delayManager.Update(fElapsedTime, type);
		// Get amount of asteroids to destroy this frame
		float fraction = levelSwitchTimeLeft / delayManager.GetTotalDuration(type);
		int asteroidsToDestroy = vecAsteroids.size() - nAsteroidCountAtLevelSwitch * fraction;

		// Avoid errors
		asteroidsToDestroy = std::fmin(asteroidsToDestroy, vecAsteroids.size());

		for (int i = 0; i < asteroidsToDestroy; i++)
		{
			nScore += vecAsteroids[i].mass;
			vecAsteroids[i].Kill();
		}

		return delayManager.GetCooldown(type) > 0;

	}

	// TODO: Fix. spawn in semicircle around player
	void SpawnAsteroids(int amount) {
		
		float radius = 10 * amount + nAsteroidSize * 2;

		float fLength = player.velocity.mag();

		float vxUnit = player.velocity.x / fLength;
		float vyUnit = player.velocity.y / fLength;

		if (fLength == 0) {
			vxUnit = 0;
			vyUnit = 0;
		}

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

			//float xPos = 20;
			//float yPos = 20;

			vecAsteroids.push_back({ {xPos,
					yPos},
					{8.0f * vyUnit * nLevel, 8.0f * vxUnit * nLevel},
					0.0f, vecModelAsteroid,
					olc::YELLOW });
		}
	}

	void ShootLaser()
	{
		olc::SOUND::PlaySample(laserShootSample);

		vector<SpaceObject> newAsteroids;

		olc::vf2d vEndPos;
		CalculateLineScreenIntersection(player.angle, player.position, vEndPos);

		olc::vf2d lineDirection = vEndPos - player.position;
		lineDirection = lineDirection.norm();

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

		// For every asteroid O(N)
		for (auto& a : vecAsteroids)
		{
			// Broad phase
			bool closeEnough = false;
			for (int i = 0; i < a.vWorldPositions.size(); i++)
			{
				olc::vf2d asteroidPlayerLocal = a.vWorldPositions[i] - player.position;
				// Distance from line to center of asteroid. https://en.wikipedia.org/wiki/Vector_projection#Scalar_rejection
				float distanceToAsteroid = abs(asteroidPlayerLocal.dot(lineDirection.perp()));
				if (distanceToAsteroid < a.boundingCircleRadius) {
					closeEnough = true;
					break;
				}
			}

			if (!closeEnough)
				continue;

			// For every list of processed verticies
			olc::vf2d firstIntersection;
			olc::vf2d secondIntersection;
			int firstIntersectionIndex = -1;
			int secondIntersectionIndex = -1;

			// O(N)
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

			if (newAsteroid.mass <= nAsteroidDisintegrateMass)
				newAsteroid.Kill();

			newAsteroids.push_back(newAsteroid);


			a.position = averageVertexPosition1;
			a.color = olc::YELLOW;
			a.angle = 0;
			a.velocity = newVelocity1;
			a.vRawVerticies = vVertices1;
			a.vWorldVerticies.resize(vVertices1.size());
			a.CalculateMass();

			if (a.mass <= nAsteroidBreakMass)
				a.color = olc::GREY;

			if (a.mass <= nAsteroidDisintegrateMass)
				a.Kill();

			olc::SOUND::PlaySample(asteroidHitSample);
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

						// Linelineintersect between edges of screen and two offset vertices
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

							// Linelineintersect between edges of screen and two offset vertices
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
		olc::vf2d lastWrap;

		// Index of world wraps
		int index = 0;
		int lastIndex;

		for (int i = 1; i < vertCount + 1; i++)
		{
			// Assign next and current vertex
			nextVert = obj.vWorldVerticies[(i + 1) % vertCount];
			currentVert = obj.vWorldVerticies[i % vertCount];

			lastWrap = currentWrap;

			// Check Wrap
			WrapCoordinates(currentVert, currentWrap);
			currentWrap = currentWrap - currentVert;

			// If current or next or previous with wrap is not added to map, add it
			lastIndex = index;
			index = obj.vWorldPositions.size();
			for (int j = 0; j < obj.vWorldPositions.size(); j++)
			{
				if (currentWrap + obj.position == obj.vWorldPositions[j]) {
					index = j;
					break;
				}
			}

			// Not found
			if (index == obj.vWorldPositions.size()) {

				obj.vWorldPositions.push_back(currentWrap + obj.position);
				obj.vProcessedVerticiesRawIndicies.push_back(i-1);
				obj.vProcessedVerticies.push_back({
					lastVert + currentWrap,
					currentVert + currentWrap,
					nextVert + currentWrap });

				lastVert = currentVert;
				continue;
			}

			// Found. Add last current and last vert if not already found in vector

			//// Push last and next vertex if not already in the vector
			bool lastFound = true;
			bool foundNext = false;
			int size = obj.vProcessedVerticies[index].size();

			// Avoid extra verticies
			if (i >= vertCount - 1) {
				for (int j = 0; j < size; j++) {
					if (obj.vProcessedVerticies[index][j] == (nextVert + currentWrap)) {
						foundNext = true;
						break;
					}
				}
				if (i == vertCount) {
					lastFound = false;
					for (int j = 0; j < size; j++) {
						if (obj.vProcessedVerticies[index][j] == (lastVert + currentWrap)) {
							lastFound = true;
							break;
						}
					}
				}
			}

			if (lastIndex != index)
				obj.vProcessedVerticies[index].push_back(currentVert + currentWrap);

			if (!lastFound)
				obj.vProcessedVerticies[index].push_back(lastVert + currentWrap);

			if (!foundNext)
				obj.vProcessedVerticies[index].push_back(nextVert + currentWrap);


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

	void CalculateLineScreenIntersection(float a, olc::vf2d position, olc::vf2d& vEndPos)
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

	bool OnUserDestroy()
	{
		olc::SOUND::DestroyAudio();
		return true;
	}

};

int main()
{
	Disasteroids game;
	if (game.Construct(240, 240, 4, 4))
		game.Start();
	return 0;
}