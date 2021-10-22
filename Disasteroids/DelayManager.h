#pragma once
#include <array>

class DelayManager
{
public:
	enum class delayTypes { throttle = 0, levelSwitch = 1, playerKilled = 2};
	void Update(float fElapsedTime, delayTypes type);
	void PutOnCooldown(delayTypes type);
	bool OnCooldown(delayTypes type);
	float GetCooldown(delayTypes type);
	float GetTotalDuration(delayTypes type);
private:
	std::array<float, 3> delays;
	const std::array<float, 3> durations{
		0.2f,	// Throttle
		3.0f,	// LevelSwitch / Death
		3.0f,	// Player dead
	};
};

