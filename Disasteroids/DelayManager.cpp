#include "DelayManager.h"

void DelayManager::Update(float fElapsedTime)
{
	for (int i = 0; i < delays.size(); i++)
	{
		delays[i] -= fElapsedTime;
	}
}

void DelayManager::PutOnCooldown(delayTypes type)
{
	int i = static_cast<int>(type);
	delays[i] = durations[i];
}

bool DelayManager::OnCooldown(delayTypes type)
{
	return GetCooldown(type) > 0;
}

float DelayManager::GetCooldown(delayTypes type)
{
	return delays[static_cast<int>(type)];
}

float DelayManager::GetTotalDuration(delayTypes type)
{
	int i = static_cast<int>(type);
	return durations[i];
}