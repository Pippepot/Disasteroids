#pragma once
#include <array>

class DelayManager
{
public:
	enum class delayTypes { throttle = 0, levelSwitch = 1};
	void Update(float fElapsedTime);
	void PutOnCooldown(delayTypes type);
	bool OnCooldown(delayTypes type);
	float GetCooldown(delayTypes type);
	float GetTotalDuration(delayTypes type);
private:
	std::array<float, 2> delays;
	const std::array<float, 2> durations{
		0.2f,	// Throttle
		0.5f	// LevelSwitch
	};
};

