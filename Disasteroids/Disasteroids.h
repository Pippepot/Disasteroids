#pragma once
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "SpaceObject.h"
#include "ParticleSystem.h"
#include "Laser.h"
#include "DelayManager.h"
#include "KeyCharMap.h"
using namespace std;

class Disasteroids : public olc::PixelGameEngine
{

private:
	const int nAsteroidSize = 16;
	const float nAsteroidBreakMass = 8.0f;
	const float nAsteroidDisintegrateMass = 2.0f;
	int nAsteroidCountAtLevelSwitch;
	vector<SpaceObject> vecAsteroids;
	vector<Laser> vecLasers;
	ParticleSystem particleSystem;
	DelayManager delayManager;
	SpaceObject player;
	bool onTitleScreen = true;
	olc::Decal* titleDecal;
	int level;
	int nScore;

	vector<olc::vf2d> vecModelAsteroid;

};