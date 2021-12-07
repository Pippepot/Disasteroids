#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_SOUND
#include "olcPGEX_Sound.h"

#include "SpaceObject.h"
#include "ParticleSystem.h"
#include "Laser.h"
#include "BlackHole.h"
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
	vector<BlackHole> vecBlackHoles;
	ParticleSystem particleSystem;
	DelayManager delayManager;
	SpaceObject player;
	olc::Decal* titleDecal;
#pragma endregion

#pragma region Tracking
	float titlePositionY = -60;
	bool bOnTitleScreen;
	bool bGameIsStarting;
	bool bSwitchingLevel;
	bool bForceLevelSwitch;
	int nAsteroidCountAtLevelSwitch;
	int nLevel;
	int nScore;
#pragma endregion

#pragma region Audio
	int musicSample; // Missing
	int laserShootSample;
	int asteroidHitSample;
	int asteroidBreakSample;
	int playerBreakSample;
	int playerThrustSample;
	int levelCompleteSample;
#pragma endregion
	
	bool bDebugSkipTitleScreen = false;


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
		// Seed the rand function with the current time
		std::srand(std::time(nullptr));

		InitializeAudio();

		CreateAsteroidModel();

		if (bDebugSkipTitleScreen) {
			bForceLevelSwitch = true;
			ResetGame();
			return true;
		}

		InitializeTitleScreen();

		bOnTitleScreen = true;

		return true;
	}

	void InitializeAudio()
	{
		olc::SOUND::InitialiseAudio(44100, 1, 8, 512);

		//musicSample = olc::SOUND::LoadAudioSample("Audio/SampleA.wav");
		laserShootSample = olc::SOUND::LoadAudioSample("Audio/Laser2.wav");
		asteroidHitSample = olc::SOUND::LoadAudioSample("Audio/AsteroidHit.wav");
		asteroidBreakSample = olc::SOUND::LoadAudioSample("Audio/AsteroidBreak5.wav");
		playerBreakSample = olc::SOUND::LoadAudioSample("Audio/PlayerBreak.wav");
		playerThrustSample = olc::SOUND::LoadAudioSample("Audio/PlayerThrust.wav");
		levelCompleteSample = olc::SOUND::LoadAudioSample("Audio/LevelComplete.wav");

		//olc::SOUND::PlaySample(musicSample);
	}

	void InitializeTitleScreen()
	{
		olc::Sprite* titleSprite = new olc::Sprite("./D6.png");
		titleDecal = new olc::Decal(titleSprite);

		for (int i = 0; i < 4; i++)
		{
			vecAsteroids.push_back({ {(float)(rand() % ScreenWidth()), (float)(rand() % ScreenHeight())},
					{(float)rand() / RAND_MAX * 6.0f, (float)rand() / RAND_MAX * 6.0f},
					(float)rand() / RAND_MAX * 3.14f, vecModelAsteroid,
					olc::YELLOW });
		}

		for (int m = 0; m < vecAsteroids.size(); m++)
			for (int n = m + 1; n < vecAsteroids.size(); n++)
				vecAsteroids[m].ShapeOverlap_DIAGS_STATIC(vecAsteroids[n]);

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

		bSwitchingLevel = true;
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
		vecBlackHoles.clear();

		float angle = (float)rand() / RAND_MAX * 3.14f;
		olc::vf2d velocity = olc::vf2d(sin(angle), -cos(angle)) * 25;

		player = SpaceObject(olc::vf2d(ScreenWidth() * ((float)rand() / RAND_MAX), ScreenHeight() * ((float)rand() / RAND_MAX)),
			velocity,
			angle,
			{
			{ 0.0f, -5.5f },
			{ -2.5f, 2.5f },
			{ 0.0f, -1.0f },
			{ 2.5f, 2.5f }
			},
			olc::WHITE);
		
		// Level will be incremented and asteroids will be spawned in CheckWinCondition()
		nLevel = 0;
		nScore = 0;
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

			if (DestroyAsteroidsOnLevelSwitch(fElapsedTime)) {
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
				if (!delayManager.OnCooldown(type)) {
					bForceLevelSwitch = true;
					ResetGame();
				}
			}
		}
		else
		{
			UpdatePlayerControls(fElapsedTime);
		}

		UpdateEntities(fElapsedTime);

		// Draw score
		DrawString(2, 2, "SCORE: " + to_string(nScore));

		HandleLevelSwitch(fElapsedTime);

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
		if (GetKey(olc::SPACE).bPressed) {
			ShootLaser();

			// There is a chance to spawn a black hole when shooting
			float chance = nLevel * 0.05f;
			if ((float)rand() / RAND_MAX < chance)
				SpawnBlackHole();
		}
	}

	void UpdateEntities(float fElapsedTime)
	{
		DestroyDeadEntities();

		for (auto& hole : vecBlackHoles) {
			hole.Update(fElapsedTime);
			hole.Attract(player, fElapsedTime);
		}

		player.Update(fElapsedTime);
		player.CalculateVerticiesWorldSpace();
		ProcessVerticiesForCollision(player);
		WrapCoordinates(player.position, player.position);

		// Update asteroids position and velocity
		for (auto& a : vecAsteroids)
		{
			for (auto& hole : vecBlackHoles)
				hole.Attract(a, fElapsedTime);

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
						olc::SOUND::PlaySample(playerBreakSample);
						player.Kill();
						return;
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
		// Remove black holes with no time left
		for (int i = vecBlackHoles.size() - 1; i >= 0; i--)
		{
			if (vecBlackHoles[i].isDead())
				vecBlackHoles.erase(vecBlackHoles.begin() + i);
		}

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
		// Draw black holes
		for (auto& h : vecBlackHoles)
			for (int i = 0; i < h.GetSize(); i++)
				DrawCircle(h.position, i, h.color * sin(h.GetRemainingLifeTime() + i));

		// Draw asteroids
		for (auto& a : vecAsteroids) {
			DrawWireFrameModel(a.vWorldVerticies, olc::vf2d(), 0, 1, a.color);
			//for (auto& p : a.vProcessedVerticies) {
			//	DrawWireFrameModel(p, olc::vf2d(), 0, 1, olc::GREEN);

			//}
		}

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

	// Checks if the level is complete
	// also clears the level if it is complete
	// Kinda violates the rule of having methods only do one thing
	void HandleLevelSwitch(float fElapsedTime)
	{
		if (bForceLevelSwitch) {
			bForceLevelSwitch = false;
			LoadNextLevel();
		}

		// Destroy every asteroid then spawn new level (new asteroids)
		if (bSwitchingLevel)
		{
			// If the level is cleared, load the next one
			if (DestroyAsteroidsOnLevelSwitch(fElapsedTime))
				LoadNextLevel();

			return;
		}


		// Check if the level is complete
		if (find_if(vecAsteroids.begin(), vecAsteroids.end(), [&](SpaceObject o) { return (o.mass > nAsteroidBreakMass); }) == vecAsteroids.end())
		{
			delayManager.PutOnCooldown(DelayManager::delayTypes::levelSwitch);
			nAsteroidCountAtLevelSwitch = vecAsteroids.size();
			olc::SOUND::PlaySample(levelCompleteSample);
			bSwitchingLevel = true;
		}
	}

	// Returns true when the level is cleared from asteroids
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

		bool waitForAsteroidsToDie = asteroidsToDestroy > 0;

		for (int i = 0; i < asteroidsToDestroy; i++)
		{
			nScore += vecAsteroids[i].mass;
			vecAsteroids[i].Kill();
		}

		if (waitForAsteroidsToDie)
			return false;

		return levelSwitchTimeLeft <= 0;

	}

	void LoadNextLevel()
	{
		vecAsteroids.clear();
		vecLasers.clear();

		nLevel++;
		bSwitchingLevel = false;

		SpawnAsteroids(min(nLevel, 5));
	}

	void SpawnAsteroids(int amount)
	{
		
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

			vecAsteroids.push_back({ {xPos,
					yPos},
					{8.0f * vyUnit * nLevel, 8.0f * vxUnit * nLevel},
					0.0f, vecModelAsteroid,
					olc::YELLOW });
		}
	}

	void SpawnBlackHole()
	{
		float width = ScreenWidth();
		float height = ScreenHeight();

		float size = (float)rand() / RAND_MAX * 15 + 5;
		float duration = (float)rand() / RAND_MAX * 15 + 5;

		// Spawn asteroid not on the edge
		vecBlackHoles.push_back({ olc::vf2d((ScreenWidth() - size * 2) * ((float)rand() / RAND_MAX) + size, (ScreenHeight() - size * 2) * ((float)rand() / RAND_MAX) + size),
			size,
			duration });
	}

	void ShootLaser()
	{
		olc::SOUND::PlaySample(laserShootSample);

		vector<SpaceObject> newAsteroids;

		olc::vf2d vEndPos;
		CalculateLineScreenIntersection(player.angle, player.position, vEndPos);

		olc::vf2d lineDirection = vEndPos - player.position;
		lineDirection = lineDirection.norm();

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
				// If the laser hit a line segment,
				// break to find the corrosponding segment on the other side of the asteroid
				if (firstIntersectionIndex != -1)
					break;

				int processedVertCount = a.vProcessedVerticies[listIndex].size();

				// For every processed vertex. Check if there is an intersection and where it is
				for (int vertIndex = 0; vertIndex < processedVertCount; vertIndex++)
				{
					if (LineLineIntersect(a.vProcessedVerticies[listIndex][vertIndex], a.vProcessedVerticies[listIndex][(vertIndex + 1) % processedVertCount], player.position, vEndPos, firstIntersection)) {
						// Intersection is in wrapped world space. Transform to world space
						firstIntersection += a.position - a.vWorldPositions[listIndex];
						firstIntersectionIndex = a.vProcessedVerticiesRawIndicies[listIndex][vertIndex];
						break;
					}
				}

			}

			// Laser doesn't hit the asteroid
			if (firstIntersectionIndex == -1)
				continue;

			int worldVertCount = a.vWorldVerticies.size();

			// Find second intersection
			for (int vertIndex = 0; vertIndex < worldVertCount; vertIndex++)
			{
				if (vertIndex == firstIntersectionIndex)
					continue;

				olc::vf2d vNewStartPos = firstIntersection - lineDirection * 1000;
				olc::vf2d vNewEndPos = firstIntersection + lineDirection * 1000;

				if (LineLineIntersect(a.vWorldVerticies[vertIndex], a.vWorldVerticies[(vertIndex + 1) % worldVertCount], vNewStartPos, vNewEndPos, secondIntersection)) {
					secondIntersectionIndex = vertIndex;
					break;
				}
			}

			// Hit both. Slice asteroid
			if (secondIntersectionIndex == -1)
				continue;

			olc::vf2d averageVertexPosition1;
			olc::vf2d averageVertexPosition2;
			vector<olc::vf2d> vVertices1;
			vector<olc::vf2d> vVertices2;

			// Swap so that the first intersection index is always smaller than the second index
			if (firstIntersectionIndex > secondIntersectionIndex) {
				swap(firstIntersectionIndex, secondIntersectionIndex);
				swap(firstIntersection, secondIntersection);
			}

			// Split vertecies into two vectors based on whether they between the first and second intersection or outside
			for (int i = 0; i < worldVertCount; i++)
			{
				// The index is between first intersection (exclusive) and second intersection (inclusive)
				if (i > firstIntersectionIndex && i <= secondIntersectionIndex) // f size - 1 s something
				{
					vVertices1.push_back(a.vWorldVerticies[i]);

					// Add verticies where the intersection lies
					if (i == secondIntersectionIndex)
					{
						vVertices1.push_back(secondIntersection);
						vVertices2.push_back(secondIntersection);
					}
				}
				else // The index is outside first intersection (inclusive) and second intersection (exclusive)
				{
					vVertices2.push_back(a.vWorldVerticies[i]);

					// Add verticies where the intersection lies
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

			// Transform vertex position from global to local space
			for (auto& v : vVertices1)
			{
				v -= averageVertexPosition1;
			}

			for (auto& v : vVertices2)
			{
				v -= averageVertexPosition2;
			}

			// New velocity is the old plus a velocity opposite the cut
			olc::vf2d newVelocity1 = 0.5f * (averageVertexPosition1 - averageVertexPosition2) + a.velocity;
			olc::vf2d newVelocity2 =  0.5f * (averageVertexPosition2 - averageVertexPosition1) + a.velocity;

			// Create new asteroid
			SpaceObject newAsteroid = { averageVertexPosition2, newVelocity2, 0, vVertices2, olc::YELLOW };

			if (newAsteroid.mass <= nAsteroidBreakMass)
				newAsteroid.color = olc::GREY;

			if (newAsteroid.mass <= nAsteroidDisintegrateMass)
				newAsteroid.Kill();

			newAsteroids.push_back(newAsteroid);

			// Addjust old asteroid
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

	}

	void ProcessVerticiesForCollision(SpaceObject& obj)
	{
		int vertCount = obj.vWorldVerticies.size();

		if (vertCount < 2)
			return;

		// Reset values
		obj.vProcessedVerticies.clear();
		obj.vProcessedVerticiesRawIndicies.clear();
		obj.vWorldPositions.clear();

		olc::vf2d lastVert = obj.vWorldVerticies[0];
		olc::vf2d currentVert;
		olc::vf2d nextVert;
		olc::vf2d currentWrap;
		olc::vf2d lastWrap;

		int lastVertIndex = 0;
		int currentVertIndex;
		int nextVertIndex;

		int worldWrapIndex = 0;
		int lastWorldWrapIndex;

		for (int i = 1; i < vertCount + 1; i++)
		{
			// Assign next and current vertex
			currentVertIndex = i % vertCount;
			nextVertIndex = (i + 1) % vertCount;

			currentVert = obj.vWorldVerticies[currentVertIndex];
			nextVert = obj.vWorldVerticies[nextVertIndex];

			lastWrap = currentWrap;

			// Check Wrap
			WrapCoordinates(currentVert, currentWrap);
			currentWrap = currentWrap - currentVert;

			// If current or next or previous with wrap is not added to map, add it
			lastWorldWrapIndex = worldWrapIndex;
			worldWrapIndex = obj.vWorldPositions.size();
			for (int j = 0; j < obj.vWorldPositions.size(); j++)
			{
				if (currentWrap + obj.position == obj.vWorldPositions[j]) {
					worldWrapIndex = j;
					break;
				}
			}

			// Not found
			if (worldWrapIndex == obj.vWorldPositions.size()) {

				obj.vWorldPositions.push_back(currentWrap + obj.position);
				obj.vProcessedVerticiesRawIndicies.push_back({ lastVertIndex, currentVertIndex, nextVertIndex });
				obj.vProcessedVerticies.push_back({
					lastVert + currentWrap,
					currentVert + currentWrap,
					nextVert + currentWrap });

				lastVert = currentVert;
				continue;
			}

			// Found. Add last current and last vert if not already found in vector

			// Push last and next vertex if not already in the vector
			bool lastFound = true;
			bool foundNext = false;
			int size = obj.vProcessedVerticies[worldWrapIndex].size();

			// Avoid extra verticies
			if (i >= vertCount - 1) {
				for (int j = 0; j < size; j++) {
					if (obj.vProcessedVerticies[worldWrapIndex][j] == (nextVert + currentWrap)) {
						foundNext = true;
						break;
					}
				}
				if (i == vertCount) {
					lastFound = false;
					for (int j = 0; j < size; j++) {
						if (obj.vProcessedVerticies[worldWrapIndex][j] == (lastVert + currentWrap)) {
							lastFound = true;
							break;
						}
					}
				}
			}

			if (lastWorldWrapIndex != worldWrapIndex) {
				for (int j = 0; j < size; j++) {
					if (obj.vProcessedVerticies[worldWrapIndex][j] == (currentVert + currentWrap)) {
						break;
					}
					if (j == size - 1) {
						obj.vProcessedVerticies[worldWrapIndex].push_back(currentVert + currentWrap);
						obj.vProcessedVerticiesRawIndicies[worldWrapIndex].push_back({ currentVertIndex });
					}

				} 

			}

			if (!lastFound) {
				obj.vProcessedVerticies[worldWrapIndex].push_back(lastVert + currentWrap);
				obj.vProcessedVerticiesRawIndicies[worldWrapIndex].push_back({ lastVertIndex });
			}

			if (!foundNext) {
				obj.vProcessedVerticies[worldWrapIndex].push_back(nextVert + currentWrap);
				obj.vProcessedVerticiesRawIndicies[worldWrapIndex].push_back({ nextVertIndex });
			}


			// Assign last vertex after calculations
			lastVertIndex = currentVertIndex;
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
			//olc::Pixel color = olc::CYAN;
			//if (i % verts == 0)
			//	color = olc::RED;
			//if (i % verts == 1)
			//	color = olc::DARK_RED;
			//if (i == 2)
			//	color = olc::VERY_DARK_RED;
			//Draw(vecTransformedCoordinates[i % verts].x, vecTransformedCoordinates[i % verts].y, color);
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